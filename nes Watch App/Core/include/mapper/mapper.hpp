#ifndef NESC_MAPPER_BASE_H
#define NESC_MAPPER_BASE_H

#include "types.hpp"

class Cartridge;

class Mapper {
public:
    virtual ~Mapper() = default;
    virtual bool cpuRead(Cartridge &cart, uint16_t addr, uint8_t *out) = 0;
    virtual bool cpuWrite(Cartridge &cart, uint16_t addr, uint8_t data) = 0;
    virtual bool ppuRead(Cartridge &cart, uint16_t addr, uint8_t *out) = 0;
    virtual bool ppuWrite(Cartridge &cart, uint16_t addr, uint8_t data) = 0;
};

#endif
