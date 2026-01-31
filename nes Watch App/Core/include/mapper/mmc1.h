#ifndef NESC_MAPPER_MMC1_H
#define NESC_MAPPER_MMC1_H

#include "types.h"

typedef struct {
    uint8_t shiftReg;
    uint8_t shiftCount;
    uint8_t control;
    uint8_t chrBank0;
    uint8_t chrBank1;
    uint8_t prgBank;
} MapperMMC1;

void mmc1_init(MapperMMC1 *mapper);
void mmc1_reset(MapperMMC1 *mapper);

#endif
