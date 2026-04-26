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
    src/main.cpp
)

if(HAIKU)
    target_link_libraries(DnsServerCpp network)
endif()
