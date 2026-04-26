#ifndef DNS_RESOURCE_RECORD_DATA_H
#define DNS_RESOURCE_RECORD_DATA_H

#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <cstdint>

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

class DnsAAAARecordData : public DnsResourceRecordData {
public:
    DnsAAAARecordData(const std::string& ipAddress);
    std::string toZoneFileEntry(const std::string& originDomain = "") const override;
    const std::string& getIpAddress() const { return ipAddress; }

private:
    std::string ipAddress;
};

class DnsCNAMERecordData : public DnsResourceRecordData {
public:
    DnsCNAMERecordData(const std::string& domain);
    std::string toZoneFileEntry(const std::string& originDomain = "") const override;
    const std::string& getDomain() const { return domain; }

private:
    std::string domain;
};

class DnsNSRecordData : public DnsResourceRecordData {
public:
    DnsNSRecordData(const std::string& domain);
    std::string toZoneFileEntry(const std::string& originDomain = "") const override;
    const std::string& getDomain() const { return domain; }

private:
    std::string domain;
};

class DnsPTRRecordData : public DnsResourceRecordData {
public:
    DnsPTRRecordData(const std::string& domain);
    std::string toZoneFileEntry(const std::string& originDomain = "") const override;
    const std::string& getDomain() const { return domain; }

private:
    std::string domain;
};

class DnsMXRecordData : public DnsResourceRecordData {
public:
    DnsMXRecordData(uint16_t preference, const std::string& domain);
    std::string toZoneFileEntry(const std::string& originDomain = "") const override;
    uint16_t getPreference() const { return preference; }
    const std::string& getDomain() const { return domain; }

private:
    uint16_t preference;
    std::string domain;
};

class DnsTXTRecordData : public DnsResourceRecordData {
public:
    DnsTXTRecordData(const std::string& text);
    std::string toZoneFileEntry(const std::string& originDomain = "") const override;
    const std::string& getText() const { return text; }

private:
    std::string text;
};

class DnsSOARecordData : public DnsResourceRecordData {
public:
    DnsSOARecordData(const std::string& mName, const std::string& rName, uint32_t serial, uint32_t refresh, uint32_t retry, uint32_t expire, uint32_t minimum);
    std::string toZoneFileEntry(const std::string& originDomain = "") const override;

    const std::string& getMName() const { return mName; }
    const std::string& getRName() const { return rName; }
    uint32_t getSerial() const { return serial; }
    uint32_t getRefresh() const { return refresh; }
    uint32_t getRetry() const { return retry; }
    uint32_t getExpire() const { return expire; }
    uint32_t getMinimum() const { return minimum; }

private:
    std::string mName;
    std::string rName;
    uint32_t serial;
    uint32_t refresh;
    uint32_t retry;
    uint32_t expire;
    uint32_t minimum;
};

#endif // DNS_RESOURCE_RECORD_DATA_H
