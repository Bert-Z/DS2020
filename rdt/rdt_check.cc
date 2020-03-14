#include "rdt_check.h"

#include <stdio.h>
#include <stdlib.h>

uint32_t crc32(const char *buf, int size)
{
    uint32_t result = 0xffffffff;

    while (size--)
        result = crc32_table[(result ^ *buf++) & 0xff] ^ (result >> 8);

    return result;
}

void RDT_AddChecksum(packet *pkt)
{
    ASSERT(pkt);

    uint32_t checksum = crc32(pkt->data + 4, RDT_PKTSIZE - CHECKSUM_SIZE);
    memcpy(pkt->data, (char *)&checksum, CHECKSUM_SIZE);

    return;
}

bool RDT_VerifyChecksum(const packet *pkt)
{
    ASSERT(pkt);

    uint32_t checksum = crc32(pkt->data + 4, RDT_PKTSIZE - CHECKSUM_SIZE);
    uint32_t header_checksum = *(uint32_t *)pkt->data;

    return checksum == header_checksum;
}