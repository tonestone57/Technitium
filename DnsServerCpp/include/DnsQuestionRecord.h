#ifndef DNS_QUESTION_RECORD_H
#define DNS_QUESTION_RECORD_H

#include "DnsEnums.h"
#include <string>
#include <vector>
#include <iostream>

class DnsQuestionRecord {
public:
    DnsQuestionRecord(const std::string& name, DnsResourceRecordType type, DnsClass dnsClass);

    const std::string& getName() const { return name; }
    DnsResourceRecordType getType() const { return type; }
    DnsClass getDnsClass() const { return dnsClass; }

    std::string toString() const;

private:
    std::string name;
    DnsResourceRecordType type;
    DnsClass dnsClass;
};

#endif // DNS_QUESTION_RECORD_H
