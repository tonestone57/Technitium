#ifndef DNS_RESOURCE_RECORD_H
#define DNS_RESOURCE_RECORD_H

#include "DnsEnums.h"
#include "DnsResourceRecordData.h"
#include <string>
#include <memory>

class DnsResourceRecord {
public:
    DnsResourceRecord(const std::string& name, DnsResourceRecordType type, DnsClass dnsClass, uint32_t ttl, std::shared_ptr<DnsResourceRecordData> rData);

    const std::string& getName() const { return name; }
    DnsResourceRecordType getType() const { return type; }
    DnsClass getDnsClass() const { return dnsClass; }
    uint32_t getTtl() const { return ttl; }
    std::shared_ptr<DnsResourceRecordData> getRData() const { return rData; }

    std::string toZoneFileEntry(const std::string& originDomain = "") const;

private:
    std::string name;
    DnsResourceRecordType type;
    DnsClass dnsClass;
    uint32_t ttl;
    std::shared_ptr<DnsResourceRecordData> rData;
};

#endif // DNS_RESOURCE_RECORD_H
