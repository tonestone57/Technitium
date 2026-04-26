#include "DnsDatagram.h"
#include <arpa/inet.h>
#include <cstring>
#include <stdexcept>
#include <algorithm>
#include <cctype>

DnsDatagram::DnsDatagram() {}

static uint16_t readUint16(const uint8_t* buffer, size_t& offset, size_t size) {
    if (offset + 2 > size) throw std::runtime_error("Buffer overflow");
    uint16_t value = (static_cast<uint16_t>(buffer[offset]) << 8) | static_cast<uint16_t>(buffer[offset + 1]);
    offset += 2;
    return value;
}

static uint32_t readUint32(const uint8_t* buffer, size_t& offset, size_t size) {
    if (offset + 4 > size) throw std::runtime_error("Buffer overflow");
    uint32_t value = (static_cast<uint32_t>(buffer[offset]) << 24) |
                     (static_cast<uint32_t>(buffer[offset + 1]) << 16) |
                     (static_cast<uint32_t>(buffer[offset + 2]) << 8) |
                     static_cast<uint32_t>(buffer[offset + 3]);
    offset += 4;
    return value;
}

static std::shared_ptr<DnsResourceRecordData> parseRData(const uint8_t* buffer, size_t size, size_t& offset, DnsResourceRecordType type, uint16_t rdlen) {
    size_t startOffset = offset;
    std::shared_ptr<DnsResourceRecordData> data;

    switch (type) {
        case DnsResourceRecordType::A: {
            if (rdlen != 4) throw std::runtime_error("Invalid A record length");
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, buffer + offset, ip, INET_ADDRSTRLEN);
            data = std::make_shared<DnsARecordData>(ip);
            offset += 4;
            break;
        }
        case DnsResourceRecordType::AAAA: {
            if (rdlen != 16) throw std::runtime_error("Invalid AAAA record length");
            char ip[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, buffer + offset, ip, INET6_ADDRSTRLEN);
            data = std::make_shared<DnsAAAARecordData>(ip);
            offset += 16;
            break;
        }
        case DnsResourceRecordType::CNAME: {
            data = std::make_shared<DnsCNAMERecordData>(DnsDatagram::readDomainName(buffer, size, offset));
            break;
        }
        case DnsResourceRecordType::NS: {
            data = std::make_shared<DnsNSRecordData>(DnsDatagram::readDomainName(buffer, size, offset));
            break;
        }
        case DnsResourceRecordType::MX: {
            uint16_t pref = readUint16(buffer, offset, size);
            std::string domain = DnsDatagram::readDomainName(buffer, size, offset);
            data = std::make_shared<DnsMXRecordData>(pref, domain);
            break;
        }
        case DnsResourceRecordType::TXT: {
            std::string text;
            size_t pos = 0;
            while (pos < rdlen) {
                uint8_t len = buffer[offset++];
                text.append(reinterpret_cast<const char*>(buffer + offset), len);
                offset += len;
                pos += 1 + len;
            }
            data = std::make_shared<DnsTXTRecordData>(text);
            break;
        }
        case DnsResourceRecordType::SOA: {
            std::string mName = DnsDatagram::readDomainName(buffer, size, offset);
            std::string rName = DnsDatagram::readDomainName(buffer, size, offset);
            uint32_t serial = readUint32(buffer, offset, size);
            uint32_t refresh = readUint32(buffer, offset, size);
            uint32_t retry = readUint32(buffer, offset, size);
            uint32_t expire = readUint32(buffer, offset, size);
            uint32_t minimum = readUint32(buffer, offset, size);
            data = std::make_shared<DnsSOARecordData>(mName, rName, serial, refresh, retry, expire, minimum);
            break;
        }
        default:
            offset += rdlen;
            break;
    }

    offset = startOffset + rdlen;
    return data;
}

static void parseRecords(const uint8_t* buffer, size_t size, size_t& offset, uint16_t count, std::vector<DnsResourceRecord>& output) {
    for (int i = 0; i < count; ++i) {
        std::string name = DnsDatagram::readDomainName(buffer, size, offset);
        uint16_t type = readUint16(buffer, offset, size);
        uint16_t dnsClass = readUint16(buffer, offset, size);
        uint32_t ttl = readUint32(buffer, offset, size);
        uint16_t rdlen = readUint16(buffer, offset, size);

        auto rData = parseRData(buffer, size, offset, static_cast<DnsResourceRecordType>(type), rdlen);
        if (rData) {
            output.emplace_back(name, static_cast<DnsResourceRecordType>(type), static_cast<DnsClass>(dnsClass), ttl, rData);
        }
    }
}

