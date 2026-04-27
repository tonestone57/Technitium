#include "DnsQuestionRecord.h"
#include <algorithm>
#include <iostream>
#include <cctype>

DnsQuestionRecord::DnsQuestionRecord(const std::string& name, DnsResourceRecordType type, DnsClass dnsClass)
    : name(name), type(type), dnsClass(dnsClass) {
    if (!this->name.empty() && this->name.back() == '.') {
        this->name.pop_back();
    }
    std::transform(this->name.begin(), this->name.end(), this->name.begin(), ::tolower);
}

std::string DnsQuestionRecord::toString() const {
    std::string typeStr;
    switch (type) {
        case DnsResourceRecordType::A: typeStr = "A"; break;
        case DnsResourceRecordType::AAAA: typeStr = "AAAA"; break;
        case DnsResourceRecordType::NS: typeStr = "NS"; break;
        case DnsResourceRecordType::CNAME: typeStr = "CNAME"; break;
        case DnsResourceRecordType::SOA: typeStr = "SOA"; break;
        case DnsResourceRecordType::PTR: typeStr = "PTR"; break;
        case DnsResourceRecordType::MX: typeStr = "MX"; break;
        case DnsResourceRecordType::TXT: typeStr = "TXT"; break;
        case DnsResourceRecordType::ANY: typeStr = "ANY"; break;
        default: typeStr = "TYPE" + std::to_string(static_cast<uint16_t>(type)); break;
    }

    std::string classStr;
    switch (dnsClass) {
        case DnsClass::IN: classStr = "IN"; break;
        case DnsClass::ANY: classStr = "ANY"; break;
        default: classStr = "CLASS" + std::to_string(static_cast<uint16_t>(dnsClass)); break;
    }

    return name + ". " + typeStr + " " + classStr;
}
