#include "../../include/mapper/mmc1.h"

void mmc1_init(MapperMMC1 *mapper) {
    mmc1_reset(mapper);
}

void mmc1_reset(MapperMMC1 *mapper) {
    mapper->shiftReg = 0x10;
    mapper->shiftCount = 0;
    mapper->control = 0x0C; // PRG mode 3, CHR mode 0
    mapper->chrBank0 = 0;
    mapper->chrBank1 = 0;
    mapper->prgBank = 0;
}
