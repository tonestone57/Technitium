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

#endif // DNS_RESOURCE_RECORD_DATA_H
