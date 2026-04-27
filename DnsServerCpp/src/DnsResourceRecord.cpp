#include "DnsResourceRecord.h"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <cctype>

DnsResourceRecord::DnsResourceRecord(const std::string& name, DnsResourceRecordType type, DnsClass dnsClass, uint32_t ttl, std::shared_ptr<DnsResourceRecordData> rData)
    : name(name), type(type), dnsClass(dnsClass), ttl(ttl), rData(rData) {
    if (!this->name.empty() && this->name.back() == '.') {
        this->name.pop_back();
    }
    std::transform(this->name.begin(), this->name.end(), this->name.begin(), ::tolower);
}

std::string DnsResourceRecord::toZoneFileEntry(const std::string& originDomain) const {
    std::stringstream ss;
    ss << std::left << std::setw(20) << name << "  " << std::setw(8) << ttl << "  ";

    switch (dnsClass) {
        case DnsClass::IN: ss << "IN"; break;
        default: ss << "CLASS" << static_cast<uint16_t>(dnsClass); break;
    }
    ss << "  ";

    switch (type) {
        case DnsResourceRecordType::A: ss << std::setw(12) << "A"; break;
        case DnsResourceRecordType::AAAA: ss << std::setw(12) << "AAAA"; break;
        case DnsResourceRecordType::NS: ss << std::setw(12) << "NS"; break;
        case DnsResourceRecordType::CNAME: ss << std::setw(12) << "CNAME"; break;
        case DnsResourceRecordType::SOA: ss << std::setw(12) << "SOA"; break;
        case DnsResourceRecordType::MX: ss << std::setw(12) << "MX"; break;
        case DnsResourceRecordType::TXT: ss << std::setw(12) << "TXT"; break;
        default: ss << "TYPE" << std::setw(8) << static_cast<uint16_t>(type); break;
    }
    ss << "  " << rData->toZoneFileEntry(originDomain);

    return ss.str();
}
