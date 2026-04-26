#ifndef ZONE_MANAGER_H
#define ZONE_MANAGER_H

#include "DnsResourceRecord.h"
#include <vector>
#include <string>
#include <map>

class ZoneManager {
public:
    void addRecord(const DnsResourceRecord& record);
    std::vector<DnsResourceRecord> findRecords(const std::string& name, DnsResourceRecordType type, bool& nameExists);
    std::vector<DnsResourceRecord> findSOA(const std::string& name);

private:
    std::map<std::string, std::vector<DnsResourceRecord>> records;
};

#endif // ZONE_MANAGER_H
