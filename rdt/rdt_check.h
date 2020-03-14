#pragma once

#include "rdt_parameters.h"
#include "rdt_struct.h"

#include <string.h>

uint32_t crc32(const char *buf, int size);

void RDT_AddChecksum(packet *pkt);

bool RDT_VerifyChecksum(const packet *pkt);