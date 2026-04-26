#include "DnsResourceRecordData.h"

DnsARecordData::DnsARecordData(const std::string& ipAddress) : ipAddress(ipAddress) {}

std::string DnsARecordData::toZoneFileEntry(const std::string& originDomain) const {
    return ipAddress;
}
