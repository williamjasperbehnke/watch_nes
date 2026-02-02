#ifndef NESC_MAPPER_NROM_H
#define NESC_MAPPER_NROM_H

#include "mapper.hpp"

class NromMapper : public Mapper {
public:
    int prgBanks;
    int chrBanks;

    explicit NromMapper(int prg, int chr) : prgBanks(prg), chrBanks(chr) {}

    bool cpuRead(Cartridge &cart, uint16_t addr, uint8_t *out) override;
    bool cpuWrite(Cartridge &cart, uint16_t addr, uint8_t data) override;
    bool ppuRead(Cartridge &cart, uint16_t addr, uint8_t *out) override;
    bool ppuWrite(Cartridge &cart, uint16_t addr, uint8_t data) override;
};

#endif
