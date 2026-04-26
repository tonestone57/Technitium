#include "ZoneManager.h"
#include <algorithm>

void ZoneManager::addRecord(const DnsResourceRecord& record) {
    std::string name = record.getName();
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    records[name].push_back(record);
}

std::vector<DnsResourceRecord> ZoneManager::findRecords(const std::string& name, DnsResourceRecordType type) {
    std::string normalizedName = name;
    std::transform(normalizedName.begin(), normalizedName.end(), normalizedName.begin(), ::tolower);

    std::vector<DnsResourceRecord> result;
    auto it = records.find(normalizedName);
    if (it != records.end()) {
        for (const auto& record : it->second) {
            if (type == DnsResourceRecordType::ANY || record.getType() == type) {
                result.push_back(record);
            }
        }
    }
    return result;
}
