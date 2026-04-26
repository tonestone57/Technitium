#ifndef DNS_CLIENT_H
#define DNS_CLIENT_H

#include "DnsDatagram.h"
#include <string>

class DnsClient {
public:
    static DnsDatagram query(const std::string& serverIp, int port, const DnsQuestionRecord& question, int timeoutSec = 5);
};

#endif // DNS_CLIENT_H
