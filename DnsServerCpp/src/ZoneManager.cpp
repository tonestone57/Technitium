#include "ZoneManager.h"
#include <algorithm>
#include <iostream>
#include <cctype>

void ZoneManager::addRecord(const DnsResourceRecord& record) {
    std::string name = record.getName();
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    records[name].push_back(record);
}

std::vector<DnsResourceRecord> ZoneManager::findRecords(const std::string& name, DnsResourceRecordType type, bool& nameExists) {
    std::string searchName = name;
    std::transform(searchName.begin(), searchName.end(), searchName.begin(), ::tolower);
    if (!searchName.empty() && searchName.back() == '.') {
        searchName.pop_back();
    }

    std::vector<DnsResourceRecord> result;
    auto it = records.find(searchName);
    if (it != records.end()) {
        nameExists = true;
        for (const auto& record : it->second) {
            if (type == DnsResourceRecordType::ANY || record.getType() == type) {
                result.push_back(record);
            }
        }
    } else {
        nameExists = false;
    }
    return result;
}
