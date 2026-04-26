#ifndef ZONE_MANAGER_H
#define ZONE_MANAGER_H

#include "DnsResourceRecord.h"
#include <map>
#include <string>
#include <vector>

class ZoneManager {
public:
    void addRecord(const DnsResourceRecord& record);
    std::vector<DnsResourceRecord> findRecords(const std::string& name, DnsResourceRecordType type, bool& nameExists);

private:
    // Simple map: domain name -> list of records
    std::map<std::string, std::vector<DnsResourceRecord>> records;
};

#endif // ZONE_MANAGER_H
