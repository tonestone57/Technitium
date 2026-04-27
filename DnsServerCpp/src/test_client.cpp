#include "DnsClient.h"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cout << "Usage: test_client <server_ip> <port> <domain> <type>" << std::endl;
        return 1;
    }

    std::string serverIp = argv[1];
    int port = std::stoi(argv[2]);
    std::string domain = argv[3];
    std::string typeStr = argv[4];

    DnsResourceRecordType type = DnsResourceRecordType::A;
    if (typeStr == "AAAA") type = DnsResourceRecordType::AAAA;
    else if (typeStr == "CNAME") type = DnsResourceRecordType::CNAME;
    else if (typeStr == "NS") type = DnsResourceRecordType::NS;
    else if (typeStr == "MX") type = DnsResourceRecordType::MX;
    else if (typeStr == "TXT") type = DnsResourceRecordType::TXT;
    else if (typeStr == "SOA") type = DnsResourceRecordType::SOA;
    else if (typeStr == "PTR") type = DnsResourceRecordType::PTR;
    else if (typeStr == "ANY") type = DnsResourceRecordType::ANY;

    try {
        std::cout << "Querying " << domain << " " << typeStr << " record from " << serverIp << ":" << port << "..." << std::endl;
        DnsQuestionRecord q(domain, type, DnsClass::IN);
        DnsDatagram response = DnsClient::query(serverIp, port, q);

        std::cout << "RCODE: " << (int)response.getRcode() << std::endl;
        std::cout << "Authoritative: " << (response.isAuthoritativeAnswer() ? "Yes" : "No") << std::endl;
        std::cout << "Answer section count: " << response.getAnswers().size() << std::endl;
        for (const auto& ans : response.getAnswers()) {
            std::cout << "  - [Answer] " << ans.getName() << " type " << static_cast<int>(ans.getType()) << " TTL " << ans.getTtl() << std::endl;
            if (ans.getType() == DnsResourceRecordType::A) {
                auto aData = std::static_pointer_cast<DnsARecordData>(ans.getRData());
                std::cout << "    IP: " << aData->getIpAddress() << std::endl;
            } else if (ans.getType() == DnsResourceRecordType::TXT) {
                auto txtData = std::static_pointer_cast<DnsTXTRecordData>(ans.getRData());
                std::cout << "    TXT: " << txtData->getText() << std::endl;
            } else if (ans.getType() == DnsResourceRecordType::SOA) {
                auto soaData = std::static_pointer_cast<DnsSOARecordData>(ans.getRData());
                std::cout << "    SOA: " << soaData->getMName() << " " << soaData->getRName() << " Serial: " << soaData->getSerial() << std::endl;
            }
        }
        std::cout << "Authority section count: " << response.getAuthorities().size() << std::endl;
        for (const auto& auth : response.getAuthorities()) {
            std::cout << "  - [Authority] " << auth.getName() << " type " << static_cast<int>(auth.getType()) << " TTL " << auth.getTtl() << std::endl;
            if (auth.getType() == DnsResourceRecordType::SOA) {
                auto soaData = std::static_pointer_cast<DnsSOARecordData>(auth.getRData());
                std::cout << "    SOA: " << soaData->getMName() << " " << soaData->getRName() << " Serial: " << soaData->getSerial() << std::endl;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
