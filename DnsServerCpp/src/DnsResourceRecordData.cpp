#include "DnsResourceRecordData.h"

DnsARecordData::DnsARecordData(const std::string& ipAddress) : ipAddress(ipAddress) {}
std::string DnsARecordData::toZoneFileEntry(const std::string& originDomain) const { return ipAddress; }

DnsAAAARecordData::DnsAAAARecordData(const std::string& ipAddress) : ipAddress(ipAddress) {}
std::string DnsAAAARecordData::toZoneFileEntry(const std::string& originDomain) const { return ipAddress; }

DnsCNAMERecordData::DnsCNAMERecordData(const std::string& domain) : domain(domain) {}
std::string DnsCNAMERecordData::toZoneFileEntry(const std::string& originDomain) const { return domain + "."; }

DnsNSRecordData::DnsNSRecordData(const std::string& domain) : domain(domain) {}
std::string DnsNSRecordData::toZoneFileEntry(const std::string& originDomain) const { return domain + "."; }

DnsMXRecordData::DnsMXRecordData(uint16_t preference, const std::string& domain) : preference(preference), domain(domain) {}
std::string DnsMXRecordData::toZoneFileEntry(const std::string& originDomain) const { return std::to_string(preference) + " " + domain + "."; }

DnsTXTRecordData::DnsTXTRecordData(const std::string& text) : text(text) {}
std::string DnsTXTRecordData::toZoneFileEntry(const std::string& originDomain) const { return "\"" + text + "\""; }
