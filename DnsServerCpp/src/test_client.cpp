#include "DnsClient.h"
#include <iostream>

int main() {
    try {
        std::cout << "Querying google.com A record from 8.8.8.8..." << std::endl;
        DnsQuestionRecord q("google.com", DnsResourceRecordType::A, DnsClass::IN);
        DnsDatagram response = DnsClient::query("8.8.8.8", 53, q);

        std::cout << "RCODE: " << (int)response.getRcode() << std::endl;
        std::cout << "Answer section count: " << response.getAnswers().size() << std::endl;
        for (const auto& ans : response.getAnswers()) {
            std::cout << "  - " << ans.getName() << " type " << static_cast<int>(ans.getType()) << " TTL " << ans.getTtl() << std::endl;
            if (ans.getType() == DnsResourceRecordType::A) {
                auto aData = std::static_pointer_cast<DnsARecordData>(ans.getRData());
                std::cout << "    IP: " << aData->getIpAddress() << std::endl;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
