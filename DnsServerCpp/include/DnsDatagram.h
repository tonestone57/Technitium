#ifndef DNS_DATAGRAM_H
#define DNS_DATAGRAM_H

#include "DnsEnums.h"
#include "DnsQuestionRecord.h"
#include "DnsResourceRecord.h"
#include <vector>
#include <cstdint>
#include <iostream>
#include <map>

class DnsDatagram {
public:
    DnsDatagram();

    static DnsDatagram readFrom(const uint8_t* buffer, size_t size);
    std::vector<uint8_t> serialize() const;

    // Getters and Setters
    uint16_t getIdentifier() const { return identifier; }
    void setIdentifier(uint16_t id) { identifier = id; }

    bool isResponse() const { return qr; }
    void setResponse(bool res) { qr = res; }

    DnsOpcode getOpcode() const { return opcode; }
    void setOpcode(DnsOpcode op) { opcode = op; }

    bool isAuthoritativeAnswer() const { return aa; }
    void setAuthoritativeAnswer(bool auth) { aa = auth; }

    bool isTruncation() const { return tc; }
    void setTruncation(bool trunc) { tc = trunc; }

    bool isRecursionDesired() const { return rd; }
    void setRecursionDesired(bool rec) { rd = rec; }

    bool isRecursionAvailable() const { return ra; }
    void setRecursionAvailable(bool rec) { ra = rec; }

    DnsResponseCode getRcode() const { return rcode; }
    void setRcode(DnsResponseCode rc) { rcode = rc; }

    const std::vector<DnsQuestionRecord>& getQuestions() const { return questions; }
    void addQuestion(const DnsQuestionRecord& q) { questions.push_back(q); }

    const std::vector<DnsResourceRecord>& getAnswers() const { return answers; }
    void addAnswer(const DnsResourceRecord& a) { answers.push_back(a); }

    const std::vector<DnsResourceRecord>& getAuthorities() const { return authorities; }
    void addAuthority(const DnsResourceRecord& a) { authorities.push_back(a); }

    const std::vector<DnsResourceRecord>& getAdditionals() const { return additionals; }
    void addAdditional(const DnsResourceRecord& a) { additionals.push_back(a); }

    static void writeDomainName(std::vector<uint8_t>& buffer, const std::string& domain, std::map<std::string, uint16_t>& domainOffsets);
    static std::string readDomainName(const uint8_t* buffer, size_t size, size_t& offset);

    bool isParsedSuccessfully() const { return parsedSuccessfully; }

private:
    uint16_t identifier = 0;
    bool qr = false;
    DnsOpcode opcode = DnsOpcode::StandardQuery;
    bool aa = false;
    bool tc = false;
    bool rd = false;
    bool ra = false;
    DnsResponseCode rcode = DnsResponseCode::NoError;

    std::vector<DnsQuestionRecord> questions;
    std::vector<DnsResourceRecord> answers;
    std::vector<DnsResourceRecord> authorities;
    std::vector<DnsResourceRecord> additionals;

    bool parsedSuccessfully;
};

#endif // DNS_DATAGRAM_H
