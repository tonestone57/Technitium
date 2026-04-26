#include "ZoneLoader.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>
#include <algorithm>

static std::vector<std::string> split(const std::string& s) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (tokenStream >> token) {
        tokens.push_back(token);
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

        auto tokens = split(line);
        if (tokens.size() < 4) continue;

        std::string name = tokens[0];
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        if (!name.empty() && name.back() == '.') name.pop_back();

        uint32_t ttl = std::stoul(tokens[1]);
        std::string typeStr = tokens[3];

        std::shared_ptr<DnsResourceRecordData> data;
        DnsResourceRecordType type;

        if (typeStr == "A") {
            type = DnsResourceRecordType::A;
            data = std::make_shared<DnsARecordData>(tokens[4]);
        } else if (typeStr == "AAAA") {
            type = DnsResourceRecordType::AAAA;
            data = std::make_shared<DnsAAAARecordData>(tokens[4]);
        } else if (typeStr == "CNAME") {
            type = DnsResourceRecordType::CNAME;
            std::string target = tokens[4];
            std::transform(target.begin(), target.end(), target.begin(), ::tolower);
            if (!target.empty() && target.back() == '.') target.pop_back();
            data = std::make_shared<DnsCNAMERecordData>(target);
        } else if (typeStr == "NS") {
            type = DnsResourceRecordType::NS;
            std::string target = tokens[4];
            std::transform(target.begin(), target.end(), target.begin(), ::tolower);
            if (!target.empty() && target.back() == '.') target.pop_back();
            data = std::make_shared<DnsNSRecordData>(target);
        } else if (typeStr == "MX") {
            type = DnsResourceRecordType::MX;
            uint16_t pref = static_cast<uint16_t>(std::stoul(tokens[4]));
            std::string target = tokens[5];
            std::transform(target.begin(), target.end(), target.begin(), ::tolower);
            if (!target.empty() && target.back() == '.') target.pop_back();
            data = std::make_shared<DnsMXRecordData>(pref, target);
        } else if (typeStr == "TXT") {
            type = DnsResourceRecordType::TXT;
            std::string text;
            for (size_t i = 4; i < tokens.size(); ++i) {
                if (i > 4) text += " ";
                text += tokens[i];
            }
            if (text.size() >= 2 && text.front() == '"' && text.back() == '"') {
                text = text.substr(1, text.size() - 2);
            }
            data = std::make_shared<DnsTXTRecordData>(text);
        } else {
            continue;
        }

        zoneManager.addRecord(DnsResourceRecord(name, type, DnsClass::IN, ttl, data));
    }
    return true;
}
