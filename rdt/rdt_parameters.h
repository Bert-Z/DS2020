/**
 * 
 * 
 * 
**/

#pragma once

#include <stdint.h>
#include <string.h>

const int PKT_MAX_PAYLOAD_SIZE = 118;
const int CHECKSUM_SIZE = 4;
const int WINDOW_SIZE = 10;

const double TIME_OUT = 0.3;
const double TIME_INTERVAL = 0.1;
const uint32_t PKT_MAX_SEQ_NUM = ((1 << 20) - 1);

class AckInfo
{
public:
    uint32_t checksum; // 4 bytes
    uint32_t ackseq;   // 4 bytes
    // last 120 bytes
    AckInfo(uint32_t seq) : checksum(0), ackseq(seq) {}
};

class PacketInfo
{
public:
    uint32_t checksum;                  // 4 bytes
    uint32_t seqnum;                    // 4 bytes
    char payload_size;                  // 1 byte
    bool isend;                         // 1 byte
    char payload[PKT_MAX_PAYLOAD_SIZE]; // 118 bytes
    PacketInfo(){}
    PacketInfo(uint32_t seq, char psize, bool end, const char *pay) : checksum(0), seqnum(seq), payload_size(psize), isend(end)
    {
        memcpy(payload, pay, psize);
    }
};