DnsDatagram DnsDatagram::readFrom(const uint8_t* buffer, size_t size) {
    DnsDatagram datagram;
    if (size < 12) return datagram;

    size_t offset = 0;
    try {
        datagram.identifier = readUint16(buffer, offset, size);

        uint8_t flags1 = buffer[offset++];
        uint8_t flags2 = buffer[offset++];

        datagram.qr = (flags1 >> 7) & 0x01;
        datagram.opcode = static_cast<DnsOpcode>((flags1 >> 3) & 0x0F);
        datagram.aa = (flags1 >> 2) & 0x01;
        datagram.tc = (flags1 >> 1) & 0x01;
        datagram.rd = flags1 & 0x01;

        datagram.ra = (flags2 >> 7) & 0x01;
        datagram.rcode = static_cast<DnsResponseCode>(flags2 & 0x0F);

        uint16_t qdcount = readUint16(buffer, offset, size);
        uint16_t ancount = readUint16(buffer, offset, size);
        uint16_t nscount = readUint16(buffer, offset, size);
        uint16_t arcount = readUint16(buffer, offset, size);

        for (int i = 0; i < qdcount; ++i) {
            std::string name = readDomainName(buffer, size, offset);
            uint16_t type = readUint16(buffer, offset, size);
            uint16_t dnsClass = readUint16(buffer, offset, size);
            datagram.addQuestion(DnsQuestionRecord(name, static_cast<DnsResourceRecordType>(type), static_cast<DnsClass>(dnsClass)));
        }

        parseRecords(buffer, size, offset, ancount, datagram.answers);
        parseRecords(buffer, size, offset, nscount, datagram.authorities);
        parseRecords(buffer, size, offset, arcount, datagram.additionals);

    } catch (const std::exception& e) {
        std::cerr << "DNS Parsing Error: " << e.what() << std::endl;
    }

    return datagram;
}

static void writeUint16(std::vector<uint8_t>& buffer, uint16_t value) {
    buffer.push_back(static_cast<uint8_t>(value >> 8));
    buffer.push_back(static_cast<uint8_t>(value & 0xFF));
}

static void writeUint32(std::vector<uint8_t>& buffer, uint32_t value) {
    buffer.push_back(static_cast<uint8_t>(value >> 24));
    buffer.push_back(static_cast<uint8_t>(value >> 16));
    buffer.push_back(static_cast<uint8_t>(value >> 8));
    buffer.push_back(static_cast<uint8_t>(value & 0xFF));
}

static void serializeRecordList(std::vector<uint8_t>& buffer, const std::vector<DnsResourceRecord>& records, std::map<std::string, uint16_t>& domainOffsets) {
    for (const auto& r : records) {
        DnsDatagram::writeDomainName(buffer, r.getName(), domainOffsets);
        writeUint16(buffer, static_cast<uint16_t>(r.getType()));
        writeUint16(buffer, static_cast<uint16_t>(r.getDnsClass()));
        writeUint32(buffer, r.getTtl());

        size_t rdlen_offset = buffer.size();
        writeUint16(buffer, 0); // Placeholder for RDLENGTH

        size_t rdata_start = buffer.size();
        switch (r.getType()) {
            case DnsResourceRecordType::A: {
                auto aData = std::static_pointer_cast<DnsARecordData>(r.getRData());
                struct in_addr addr;
                if (inet_pton(AF_INET, aData->getIpAddress().c_str(), &addr) == 1) {
                    uint32_t ip = ntohl(addr.s_addr);
                    writeUint32(buffer, ip);
                } else {
                    writeUint32(buffer, 0);
                }
                break;
            }
            case DnsResourceRecordType::AAAA: {
                auto aaaaData = std::static_pointer_cast<DnsAAAARecordData>(r.getRData());
                struct in6_addr addr;
                if (inet_pton(AF_INET6, aaaaData->getIpAddress().c_str(), &addr) == 1) {
                    buffer.insert(buffer.end(), addr.s6_addr, addr.s6_addr + 16);
                } else {
                    buffer.resize(buffer.size() + 16, 0);
                }
                break;
            }
            case DnsResourceRecordType::CNAME: {
                auto cnameData = std::static_pointer_cast<DnsCNAMERecordData>(r.getRData());
                DnsDatagram::writeDomainName(buffer, cnameData->getDomain(), domainOffsets);
                break;
            }
            case DnsResourceRecordType::NS: {
                auto nsData = std::static_pointer_cast<DnsNSRecordData>(r.getRData());
                DnsDatagram::writeDomainName(buffer, nsData->getDomain(), domainOffsets);
                break;
            }
            case DnsResourceRecordType::MX: {
                auto mxData = std::static_pointer_cast<DnsMXRecordData>(r.getRData());
                writeUint16(buffer, mxData->getPreference());
                DnsDatagram::writeDomainName(buffer, mxData->getDomain(), domainOffsets);
                break;
            }
            case DnsResourceRecordType::TXT: {
                auto txtData = std::static_pointer_cast<DnsTXTRecordData>(r.getRData());
                std::string text = txtData->getText();
                size_t pos = 0;
                while (pos < text.size()) {
                    size_t chunkLen = std::min(text.size() - pos, static_cast<size_t>(255));
                    buffer.push_back(static_cast<uint8_t>(chunkLen));
                    buffer.insert(buffer.end(), text.begin() + pos, text.begin() + pos + chunkLen);
                    pos += chunkLen;
                }
                if (text.empty()) buffer.push_back(0);
                break;
            }
            case DnsResourceRecordType::SOA: {
                auto soaData = std::static_pointer_cast<DnsSOARecordData>(r.getRData());
                DnsDatagram::writeDomainName(buffer, soaData->getMName(), domainOffsets);
                DnsDatagram::writeDomainName(buffer, soaData->getRName(), domainOffsets);
                writeUint32(buffer, soaData->getSerial());
                writeUint32(buffer, soaData->getRefresh());
                writeUint32(buffer, soaData->getRetry());
                writeUint32(buffer, soaData->getExpire());
                writeUint32(buffer, soaData->getMinimum());
                break;
            }
            default:
                break;
        }
        size_t rdata_end = buffer.size();
        uint16_t rdlen = static_cast<uint16_t>(rdata_end - rdata_start);
        buffer[rdlen_offset] = static_cast<uint8_t>(rdlen >> 8);
        buffer[rdlen_offset + 1] = static_cast<uint8_t>(rdlen & 0xFF);
    }
}

