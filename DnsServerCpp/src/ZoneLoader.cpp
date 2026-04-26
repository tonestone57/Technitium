#include "ZoneLoader.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>
#include <algorithm>
#include <cctype>

static std::vector<std::string> robustSplit(const std::string& line) {
    std::vector<std::string> tokens;
    std::string current;
    bool inQuotes = false;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
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

bool ZoneLoader::load(ZoneManager& zoneManager, const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Could not open zone file: " << filename << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == ';') continue;

        auto tokens = robustSplit(line);
        if (tokens.size() < 5) continue; // Minimum: name, ttl, class, type, data

        std::string name = tokens[0];
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        if (!name.empty() && name.back() == '.') name.pop_back();

        uint32_t ttl;
        try {
            ttl = std::stoul(tokens[1]);
        } catch (...) {
            continue;
        }

        std::string typeStr = tokens[3];
        std::shared_ptr<DnsResourceRecordData> data;
        DnsResourceRecordType type;

        if (typeStr == "A") {
            if (tokens.size() < 5) continue;
            type = DnsResourceRecordType::A;
            data = std::make_shared<DnsARecordData>(tokens[4]);
        } else if (typeStr == "AAAA") {
            if (tokens.size() < 5) continue;
            type = DnsResourceRecordType::AAAA;
            data = std::make_shared<DnsAAAARecordData>(tokens[4]);
        } else if (typeStr == "CNAME") {
            if (tokens.size() < 5) continue;
            type = DnsResourceRecordType::CNAME;
            std::string target = tokens[4];
            std::transform(target.begin(), target.end(), target.begin(), ::tolower);
            if (!target.empty() && target.back() == '.') target.pop_back();
            data = std::make_shared<DnsCNAMERecordData>(target);
        } else if (typeStr == "NS") {
            if (tokens.size() < 5) continue;
            type = DnsResourceRecordType::NS;
            std::string target = tokens[4];
            std::transform(target.begin(), target.end(), target.begin(), ::tolower);
            if (!target.empty() && target.back() == '.') target.pop_back();
            data = std::make_shared<DnsNSRecordData>(target);
        } else if (typeStr == "MX") {
            if (tokens.size() < 6) continue;
            type = DnsResourceRecordType::MX;
            uint16_t pref = static_cast<uint16_t>(std::stoul(tokens[4]));
            std::string target = tokens[5];
            std::transform(target.begin(), target.end(), target.begin(), ::tolower);
            if (!target.empty() && target.back() == '.') target.pop_back();
            data = std::make_shared<DnsMXRecordData>(pref, target);
        } else if (typeStr == "TXT") {
            if (tokens.size() < 5) continue;
            type = DnsResourceRecordType::TXT;
            data = std::make_shared<DnsTXTRecordData>(tokens[4]);
        } else if (typeStr == "SOA") {
            if (tokens.size() < 11) continue;
            type = DnsResourceRecordType::SOA;
            std::string mName = tokens[4];
            std::string rName = tokens[5];
            uint32_t serial = std::stoul(tokens[6]);
            uint32_t refresh = std::stoul(tokens[7]);
            uint32_t retry = std::stoul(tokens[8]);
            uint32_t expire = std::stoul(tokens[9]);
            uint32_t minimum = std::stoul(tokens[10]);
            data = std::make_shared<DnsSOARecordData>(mName, rName, serial, refresh, retry, expire, minimum);
        } else {
            continue;
        }

        zoneManager.addRecord(DnsResourceRecord(name, type, DnsClass::IN, ttl, data));
    }
    return true;
}
