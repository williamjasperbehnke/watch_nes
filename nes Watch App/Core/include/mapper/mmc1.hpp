#ifndef NESC_MAPPER_MMC1_H
#define NESC_MAPPER_MMC1_H

#include "mapper.hpp"

class Mmc1Mapper : public Mapper {
public:
    uint8_t shiftReg = 0x10;
    uint8_t shiftCount = 0;
    uint8_t control = 0x0C;
    uint8_t chrBank0 = 0;
    uint8_t chrBank1 = 0;
    uint8_t prgBank = 0;

    bool cpuRead(Cartridge &cart, uint16_t addr, uint8_t *out) override;
    bool cpuWrite(Cartridge &cart, uint16_t addr, uint8_t data) override;
    bool ppuRead(Cartridge &cart, uint16_t addr, uint8_t *out) override;
    bool ppuWrite(Cartridge &cart, uint16_t addr, uint8_t data) override;

private:
    void applyControl(Cartridge &cart, uint8_t value);
};

#endif
