#pragma once

#include <stdint.h>
#include "rdt_struct.h"

#include <string.h>

const uint32_t CHECKSUM_SIZE = 4;

uint32_t crc32(const char *buf, int size);

void RDT_AddChecksum(packet *pkt);

bool RDT_VerifyChecksum(const packet *pkt);