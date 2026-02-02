#include "../../include/mapper/nrom.hpp"

#include "../../include/cartridge.hpp"

static int nrom_cpu_map(int prgBanks, uint16_t addr) {
    if (addr < 0x8000) {
        return -1;
    }
    if (prgBanks == 1) {
        return (int)(addr & 0x3FFF);
    }
    return (int)(addr - 0x8000);
}

static int nrom_ppu_map(int chrBanks, uint16_t addr, bool write) {
    if (addr >= 0x2000) {
        return -1;
    }
    if (chrBanks == 0 && write) {
        return (int)addr;
    }
    if (chrBanks > 0) {
        return (int)addr;
    }
    return -1;
}

bool NromMapper::cpuRead(Cartridge &cart, uint16_t addr, uint8_t *out) {
    int mapped = nrom_cpu_map(prgBanks, addr);
    if (mapped < 0) {
        return false;
    }
    *out = cart.prgROM[mapped];
    return true;
}

bool NromMapper::cpuWrite(Cartridge &cart, uint16_t addr, uint8_t data) {
    (void)cart;
    (void)data;
    int mapped = nrom_cpu_map(prgBanks, addr);
    return mapped >= 0;
}

bool NromMapper::ppuRead(Cartridge &cart, uint16_t addr, uint8_t *out) {
    int mapped = nrom_ppu_map(chrBanks, addr, false);
    if (mapped < 0) {
        return false;
    }
    *out = cart.chrROM[mapped];
    return true;
}

bool NromMapper::ppuWrite(Cartridge &cart, uint16_t addr, uint8_t data) {
    int mapped = nrom_ppu_map(chrBanks, addr, true);
    if (mapped < 0) {
        return false;
    }
    if (cart.hasChrRam) {
        cart.chrROM[mapped] = data;
        return true;
    }
    return false;
}
