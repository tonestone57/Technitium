#include "DnsDatagram.h"
#include "ZoneManager.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <vector>

int main() {
    std::cout << "Starting C++ DNS Server for Haiku OS..." << std::endl;

    ZoneManager zoneManager;
    // Add a sample record: example.com. A 127.0.0.1
    zoneManager.addRecord(DnsResourceRecord("example.com", DnsResourceRecordType::A, DnsClass::IN, 3600, std::make_shared<DnsARecordData>("127.0.0.1")));

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        return 1;
    }

    struct sockaddr_in servaddr, cliaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(5353); // Using 5353 for testing if 53 is privileged

    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind failed");
        close(sockfd);
        return 1;
    }

    std::cout << "DNS Server listening on port 5353..." << std::endl;

    uint8_t buffer[1024];
    while (true) {
        socklen_t len = sizeof(cliaddr);
        ssize_t n = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&cliaddr, &len);
        if (n < 0) {
            perror("recvfrom failed");
            continue;
        }

        try {
            DnsDatagram request = DnsDatagram::readFrom(buffer, n);
            if (request.getQuestions().empty()) continue;

            DnsDatagram response;
            response.setIdentifier(request.getIdentifier());
            response.setResponse(true);
            response.setAuthoritativeAnswer(true);

            for (const auto& question : request.getQuestions()) {
                response.addQuestion(question);
                auto records = zoneManager.findRecords(question.getName(), question.getType());
                for (const auto& record : records) {
                    response.addAnswer(record);
                }
                if (records.empty()) {
                    response.setRcode(DnsResponseCode::NxDomain);
                }
            }

            std::vector<uint8_t> responseBytes = response.serialize();
            sendto(sockfd, responseBytes.data(), responseBytes.size(), 0, (const struct sockaddr *)&cliaddr, len);

            std::cout << "Handled request for: " << request.getQuestions()[0].getName() << " (RCODE: " << (int)response.getRcode() << ")" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error processing request: " << e.what() << std::endl;
        }
    }

    close(sockfd);
    return 0;
}
