#ifndef NESC_MAPPER_CNROM_H
#define NESC_MAPPER_CNROM_H

#include "mapper.hpp"

class CnromMapper : public Mapper {
public:
    uint8_t chrBank = 0;

    bool cpuRead(Cartridge &cart, uint16_t addr, uint8_t *out) override;
    bool cpuWrite(Cartridge &cart, uint16_t addr, uint8_t data) override;
    bool ppuRead(Cartridge &cart, uint16_t addr, uint8_t *out) override;
    bool ppuWrite(Cartridge &cart, uint16_t addr, uint8_t data) override;
};

#endif
