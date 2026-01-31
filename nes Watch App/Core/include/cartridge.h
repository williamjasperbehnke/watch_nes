#ifndef NESC_CARTRIDGE_H
#define NESC_CARTRIDGE_H

#include "mmc1.h"
#include "nrom.h"

typedef enum {
    MAPPER_NROM = 0,
    MAPPER_MMC1 = 1
} MapperType;

typedef struct {
    uint8_t *prgROM;
    size_t prgSize;
    uint8_t *chrROM;
    size_t chrSize;
    uint8_t mapperID;
    Mirroring mirroring;
    bool hasChrRam;
    MapperNROM mapperNrom;
    MapperMMC1 mapperMmc1;
    MapperType mapperType;
} Cartridge;

void cartridge_free(Cartridge *cart);
bool cartridge_load(Cartridge *cart, const uint8_t *data, size_t size);

bool cartridge_cpu_read(const Cartridge *cart, uint16_t addr, uint8_t *out);
bool cartridge_cpu_write(const Cartridge *cart, uint16_t addr);

bool cartridge_ppu_read(const Cartridge *cart, uint16_t addr, uint8_t *out);
bool cartridge_ppu_write(Cartridge *cart, uint16_t addr, uint8_t data);

#endif
