#ifndef DNS_RESOURCE_RECORD_DATA_H
#define DNS_RESOURCE_RECORD_DATA_H

#include <string>
#include <vector>
#include <memory>
#include <iostream>

class DnsResourceRecordData {
public:
    virtual ~DnsResourceRecordData() = default;
    virtual std::string toZoneFileEntry(const std::string& originDomain = "") const = 0;
};

class DnsARecordData : public DnsResourceRecordData {
public:
    DnsARecordData(const std::string& ipAddress);
    std::string toZoneFileEntry(const std::string& originDomain = "") const override;
    const std::string& getIpAddress() const { return ipAddress; }

private:
    std::string ipAddress;
};

#endif // DNS_RESOURCE_RECORD_DATA_H
