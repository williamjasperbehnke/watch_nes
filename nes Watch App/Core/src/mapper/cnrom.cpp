#include "../../include/mapper/cnrom.hpp"

#include "../../include/cartridge.hpp"

bool CnromMapper::cpuRead(Cartridge &cart, uint16_t addr, uint8_t *out) {
    if (addr < 0x8000) {
        return false;
    }
    uint32_t mapped = 0;
    if (cart.prgSize == 16 * 1024) {
        mapped = (uint32_t)(addr & 0x3FFF);
    } else {
        mapped = (uint32_t)(addr - 0x8000);
    }
    if (mapped >= cart.prgSize) {
        return false;
    }
    *out = cart.prgROM[mapped];
    return true;
}

bool CnromMapper::cpuWrite(Cartridge &cart, uint16_t addr, uint8_t data) {
    if (addr < 0x8000) {
        return false;
    }
    int chrBankCount = (int)(cart.chrSize / (8 * 1024));
    uint8_t bank = data;
    if (chrBankCount > 0) {
        if ((chrBankCount & (chrBankCount - 1)) == 0) {
            bank = (uint8_t)(bank & (uint8_t)(chrBankCount - 1));
        } else {
            bank = (uint8_t)(bank % chrBankCount);
        }
    } else {
        bank = 0;
    }
    chrBank = bank;
    return true;
}

bool CnromMapper::ppuRead(Cartridge &cart, uint16_t addr, uint8_t *out) {
    if (addr >= 0x2000) {
        return false;
    }
    int chrBankCount = (int)(cart.chrSize / (8 * 1024));
    if (chrBankCount <= 0) {
        return false;
    }
    uint8_t bank = chrBank;
    if ((chrBankCount & (chrBankCount - 1)) == 0) {
        bank = (uint8_t)(bank & (uint8_t)(chrBankCount - 1));
    } else {
        bank = (uint8_t)(bank % chrBankCount);
    }
    uint32_t mapped = (uint32_t)bank * 8 * 1024 + addr;
    if (mapped >= cart.chrSize) {
        return false;
    }
    *out = cart.chrROM[mapped];
    return true;
}

bool CnromMapper::ppuWrite(Cartridge &cart, uint16_t addr, uint8_t data) {
    if (!cart.hasChrRam) {
        return false;
    }
    if (addr >= 0x2000) {
        return false;
    }
    int chrBankCount = (int)(cart.chrSize / (8 * 1024));
    if (chrBankCount <= 0) {
        return false;
    }
    uint8_t bank = chrBank;
    if ((chrBankCount & (chrBankCount - 1)) == 0) {
        bank = (uint8_t)(bank & (uint8_t)(chrBankCount - 1));
    } else {
        bank = (uint8_t)(bank % chrBankCount);
    }
    uint32_t mapped = (uint32_t)bank * 8 * 1024 + addr;
    if (mapped >= cart.chrSize) {
        return false;
    }
    cart.chrROM[mapped] = data;
    return true;
}
