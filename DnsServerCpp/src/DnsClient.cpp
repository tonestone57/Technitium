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

DnsDatagram DnsClient::query(const std::string& serverIp, int port, const DnsQuestionRecord& question, int timeoutSec) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) throw std::runtime_error("Socket creation failed");

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(static_cast<uint16_t>(port));

    // Support parsing IP:PORT in serverIp
    std::string ip = serverIp;
    int targetPort = port;
    size_t colon = serverIp.find(':');
    if (colon != std::string::npos) {
        ip = serverIp.substr(0, colon);
        targetPort = std::stoi(serverIp.substr(colon + 1));
        servaddr.sin_port = htons(static_cast<uint16_t>(targetPort));
    }

    if (inet_pton(AF_INET, ip.c_str(), &servaddr.sin_addr) <= 0) {
        close(sockfd);
        throw std::runtime_error("Invalid server IP address");
    }

    DnsDatagram request;
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 65535);
    request.setIdentifier(static_cast<uint16_t>(dis(gen)));
    request.setRecursionDesired(true);
    request.addQuestion(question);

    std::vector<uint8_t> requestBytes = request.serialize();
    if (sendto(sockfd, requestBytes.data(), requestBytes.size(), 0, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        close(sockfd);
        throw std::runtime_error("Sendto failed");
    }

    struct pollfd pfd;
    pfd.fd = sockfd;
    pfd.events = POLLIN;
    int res = poll(&pfd, 1, timeoutSec * 1000);

    if (res <= 0) {
        close(sockfd);
        throw std::runtime_error("Query timed out or failed");
    }

    uint8_t buffer[4096];
    ssize_t n = recvfrom(sockfd, buffer, sizeof(buffer), 0, nullptr, nullptr);
    close(sockfd);

    if (n < 0) throw std::runtime_error("Recvfrom failed");

    return DnsDatagram::readFrom(buffer, n);
}
