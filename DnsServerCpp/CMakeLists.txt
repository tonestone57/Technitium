cmake_minimum_required(VERSION 3.10)
project(DnsServerCpp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_executable(DnsServerCpp
    src/DnsDatagram.cpp
    src/DnsQuestionRecord.cpp
    src/DnsResourceRecord.cpp
    src/DnsResourceRecordData.cpp
    src/ZoneLoader.cpp
    src/ZoneManager.cpp
    src/DnsClient.cpp
    src/main.cpp
)

add_executable(test_client
    src/DnsDatagram.cpp
    src/DnsQuestionRecord.cpp
    src/DnsResourceRecord.cpp
    src/DnsResourceRecordData.cpp
    src/DnsClient.cpp
    src/test_client.cpp
)

if(HAIKU)
    target_link_libraries(DnsServerCpp network)
    target_link_libraries(test_client network)
endif()
