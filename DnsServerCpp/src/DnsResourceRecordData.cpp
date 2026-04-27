#include "DnsResourceRecordData.h"

DnsARecordData::DnsARecordData(const std::string& ipAddress) : ipAddress(ipAddress) {}
std::string DnsARecordData::toZoneFileEntry(const std::string& originDomain) const { return ipAddress; }

DnsAAAARecordData::DnsAAAARecordData(const std::string& ipAddress) : ipAddress(ipAddress) {}
std::string DnsAAAARecordData::toZoneFileEntry(const std::string& originDomain) const { return ipAddress; }

DnsCNAMERecordData::DnsCNAMERecordData(const std::string& domain) : domain(domain) {}
std::string DnsCNAMERecordData::toZoneFileEntry(const std::string& originDomain) const { return domain + "."; }

DnsNSRecordData::DnsNSRecordData(const std::string& domain) : domain(domain) {}
std::string DnsNSRecordData::toZoneFileEntry(const std::string& originDomain) const { return domain + "."; }

DnsPTRRecordData::DnsPTRRecordData(const std::string& domain) : domain(domain) {}
std::string DnsPTRRecordData::toZoneFileEntry(const std::string& originDomain) const { return domain + "."; }

DnsMXRecordData::DnsMXRecordData(uint16_t preference, const std::string& domain) : preference(preference), domain(domain) {}
std::string DnsMXRecordData::toZoneFileEntry(const std::string& originDomain) const { return std::to_string(preference) + " " + domain + "."; }

DnsTXTRecordData::DnsTXTRecordData(const std::string& text) : text(text) {}
std::string DnsTXTRecordData::toZoneFileEntry(const std::string& originDomain) const { return "\"" + text + "\""; }

DnsSOARecordData::DnsSOARecordData(const std::string& mName, const std::string& rName, uint32_t serial, uint32_t refresh, uint32_t retry, uint32_t expire, uint32_t minimum)
    : mName(mName), rName(rName), serial(serial), refresh(refresh), retry(retry), expire(expire), minimum(minimum) {}

std::string DnsSOARecordData::toZoneFileEntry(const std::string& originDomain) const {
    return mName + ". " + rName + ". (" + std::to_string(serial) + " " + std::to_string(refresh) + " " + std::to_string(retry) + " " + std::to_string(expire) + " " + std::to_string(minimum) + ")";
}
