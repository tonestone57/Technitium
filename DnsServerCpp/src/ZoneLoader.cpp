#include "ZoneLoader.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <set>

static std::vector<std::string> robustSplit(const std::string& line) {
    std::vector<std::string> tokens;
    std::string current;
    bool inQuotes = false;
    bool escaped = false;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (escaped) {
            current += c;
            escaped = false;
            continue;
        }
        if (c == '\\') {
            escaped = true;
            continue;
        }
        if (c == '"') {
            inQuotes = !inQuotes;
        } else if (std::isspace(c) && !inQuotes) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }
    if (!current.empty()) {
        tokens.push_back(current);
    }
    return tokens;
}

static std::string normalizeDomain(std::string domain, const std::string& origin) {
    if (domain == "@") return origin;
    if (domain.empty()) return origin;
    if (domain.back() != '.') {
        if (!origin.empty()) {
            domain += "." + origin;
        }
    } else {
        domain.pop_back(); // Remove trailing dot for internal storage
    }
    std::transform(domain.begin(), domain.end(), domain.begin(), ::tolower);
    return domain;
}

bool ZoneLoader::load(ZoneManager& zoneManager, const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Could not open zone file: " << filename << std::endl;
        return false;
    }

    std::string line;
    std::string origin;
    std::string lastRecordName;
    uint32_t defaultTtl = 3600;
    bool inMultiline = false;
    std::string multilineBuffer;

    while (std::getline(file, line)) {
        size_t commentPos = line.find(';');
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }
        if (line.empty()) continue;

        if (!inMultiline) {
            if (line.find('(') != std::string::npos && line.find(')') == std::string::npos) {
                inMultiline = true;
                multilineBuffer = line;
                continue;
            }
        } else {
            multilineBuffer += " " + line;
            if (line.find(')') != std::string::npos) {
                inMultiline = false;
                line = multilineBuffer;
            } else {
                continue;
            }
        }

        auto tokens = robustSplit(line);
        if (tokens.empty()) continue;

        if (tokens[0] == "$ORIGIN") {
            if (tokens.size() > 1) {
                origin = tokens[1];
                if (!origin.empty() && origin.back() == '.') origin.pop_back();
                std::transform(origin.begin(), origin.end(), origin.begin(), ::tolower);
            }
            continue;
        }

        if (tokens[0] == "$TTL") {
            if (tokens.size() > 1) {
                try {
                    defaultTtl = static_cast<uint32_t>(std::stoul(tokens[1]));
                } catch (...) {}
            }
            continue;
        }

        size_t tokenIdx = 0;
        std::string name;

        if (std::isspace(line[0])) {
            name = lastRecordName;
        } else {
            name = normalizeDomain(tokens[0], origin);
            lastRecordName = name;
            tokenIdx++;
        }

        if (tokenIdx >= tokens.size()) continue;

        uint32_t ttl = defaultTtl;
        // Try to parse TTL
        try {
            size_t pos;
            unsigned long val = std::stoul(tokens[tokenIdx], &pos);
            if (pos == tokens[tokenIdx].size()) {
                ttl = static_cast<uint32_t>(val);
                tokenIdx++;
            }
        } catch (...) {}

        if (tokenIdx >= tokens.size()) continue;

        // Skip "IN" if present
        if (tokens[tokenIdx] == "IN") {
            tokenIdx++;
        }

        if (tokenIdx >= tokens.size()) continue;

        std::string typeStr = tokens[tokenIdx];
        tokenIdx++;

        std::shared_ptr<DnsResourceRecordData> data;
        DnsResourceRecordType type;
        size_t dataIdx = tokenIdx;

        if (typeStr == "A" && dataIdx < tokens.size()) {
            type = DnsResourceRecordType::A;
            data = std::make_shared<DnsARecordData>(tokens[dataIdx]);
        } else if (typeStr == "AAAA" && dataIdx < tokens.size()) {
            type = DnsResourceRecordType::AAAA;
            data = std::make_shared<DnsAAAARecordData>(tokens[dataIdx]);
        } else if (typeStr == "CNAME" && dataIdx < tokens.size()) {
            type = DnsResourceRecordType::CNAME;
            data = std::make_shared<DnsCNAMERecordData>(normalizeDomain(tokens[dataIdx], origin));
        } else if (typeStr == "NS" && dataIdx < tokens.size()) {
            type = DnsResourceRecordType::NS;
            data = std::make_shared<DnsNSRecordData>(normalizeDomain(tokens[dataIdx], origin));
        } else if (typeStr == "PTR" && dataIdx < tokens.size()) {
            type = DnsResourceRecordType::PTR;
            data = std::make_shared<DnsPTRRecordData>(normalizeDomain(tokens[dataIdx], origin));
        } else if (typeStr == "MX" && dataIdx + 1 < tokens.size()) {
            try {
                type = DnsResourceRecordType::MX;
                uint16_t pref = static_cast<uint16_t>(std::stoul(tokens[dataIdx]));
                data = std::make_shared<DnsMXRecordData>(pref, normalizeDomain(tokens[dataIdx + 1], origin));
            } catch (...) { continue; }
        } else if (typeStr == "TXT" && dataIdx < tokens.size()) {
            type = DnsResourceRecordType::TXT;
            std::string text = tokens[dataIdx];
            for (size_t i = dataIdx + 1; i < tokens.size(); ++i) {
                text += " " + tokens[i];
            }
            data = std::make_shared<DnsTXTRecordData>(text);
        } else if (typeStr == "SOA" && dataIdx < tokens.size()) {
            try {
                type = DnsResourceRecordType::SOA;
                std::vector<std::string> soaTokens;
                for (size_t i = dataIdx; i < tokens.size(); ++i) {
                    if (tokens[i] != "(" && tokens[i] != ")") {
                        soaTokens.push_back(tokens[i]);
                    }
                }
                if (soaTokens.size() < 7) continue;

                std::string mName = normalizeDomain(soaTokens[0], origin);
                std::string rName = normalizeDomain(soaTokens[1], origin);
                uint32_t serial = std::stoul(soaTokens[2]);
                uint32_t refresh = std::stoul(soaTokens[3]);
                uint32_t retry = std::stoul(soaTokens[4]);
                uint32_t expire = std::stoul(soaTokens[5]);
                uint32_t minimum = std::stoul(soaTokens[6]);
                data = std::make_shared<DnsSOARecordData>(mName, rName, serial, refresh, retry, expire, minimum);
            } catch (...) { continue; }
        } else {
            continue;
        }

        zoneManager.addRecord(DnsResourceRecord(name, type, DnsClass::IN, ttl, data));
    }
    return true;
}
