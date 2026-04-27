#ifndef DNS_ENUMS_H
#define DNS_ENUMS_H

#include <cstdint>

enum class DnsResourceRecordType : uint16_t {
    Unknown = 0,
    A = 1,
    NS = 2,
    CNAME = 5,
    SOA = 6,
    PTR = 12,
    MX = 15,
    TXT = 16,
    AAAA = 28,
    SRV = 33,
    OPT = 41,
    DS = 43,
    RRSIG = 46,
    NSEC = 47,
    DNSKEY = 48,
    NSEC3 = 50,
    NSEC3PARAM = 51,
    TLSA = 52,
    SVCB = 64,
    HTTPS = 65,
    ANY = 255
};

enum class DnsClass : uint16_t {
    Unknown = 0,
    IN = 1,
    NONE = 254,
    ANY = 255
};

enum class DnsOpcode : uint8_t {
    StandardQuery = 0,
    InverseQuery = 1,
    ServerStatusRequest = 2,
    Notify = 4,
    Update = 5,
    DnsStatefulOperations = 6
};

enum class DnsResponseCode : uint16_t {
    NoError = 0,
    FormatError = 1,
    ServerFailure = 2,
    NxDomain = 3,
    NotImplemented = 4,
    Refused = 5,
    YXDomain = 6,
    YXRRSet = 7,
    NXRRSet = 8,
    NotAuth = 9,
    NotZone = 10,
    BADVERS = 16,
    BADCOOKIE = 23
};

#endif // DNS_ENUMS_H
