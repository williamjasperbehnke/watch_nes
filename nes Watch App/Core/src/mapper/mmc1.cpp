#include "../../include/mapper/mmc1.hpp"

#include "../../include/cartridge.hpp"

void Mmc1Mapper::applyControl(Cartridge &cart, uint8_t value) {
    control = value;
    uint8_t mirror = value & 0x03;
    if (mirror == 3) {
        cart.mirroring = MIRROR_HORIZONTAL;
    } else {
        cart.mirroring = MIRROR_VERTICAL;
    }
}

bool Mmc1Mapper::cpuRead(Cartridge &cart, uint16_t addr, uint8_t *out) {
    if (addr < 0x8000) {
        return false;
    }
    uint8_t prgMode = (control >> 2) & 0x03;
    int prgBankCount = (int)(cart.prgSize / (16 * 1024));
    int bank = prgBank & 0x0F;
    uint32_t mapped = 0;

    switch (prgMode) {
        case 0:
        case 1: {
            int bank32 = (bank & 0x0E);
            if (addr < 0xC000) {
                mapped = (uint32_t)bank32 * 16 * 1024 + (addr - 0x8000);
            } else {
                mapped = (uint32_t)(bank32 + 1) * 16 * 1024 + (addr - 0xC000);
            }
            break;
        }
        case 2: {
            if (addr < 0xC000) {
                mapped = (uint32_t)(0 * 16 * 1024 + (addr - 0x8000));
            } else {
                int b = bank % prgBankCount;
                mapped = (uint32_t)b * 16 * 1024 + (addr - 0xC000);
            }
            break;
        }
        case 3:
        default: {
            if (addr < 0xC000) {
                int b = bank % prgBankCount;
                mapped = (uint32_t)b * 16 * 1024 + (addr - 0x8000);
            } else {
                mapped = (uint32_t)(prgBankCount - 1) * 16 * 1024 + (addr - 0xC000);
            }
            break;
        }
    }

    if (mapped >= cart.prgSize) {
        return false;
    }
    *out = cart.prgROM[mapped];
    return true;
}

bool Mmc1Mapper::cpuWrite(Cartridge &cart, uint16_t addr, uint8_t data) {
    if (addr < 0x8000) {
        return false;
    }
    if (data & 0x80) {
        shiftReg = 0x10;
        shiftCount = 0;
        control |= 0x0C;
        return true;
    }

    shiftReg = (uint8_t)((shiftReg >> 1) | ((data & 0x01) << 4));
    shiftCount += 1;
    if (shiftCount == 5) {
        uint8_t value = shiftReg;
        uint16_t region = (addr >> 13) & 0x03;
        if (region == 0) {
            applyControl(cart, value);
        } else if (region == 1) {
            chrBank0 = value;
        } else if (region == 2) {
            chrBank1 = value;
        } else {
            prgBank = value;
        }
        shiftReg = 0x10;
        shiftCount = 0;
    }
    return true;
}

bool Mmc1Mapper::ppuRead(Cartridge &cart, uint16_t addr, uint8_t *out) {
    if (addr >= 0x2000) {
        return false;
    }
    uint8_t chrMode = (control >> 4) & 0x01;
    uint32_t mapped = 0;
    if (chrMode == 0) {
        uint32_t bank = (uint32_t)(chrBank0 & 0x1E);
        mapped = bank * 4 * 1024 + addr;
    } else {
        if (addr < 0x1000) {
            mapped = (uint32_t)chrBank0 * 4 * 1024 + addr;
        } else {
            mapped = (uint32_t)chrBank1 * 4 * 1024 + (addr - 0x1000);
        }
    }
    if (mapped >= cart.chrSize) {
        return false;
    }
    *out = cart.chrROM[mapped];
    return true;
}

bool Mmc1Mapper::ppuWrite(Cartridge &cart, uint16_t addr, uint8_t data) {
    if (!cart.hasChrRam) {
        return false;
    }
    if (addr >= 0x2000) {
        return false;
    }
    uint8_t chrMode = (control >> 4) & 0x01;
    uint32_t mapped = 0;
    if (chrMode == 0) {
        uint32_t bank = (uint32_t)(chrBank0 & 0x1E);
        mapped = bank * 4 * 1024 + addr;
    } else {
        if (addr < 0x1000) {
            mapped = (uint32_t)chrBank0 * 4 * 1024 + addr;
        } else {
            mapped = (uint32_t)chrBank1 * 4 * 1024 + (addr - 0x1000);
        }
    }
    if (mapped >= cart.chrSize) {
        return false;
    }
    cart.chrROM[mapped] = data;
    return true;
}
