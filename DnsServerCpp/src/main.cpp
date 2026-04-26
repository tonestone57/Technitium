#include "DnsDatagram.h"
#include "ZoneManager.h"
#include "ZoneLoader.h"
#include "DnsClient.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <thread>
#include <poll.h>
#include <cstdio>
#include <functional>

void printUsage(const char* progName) {
    std::cout << "Usage: " << progName << " [-p <port>] [-f <forwarder_ip>] <zone_file>" << std::endl;
    std::cout << "Default port: 53" << std::endl;
}

std::vector<uint8_t> processQuery(const uint8_t* buffer, size_t size, ZoneManager& zoneManager, const std::string& forwarderIp) {
    DnsDatagram request = DnsDatagram::readFrom(buffer, size);
    if (request.getQuestions().empty()) return {};

    DnsDatagram response;
    response.setIdentifier(request.getIdentifier());
    response.setResponse(true);
    response.setOpcode(request.getOpcode());
    response.setRecursionDesired(request.isRecursionDesired());

    DnsResponseCode finalRcode = DnsResponseCode::NoError;
    bool isAuthoritative = true;

    for (const auto& question : request.getQuestions()) {
        response.addQuestion(question);
        bool nameExists = false;
        auto records = zoneManager.findRecords(question.getName(), question.getType(), nameExists);

        if (nameExists) {
            for (const auto& record : records) {
                response.addAnswer(record);
            }
            if (records.empty() && finalRcode == DnsResponseCode::NoError) {
                // NODATA
            }
        } else if (!forwarderIp.empty()) {
            // Not in local zone, try forwarding if it's the only question (common case)
            if (request.getQuestions().size() == 1) {
                try {
                    DnsDatagram forwardResponse = DnsClient::query(forwarderIp, 53, question);
                    forwardResponse.setIdentifier(request.getIdentifier());
                    return forwardResponse.serialize();
                } catch (...) {
                    finalRcode = DnsResponseCode::ServerFailure;
                }
            } else {
                finalRcode = DnsResponseCode::NxDomain;
            }
            isAuthoritative = false;
        } else {
            if (finalRcode == DnsResponseCode::NoError) {
                finalRcode = DnsResponseCode::NxDomain;
            }
        }
    }

    response.setRcode(finalRcode);
    response.setAuthoritativeAnswer(isAuthoritative);
    return response.serialize();
}

void udpServer(int port, ZoneManager& zoneManager, std::string forwarderIp) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) { perror("UDP socket failed"); return; }

    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(static_cast<uint16_t>(port));

    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("UDP bind failed");
        close(sockfd);
        return;
    }

    std::cout << "UDP DNS Server listening on port " << port << "..." << std::endl;

    uint8_t buffer[1024];
    while (true) {
        struct sockaddr_in cliaddr;
        socklen_t len = sizeof(cliaddr);
        ssize_t n = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&cliaddr, &len);
        if (n < 0) continue;

        auto responseBytes = processQuery(buffer, n, zoneManager, forwarderIp);
        if (!responseBytes.empty()) {
            sendto(sockfd, responseBytes.data(), responseBytes.size(), 0, (const struct sockaddr *)&cliaddr, len);
        }
    }
}

void tcpServer(int port, ZoneManager& zoneManager, std::string forwarderIp) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) { perror("TCP socket failed"); return; }

    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(static_cast<uint16_t>(port));

    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("TCP bind failed");
        close(sockfd);
        return;
    }

    listen(sockfd, 5);
    std::cout << "TCP DNS Server listening on port " << port << "..." << std::endl;

    while (true) {
        struct sockaddr_in cliaddr;
        socklen_t len = sizeof(cliaddr);
        int connfd = accept(sockfd, (struct sockaddr *)&cliaddr, &len);
        if (connfd < 0) continue;

        std::thread([connfd, &zoneManager, forwarderIp]() {
            uint8_t lenBuf[2];
            if (recv(connfd, lenBuf, 2, MSG_WAITALL) == 2) {
                uint16_t dnsLen = (lenBuf[0] << 8) | lenBuf[1];
                std::vector<uint8_t> buffer(dnsLen);
                if (recv(connfd, buffer.data(), dnsLen, MSG_WAITALL) == dnsLen) {
                    auto responseBytes = processQuery(buffer.data(), dnsLen, zoneManager, forwarderIp);
                    if (!responseBytes.empty()) {
                        uint16_t resLen = htons(static_cast<uint16_t>(responseBytes.size()));
                        send(connfd, &resLen, 2, 0);
                        send(connfd, responseBytes.data(), responseBytes.size(), 0);
                    }
                }
            }
            close(connfd);
        }).detach();
    }
}

int main(int argc, char* argv[]) {
    int port = 53;
    std::string zoneFile;
    std::string forwarderIp;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-p" && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        } else if (arg == "-f" && i + 1 < argc) {
            forwarderIp = argv[++i];
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

    ZoneManager zoneManager;
    if (!ZoneLoader::load(zoneManager, zoneFile)) {
        std::cerr << "Failed to load zone file: " << zoneFile << "." << std::endl;
        return 1;
    }

    std::thread udpThread(udpServer, port, std::ref(zoneManager), forwarderIp);
    tcpServer(port, zoneManager, forwarderIp);

    udpThread.join();
    return 0;
}