std::vector<uint8_t> DnsDatagram::serialize() const {
    std::vector<uint8_t> buffer;
    std::map<std::string, uint16_t> domainOffsets;

    writeUint16(buffer, identifier);

    uint8_t flags1 = (static_cast<uint8_t>(qr) << 7) | (static_cast<uint8_t>(opcode) << 3) | (static_cast<uint8_t>(aa) << 2) | (static_cast<uint8_t>(tc) << 1) | static_cast<uint8_t>(rd);
    uint8_t flags2 = (static_cast<uint8_t>(ra) << 7) | (static_cast<uint16_t>(rcode) & 0x0F);

    buffer.push_back(flags1);
    buffer.push_back(flags2);

    writeUint16(buffer, static_cast<uint16_t>(questions.size()));
    writeUint16(buffer, static_cast<uint16_t>(answers.size()));
    writeUint16(buffer, static_cast<uint16_t>(authorities.size()));
    writeUint16(buffer, static_cast<uint16_t>(additionals.size()));

    for (const auto& q : questions) {
        writeDomainName(buffer, q.getName(), domainOffsets);
        writeUint16(buffer, static_cast<uint16_t>(q.getType()));
        writeUint16(buffer, static_cast<uint16_t>(q.getDnsClass()));
    }

    serializeRecordList(buffer, answers, domainOffsets);
    serializeRecordList(buffer, authorities, domainOffsets);
    serializeRecordList(buffer, additionals, domainOffsets);

    return buffer;
}

std::string DnsDatagram::readDomainName(const uint8_t* buffer, size_t size, size_t& offset) {
    std::string domain;
    bool jump = false;
    size_t current_offset = offset;
    size_t next_offset = 0;
    int jumps_count = 0;
    const int max_jumps = 10;

    while (current_offset < size && buffer[current_offset] != 0) {
        uint8_t length = buffer[current_offset];
        if ((length & 0xC0) == 0xC0) {
            if (current_offset + 1 >= size) throw std::runtime_error("Truncated domain name pointer");
            if (!jump) next_offset = current_offset + 2;
            current_offset = ((length & 0x3F) << 8) | buffer[current_offset + 1];
            jump = true;
            if (++jumps_count > max_jumps) throw std::runtime_error("Too many domain name jumps (circular reference?)");
        } else {
            if (current_offset + 1 + length > size) throw std::runtime_error("Truncated domain name label");
            if (!domain.empty()) domain += ".";
            for (int i = 0; i < length; ++i) {
                domain += (char)buffer[current_offset + 1 + i];
            }
            current_offset += 1 + length;
        }
    }

    if (!jump) offset = current_offset + 1;
    else offset = next_offset;

    if (offset > size) offset = size;

    return domain;
}

void DnsDatagram::writeDomainName(std::vector<uint8_t>& buffer, const std::string& domain, std::map<std::string, uint16_t>& domainOffsets) {
    std::string normalizedDomain = domain;
    if (!normalizedDomain.empty() && normalizedDomain.back() == '.') {
        normalizedDomain.pop_back();
    }
    std::transform(normalizedDomain.begin(), normalizedDomain.end(), normalizedDomain.begin(), ::tolower);

    if (normalizedDomain.empty()) {
        buffer.push_back(0);
        return;
    }

    std::string currentSuffix = normalizedDomain;
    while (!currentSuffix.empty()) {
        auto it = domainOffsets.find(currentSuffix);
        if (it != domainOffsets.end()) {
            uint16_t pointer = 0xC000 | it->second;
            writeUint16(buffer, pointer);
            return;
        }

        if (buffer.size() < 0x4000) {
            domainOffsets[currentSuffix] = static_cast<uint16_t>(buffer.size());
        }

        size_t dotPos = currentSuffix.find('.');
        std::string label;
        if (dotPos == std::string::npos) {
            label = currentSuffix;
            currentSuffix = "";
        } else {
            label = currentSuffix.substr(0, dotPos);
            currentSuffix = currentSuffix.substr(dotPos + 1);
        }

        if (label.size() > 63) label = label.substr(0, 63);
        buffer.push_back(static_cast<uint8_t>(label.size()));
        buffer.insert(buffer.end(), label.begin(), label.end());
    }

    buffer.push_back(0);
}
