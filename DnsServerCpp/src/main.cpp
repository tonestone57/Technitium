#include "DnsDatagram.h"
#include "ZoneManager.h"
#include "ZoneLoader.h"
#include "DnsClient.h"
#include "ThreadPool.h"
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
#include <csignal>

void ignore_sigpipe() {
#ifndef _WIN32
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGPIPE, &sa, nullptr);
#endif
}

void printUsage(const char* progName) {
    std::cout << "Usage: " << progName << " [-p <port>] [-f <forwarder_ip[:port]>] <zone_file>" << std::endl;
    std::cout << "Default port: 53" << std::endl;
}

std::vector<uint8_t> processQuery(const uint8_t* buffer, size_t size, ZoneManager& zoneManager, const std::string& forwarderIp) {
    DnsDatagram request = DnsDatagram::readFrom(buffer, size);
    if (!request.isParsedSuccessfully() || request.getQuestions().empty()) return {};

    DnsDatagram response;
    response.setIdentifier(request.getIdentifier());
    response.setResponse(true);
    response.setOpcode(request.getOpcode());
    response.setRecursionDesired(request.isRecursionDesired());

    // Always echo questions back as per DNS standard
    for (const auto& q : request.getQuestions()) {
        response.addQuestion(q);
    }

    bool allLocal = true;
    for (const auto& question : request.getQuestions()) {
        bool nameExists = false;
        zoneManager.findRecords(question.getName(), question.getType(), nameExists);
        if (!nameExists) {
            allLocal = false;
            break;
        }
    }

    if (allLocal || forwarderIp.empty()) {
        DnsResponseCode finalRcode = DnsResponseCode::NoError;
        bool isAuthoritative = true;

        for (const auto& question : request.getQuestions()) {
            bool nameExists = false;
            auto records = zoneManager.findRecords(question.getName(), question.getType(), nameExists);

            if (nameExists) {
                if (records.empty()) {
                    // NODATA: Name exists but no records of this type.
                    // RCODE remains NoError. Authority should have SOA.
                    auto soaRecords = zoneManager.findSOA(question.getName());
                    for (const auto& soa : soaRecords) response.addAuthority(soa);
                } else {
                    for (const auto& record : records) {
                        response.addAnswer(record);
                    }
                }
            } else {
                // NXDOMAIN: Name does not exist.
                finalRcode = DnsResponseCode::NxDomain;
                auto soaRecords = zoneManager.findSOA(question.getName());
                for (const auto& soa : soaRecords) response.addAuthority(soa);
            }
        }
        response.setRcode(finalRcode);
        response.setAuthoritativeAnswer(isAuthoritative);
        return response.serialize();
    } else {
        // Forwarding
        try {
            DnsDatagram forwardResponse = DnsClient::query(forwarderIp, 53, request.getQuestions());
            if (forwardResponse.isParsedSuccessfully()) {
                forwardResponse.setIdentifier(request.getIdentifier());
                return forwardResponse.serialize();
            }
        } catch (...) {}

        // If forwarding fails, return ServerFailure but still with echoed questions
        response.setRcode(DnsResponseCode::ServerFailure);
        response.setAuthoritativeAnswer(false);
        return response.serialize();
    }
}

volatile sig_atomic_t stopServer = 0;
void handleSignal(int sig) {
    stopServer = 1;
}

void udpServer(int port, ZoneManager& zoneManager, std::string forwarderIp, ThreadPool& pool) {
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

    while (!stopServer) {
        struct pollfd pfd;
        pfd.fd = sockfd;
        pfd.events = POLLIN;
        if (poll(&pfd, 1, 1000) <= 0) continue;

        struct sockaddr_in cliaddr;
        socklen_t len = sizeof(cliaddr);
        std::vector<uint8_t> buffer(4096);
        ssize_t n = recvfrom(sockfd, buffer.data(), buffer.size(), 0, (struct sockaddr *)&cliaddr, &len);
        if (n < 0) {
            continue;
        }
        buffer.resize(static_cast<size_t>(n));

        pool.enqueue([sockfd, buffer = std::move(buffer), &zoneManager, forwarderIp, cliaddr, len]() {
            auto responseBytes = processQuery(buffer.data(), buffer.size(), zoneManager, forwarderIp);
            if (!responseBytes.empty()) {
                sendto(sockfd, responseBytes.data(), responseBytes.size(), 0, (const struct sockaddr *)&cliaddr, len);
            }
        });
    }
    close(sockfd);
}

void tcpServer(int port, ZoneManager& zoneManager, std::string forwarderIp, ThreadPool& pool) {
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

    listen(sockfd, 10);
    std::cout << "TCP DNS Server listening on port " << port << "..." << std::endl;

    while (!stopServer) {
        struct pollfd pfd;
        pfd.fd = sockfd;
        pfd.events = POLLIN;
        if (poll(&pfd, 1, 1000) <= 0) continue;

        struct sockaddr_in cliaddr;
        socklen_t len = sizeof(cliaddr);
        int connfd = accept(sockfd, (struct sockaddr *)&cliaddr, &len);
        if (connfd < 0) continue;

        pool.enqueue([connfd, &zoneManager, forwarderIp]() {
            struct pollfd cpfd;
            cpfd.fd = connfd;
            cpfd.events = POLLIN;
            if (poll(&cpfd, 1, 5000) > 0) {
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
            }
            close(connfd);
        });
    }
    close(sockfd);
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

    signal(SIGINT, handleSignal);
    signal(SIGTERM, handleSignal);
    ignore_sigpipe();

    ZoneManager zoneManager;
    if (!ZoneLoader::load(zoneManager, zoneFile)) {
        std::cerr << "Failed to load zone file: " << zoneFile << "." << std::endl;
        return 1;
    }

    ThreadPool pool(10);
    std::thread udpThread(udpServer, port, std::ref(zoneManager), forwarderIp, std::ref(pool));
    tcpServer(port, zoneManager, forwarderIp, pool);

    udpThread.join();
    std::cout << "Server stopped." << std::endl;
    return 0;
}
