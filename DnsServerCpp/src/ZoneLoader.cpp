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
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == '"') {
            inQuotes = !inQuotes;
            // Keep quotes for now to identify quoted tokens later if needed,
            // or just strip them here. Let's strip them for simplicity in the tokens.
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
        if (tokens.size() < 3) continue;

        std::string name = tokens[0];
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        if (!name.empty() && name.back() == '.') name.pop_back();

        uint32_t ttl = 3600;
        size_t typeIdx = 1;

        // Try to parse TTL at index 1
        try {
            size_t pos;
            unsigned long val = std::stoul(tokens[1], &pos);
            if (pos == tokens[1].size()) {
                ttl = static_cast<uint32_t>(val);
                typeIdx = 2;
            }
        } catch (...) {}

        // Skip "IN" if present
        if (typeIdx < tokens.size() && tokens[typeIdx] == "IN") {
            typeIdx++;
        }

        if (typeIdx >= tokens.size()) continue;

        std::string typeStr = tokens[typeIdx];
        std::shared_ptr<DnsResourceRecordData> data;
        DnsResourceRecordType type;
        size_t dataIdx = typeIdx + 1;

        if (typeStr == "A" && dataIdx < tokens.size()) {
            type = DnsResourceRecordType::A;
            data = std::make_shared<DnsARecordData>(tokens[dataIdx]);
        } else if (typeStr == "AAAA" && dataIdx < tokens.size()) {
            type = DnsResourceRecordType::AAAA;
            data = std::make_shared<DnsAAAARecordData>(tokens[dataIdx]);
        } else if (typeStr == "CNAME" && dataIdx < tokens.size()) {
            type = DnsResourceRecordType::CNAME;
            std::string target = tokens[dataIdx];
            std::transform(target.begin(), target.end(), target.begin(), ::tolower);
            if (!target.empty() && target.back() == '.') target.pop_back();
            data = std::make_shared<DnsCNAMERecordData>(target);
        } else if (typeStr == "NS" && dataIdx < tokens.size()) {
            type = DnsResourceRecordType::NS;
            std::string target = tokens[dataIdx];
            std::transform(target.begin(), target.end(), target.begin(), ::tolower);
            if (!target.empty() && target.back() == '.') target.pop_back();
            data = std::make_shared<DnsNSRecordData>(target);
        } else if (typeStr == "MX" && dataIdx + 1 < tokens.size()) {
            try {
                type = DnsResourceRecordType::MX;
                uint16_t pref = static_cast<uint16_t>(std::stoul(tokens[dataIdx]));
                std::string target = tokens[dataIdx + 1];
                std::transform(target.begin(), target.end(), target.begin(), ::tolower);
                if (!target.empty() && target.back() == '.') target.pop_back();
                data = std::make_shared<DnsMXRecordData>(pref, target);
            } catch (...) { continue; }
        } else if (typeStr == "TXT" && dataIdx < tokens.size()) {
            type = DnsResourceRecordType::TXT;
            // Join remaining tokens for TXT if they were split by spaces outside quotes
            std::string text = tokens[dataIdx];
            for (size_t i = dataIdx + 1; i < tokens.size(); ++i) {
                text += " " + tokens[i];
            }
            data = std::make_shared<DnsTXTRecordData>(text);
        } else if (typeStr == "SOA" && dataIdx + 6 < tokens.size()) {
            try {
                type = DnsResourceRecordType::SOA;
                std::string mName = tokens[dataIdx];
                std::string rName = tokens[dataIdx + 1];
                uint32_t serial = std::stoul(tokens[dataIdx + 2]);
                uint32_t refresh = std::stoul(tokens[dataIdx + 3]);
                uint32_t retry = std::stoul(tokens[dataIdx + 4]);
                uint32_t expire = std::stoul(tokens[dataIdx + 5]);
                uint32_t minimum = std::stoul(tokens[dataIdx + 6]);
                data = std::make_shared<DnsSOARecordData>(mName, rName, serial, refresh, retry, expire, minimum);
            } catch (...) { continue; }
        } else {
            continue;
        }

        zoneManager.addRecord(DnsResourceRecord(name, type, DnsClass::IN, ttl, data));
    }
    return true;
}
