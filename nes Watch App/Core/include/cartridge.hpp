#ifndef NESC_CARTRIDGE_H
#define NESC_CARTRIDGE_H

#include "mapper/mapper.hpp"
#include <memory>

class Cartridge {
public:
    uint8_t *prgROM;
    size_t prgSize;
    uint8_t *chrROM;
    size_t chrSize;
    uint8_t mapperID;
    Mirroring mirroring;
    bool hasChrRam;
    std::unique_ptr<Mapper> mapper;

    Cartridge()
        : prgROM(nullptr),
          prgSize(0),
          chrROM(nullptr),
          chrSize(0),
          mapperID(0),
          mirroring(MIRROR_HORIZONTAL),
          hasChrRam(false),
          mapper(nullptr) {}

    ~Cartridge() { free(); }

    void free();
    bool load(const uint8_t *data, size_t size);
    bool cpuRead(uint16_t addr, uint8_t *out) const;
    bool cpuWrite(uint16_t addr, uint8_t data);
    bool ppuRead(uint16_t addr, uint8_t *out) const;
    bool ppuWrite(uint16_t addr, uint8_t data);
};

#endif
