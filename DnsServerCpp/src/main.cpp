#include "DnsDatagram.h"
#include "ZoneManager.h"
#include "ZoneLoader.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <vector>
#include <string>

void printUsage(const char* progName) {
    std::cout << "Usage: " << progName << " [-p <port>] <zone_file>" << std::endl;
    std::cout << "Default port: 53" << std::endl;
}

int main(int argc, char* argv[]) {
    int port = 53;
    std::string zoneFile;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-p" && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        } else if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else {
            zoneFile = arg;
        }
    }

    if (zoneFile.empty()) {
        printUsage(argv[0]);
        return 1;
    }

    std::cout << "Starting C++ DNS Server for Haiku OS..." << std::endl;

    ZoneManager zoneManager;
    if (ZoneLoader::load(zoneManager, zoneFile)) {
        std::cout << "Loaded zone file: " << zoneFile << std::endl;
    } else {
        std::cerr << "Failed to load zone file: " << zoneFile << "." << std::endl;
        return 1;
    }

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        return 1;
    }

    struct sockaddr_in servaddr, cliaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(static_cast<uint16_t>(port));

    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind failed");
        if (port == 53) {
            std::cerr << "Note: Port 53 usually requires root privileges." << std::endl;
        }
        close(sockfd);
        return 1;
    }

    std::cout << "DNS Server listening on port " << port << "..." << std::endl;

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
                bool nameExists = false;
                auto records = zoneManager.findRecords(question.getName(), question.getType(), nameExists);
                for (const auto& record : records) {
                    response.addAnswer(record);
                }
                if (records.empty()) {
                    if (nameExists) {
                        response.setRcode(DnsResponseCode::NoError);
                    } else {
                        response.setRcode(DnsResponseCode::NxDomain);
                    }
                }
            }

            std::vector<uint8_t> responseBytes = response.serialize();
            sendto(sockfd, responseBytes.data(), responseBytes.size(), 0, (const struct sockaddr *)&cliaddr, len);

            std::cout << "Handled request for: " << request.getQuestions()[0].getName()
                      << " type " << static_cast<int>(request.getQuestions()[0].getType())
                      << " (RCODE: " << (int)response.getRcode() << ")" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error processing request: " << e.what() << std::endl;
        }
    }

    close(sockfd);
    return 0;
}
