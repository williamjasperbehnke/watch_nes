#include "../include/cartridge.hpp"

#include <stdlib.h>
#include <string.h>

#include "../include/mapper/cnrom.hpp"
#include "../include/mapper/mmc1.hpp"
#include "../include/mapper/nrom.hpp"

void Cartridge::free() {
    ::free(prgROM);
    ::free(chrROM);
    prgROM = nullptr;
    chrROM = nullptr;
    prgSize = 0;
    chrSize = 0;
    mapperID = 0;
    mirroring = MIRROR_HORIZONTAL;
    hasChrRam = false;
    mapper.reset();
}

bool Cartridge::load(const uint8_t *data, size_t size) {
    if (!data || size < 16) {
        return false;
    }
    if (data[0] != 0x4E || data[1] != 0x45 || data[2] != 0x53 || data[3] != 0x1A) {
        return false;
    }

    uint8_t prgBanks = data[4];
    uint8_t chrBanks = data[5];
    uint8_t flags6 = data[6];
    uint8_t flags7 = data[7];

    mapperID = (uint8_t)((flags7 & 0xF0) | (flags6 >> 4));
    mirroring = (flags6 & 0x01) == 0 ? MIRROR_HORIZONTAL : MIRROR_VERTICAL;

    size_t offset = 16;
    if (flags6 & 0x04) {
        offset += 512;
    }

    size_t prgSizeLocal = (size_t)prgBanks * 16 * 1024;
    size_t chrSizeLocal = (size_t)chrBanks * 8 * 1024;
    size_t prgStart = offset;
    size_t chrStart = prgStart + prgSizeLocal;

    if (size < chrStart + chrSizeLocal) {
        return false;
    }

    prgROM = (uint8_t *)malloc(prgSizeLocal);
    if (!prgROM) {
        return false;
    }
    memcpy(prgROM, data + prgStart, prgSizeLocal);
    prgSize = prgSizeLocal;

    if (chrSizeLocal > 0) {
        chrROM = (uint8_t *)malloc(chrSizeLocal);
        if (!chrROM) {
            return false;
        }
        memcpy(chrROM, data + chrStart, chrSizeLocal);
        chrSize = chrSizeLocal;
        hasChrRam = false;
    } else {
        chrROM = (uint8_t *)calloc(8 * 1024, 1);
        if (!chrROM) {
            return false;
        }
        chrSize = 8 * 1024;
        hasChrRam = true;
    }

    mapper.reset();
    if (mapperID == 0) {
        mapper = std::make_unique<NromMapper>(prgBanks, chrBanks);
    } else if (mapperID == 1) {
        mapper = std::make_unique<Mmc1Mapper>();
    } else if (mapperID == 3) {
        mapper = std::make_unique<CnromMapper>();
    } else {
        return false;
    }
    return true;
}

bool Cartridge::cpuRead(uint16_t addr, uint8_t *out) const {
    if (!mapper) {
        return false;
    }
    return mapper->cpuRead(const_cast<Cartridge &>(*this), addr, out);
}

bool Cartridge::cpuWrite(uint16_t addr, uint8_t data) {
    if (!mapper) {
        return false;
    }
    return mapper->cpuWrite(*this, addr, data);
}

bool Cartridge::ppuRead(uint16_t addr, uint8_t *out) const {
    if (!mapper) {
        return false;
    }
    return mapper->ppuRead(const_cast<Cartridge &>(*this), addr, out);
}

bool Cartridge::ppuWrite(uint16_t addr, uint8_t data) {
    if (!mapper) {
        return false;
    }
    return mapper->ppuWrite(*this, addr, data);
}
