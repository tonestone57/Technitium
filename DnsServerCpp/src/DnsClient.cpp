#include "DnsClient.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <stdexcept>
#include <poll.h>
#include <cstring>
#include <random>
#include <netdb.h>

DnsDatagram DnsClient::query(const std::string& serverIp, int port, const DnsQuestionRecord& question, int timeoutSec) {
    return query(serverIp, port, std::vector<DnsQuestionRecord>{question}, timeoutSec);
}

DnsDatagram DnsClient::query(const std::string& serverIp, int port, const std::vector<DnsQuestionRecord>& questions, int timeoutSec) {
    std::string ip = serverIp;
    std::string portStr = std::to_string(port);

    // Robust parsing for IP:PORT, handling IPv6 [addr]:port
    if (!serverIp.empty()) {
        if (serverIp[0] == '[') {
            size_t bracketClose = serverIp.find(']');
            if (bracketClose != std::string::npos) {
                ip = serverIp.substr(1, bracketClose - 1);
                size_t colon = serverIp.find(':', bracketClose);
                if (colon != std::string::npos) {
                    portStr = serverIp.substr(colon + 1);
                }
            }
        } else {
            size_t firstColon = serverIp.find(':');
            size_t lastColon = serverIp.rfind(':');
            // If there's only one colon, it's likely IPv4:port
            if (firstColon != std::string::npos && firstColon == lastColon) {
                ip = serverIp.substr(0, firstColon);
                portStr = serverIp.substr(firstColon + 1);
            }
        }
    }

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC; // IPv4 or IPv6
    hints.ai_socktype = SOCK_DGRAM;

    if (getaddrinfo(ip.c_str(), portStr.c_str(), &hints, &res) != 0) {
        throw std::runtime_error("Invalid server address or port: " + serverIp);
    }

    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0) {
        freeaddrinfo(res);
        throw std::runtime_error("Socket creation failed");
    }

    DnsDatagram request;
    static std::random_device rd;
    thread_local std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 65535);
    request.setIdentifier(static_cast<uint16_t>(dis(gen)));
    request.setRecursionDesired(true);
    for (const auto& q : questions) {
        request.addQuestion(q);
    }

    std::vector<uint8_t> requestBytes = request.serialize();
    if (sendto(sockfd, requestBytes.data(), requestBytes.size(), 0, res->ai_addr, res->ai_addrlen) < 0) {
        freeaddrinfo(res);
        close(sockfd);
        throw std::runtime_error("Sendto failed");
    }
    freeaddrinfo(res);

    struct pollfd pfd;
    pfd.fd = sockfd;
    pfd.events = POLLIN;
    int pollRes = poll(&pfd, 1, timeoutSec * 1000);

    if (pollRes <= 0) {
        close(sockfd);
        throw std::runtime_error("Query timed out or failed");
    }

    uint8_t buffer[4096];
    ssize_t n = recvfrom(sockfd, buffer, sizeof(buffer), 0, nullptr, nullptr);
    close(sockfd);

    if (n < 0) throw std::runtime_error("Recvfrom failed");

    return DnsDatagram::readFrom(buffer, n);
}
