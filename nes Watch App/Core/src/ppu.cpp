#include "../include/ppu.hpp"

static const uint32_t nes_palette[64] = {
    0xFF7C7C7C, 0xFF0000FC, 0xFF0000BC, 0xFF4428BC, 0xFF940084, 0xFFA80020, 0xFFA81000, 0xFF881400,
    0xFF503000, 0xFF007800, 0xFF006800, 0xFF005800, 0xFF004058, 0xFF000000, 0xFF000000, 0xFF000000,
    0xFFBCBCBC, 0xFF0078F8, 0xFF0058F8, 0xFF6844FC, 0xFFD800CC, 0xFFE40058, 0xFFF83800, 0xFFE45C10,
    0xFFAC7C00, 0xFF00B800, 0xFF00A800, 0xFF00A844, 0xFF008888, 0xFF000000, 0xFF000000, 0xFF000000,
    0xFFF8F8F8, 0xFF3CBCFC, 0xFF6888FC, 0xFF9878F8, 0xFFF878F8, 0xFFF85898, 0xFFF87858, 0xFFFCA044,
    0xFFF8B800, 0xFFB8F818, 0xFF58D854, 0xFF58F898, 0xFF00E8D8, 0xFF787878, 0xFF000000, 0xFF000000,
    0xFFFCFCFC, 0xFFA4E4FC, 0xFFB8B8F8, 0xFFD8B8F8, 0xFFF8B8F8, 0xFFF8A4C0, 0xFFF0D0B0, 0xFFFCE0A8,
    0xFFF8D878, 0xFFD8F878, 0xFFB8F8B8, 0xFFB8F8D8, 0xFF00FCFC, 0xFFF8D8F8, 0xFF000000, 0xFF000000
};

int PPU::mirrorNametable(uint16_t addr) {
    int offset = (int)(addr & 0x0FFF);
    Mirroring activeMirroring = cartridge ? cartridge->mirroring : mirroring;
    if (activeMirroring == MIRROR_VERTICAL) {
        return offset & 0x07FF;
    }
    int table = (offset / 0x400) & 0x03;
    int index = offset & 0x03FF;
    if (table == 0 || table == 1) {
        return index;
    }
    return 0x400 + index;
}

int PPU::mirrorPalette(uint16_t addr) {
    int index = (int)(addr & 0x001F);
    if (index == 0x10) index = 0x00;
    if (index == 0x14) index = 0x04;
    if (index == 0x18) index = 0x08;
    if (index == 0x1C) index = 0x0C;
    return index;
}

uint32_t PPU::paletteColor(int palette, int color) {
    uint16_t indexAddr = (color == 0) ? 0x3F00 : (uint16_t)(0x3F00 + palette * 4 + color);
    uint8_t paletteIndex = (uint8_t)(readMemory(indexAddr) & 0x3F);
    return nes_palette[paletteIndex % 64];
}

uint32_t PPU::spritePaletteColor(int palette, int color) {
    uint16_t indexAddr = (uint16_t)(0x3F10 + palette * 4 + color);
    uint8_t paletteIndex = (uint8_t)(readMemory(indexAddr) & 0x3F);
    return nes_palette[paletteIndex % 64];
}

void PPU::renderBackgroundScanline(int y) {
    int width = NES_WIDTH;
    bool showBackground = (mask & 0x08) != 0;
    bool showLeftBackground = (mask & 0x02) != 0;
    uint16_t patternBase = (ctrl & 0x10) != 0 ? 0x1000 : 0x0000;
    int baseNTX = (ctrl & 0x01) != 0 ? 1 : 0;
    int baseNTY = (ctrl & 0x02) != 0 ? 1 : 0;

    int scrolledY = (y + (int)scrollY) & 0x1FF;
    int tileY = (scrolledY / 8) % 30;
    int fineY = scrolledY % 8;
    int ntY = ((scrolledY / 240) + baseNTY) & 0x01;

    for (int x = 0; x < width; x++) {
        if (!showBackground || (x < 8 && !showLeftBackground)) {
            uint32_t pixelColor = paletteColor(0, 0);
            frameBuffer.pixels[y * width + x] = pixelColor;
            bgColorIndex[y * width + x] = 0;
            continue;
        }

        int scrolledX = (x + (int)scrollX) & 0x1FF;
        int tileX = (scrolledX / 8) % 32;
        int fineX = scrolledX % 8;
        int ntX = ((scrolledX / 256) + baseNTX) & 0x01;
        uint16_t baseNameTable = (uint16_t)(0x2000 + ((ntY << 1) | ntX) * 0x400);
        uint16_t nameAddr = (uint16_t)(baseNameTable + tileY * 32 + tileX);
        uint8_t tileId = readMemory(nameAddr);
        uint16_t patternAddr = (uint16_t)(patternBase + (uint16_t)tileId * 16 + (uint16_t)fineY);
        uint8_t plane0 = readMemory(patternAddr);
        uint8_t plane1 = readMemory(patternAddr + 8);
        int bit = 7 - fineX;
        int color = ((plane1 >> bit) & 0x01) << 1 | ((plane0 >> bit) & 0x01);

        uint16_t attrAddr = (uint16_t)(baseNameTable + 0x03C0 + (tileY / 4) * 8 + (tileX / 4));
        uint8_t attr = readMemory(attrAddr);
        int quadrantX = (tileX % 4) / 2;
        int quadrantY = (tileY % 4) / 2;
        int shift = (quadrantY * 2 + quadrantX) * 2;
        int palette = (attr >> shift) & 0x03;

        uint32_t pixelColor = paletteColor(palette, color);
        frameBuffer.pixels[y * width + x] = pixelColor;
        bgColorIndex[y * width + x] = (uint8_t)color;
    }
}

void PPU::renderSpritesScanline(int y) {
    bool showSprites = (mask & 0x10) != 0;
    bool showLeftSprites = (mask & 0x04) != 0;
    if (!showSprites) {
        return;
    }

    int width = NES_WIDTH;
    int spriteHeight = (ctrl & 0x20) != 0 ? 16 : 8;
    uint16_t spriteTable = (ctrl & 0x08) != 0 ? 0x1000 : 0x0000;

    for (int i = 63; i >= 0; i--) {
        int base = i * 4;
        int spriteY = (int)oam[base] + 1;
        uint8_t tileId = oam[base + 1];
        uint8_t attr = oam[base + 2];
        int spriteX = (int)oam[base + 3];

        if (y < spriteY || y >= spriteY + spriteHeight) {
            continue;
        }

        int palette = attr & 0x03;
        bool priorityBehind = (attr & 0x20) != 0;
        bool flipH = (attr & 0x40) != 0;
        bool flipV = (attr & 0x80) != 0;

        int row = y - spriteY;
        int spriteRow = flipV ? (spriteHeight - 1 - row) : row;
        uint16_t tileIndex;
        uint16_t patternBase;
        int fineY;

        if (spriteHeight == 16) {
            uint16_t table = (tileId & 0x01) != 0 ? 0x1000 : 0x0000;
            patternBase = table;
            uint16_t baseTile = (uint16_t)(tileId & 0xFE);
            tileIndex = (uint16_t)(baseTile + (spriteRow / 8));
            fineY = spriteRow % 8;
        } else {
            patternBase = spriteTable;
            tileIndex = tileId;
            fineY = spriteRow;
        }

        uint16_t patternAddr = (uint16_t)(patternBase + tileIndex * 16 + fineY);
        uint8_t plane0 = readMemory(patternAddr);
        uint8_t plane1 = readMemory(patternAddr + 8);

        for (int col = 0; col < 8; col++) {
            int x = spriteX + col;
            if (x < 0 || x >= width) {
                continue;
            }
            if (x < 8 && !showLeftSprites) {
                continue;
            }

            int fineX = flipH ? col : (7 - col);
            int bit = fineX;
            int color = ((plane1 >> bit) & 0x01) << 1 | ((plane0 >> bit) & 0x01);
            if (color == 0) {
                continue;
            }

            uint8_t bgIndex = bgColorIndex[y * width + x];
            if (priorityBehind && bgIndex != 0) {
                continue;
            }

            if (i == 0 && (mask & 0x08) != 0) {
                status |= 0x40;
            }

            uint32_t pixelColor = spritePaletteColor(palette, color);
            frameBuffer.pixels[y * width + x] = pixelColor;
        }
    }
}

void PPU::connectCartridge(Cartridge *cart) {
    cartridge = cart;
    mirroring = cart->mirroring;
}

void PPU::resetFrame() {
    frameComplete = false;
}

uint8_t PPU::cpuRead(uint16_t addr) {
    switch (addr) {
        case 0x2002: {
            uint8_t value = (uint8_t)((status & 0xE0) | (dataBus & 0x1F));
            status &= 0x1F;
            addressLatch = false;
            dataBus = value;
            return value;
        }
        case 0x2004: {
            uint8_t value = oam[oamAddr];
            dataBus = value;
            return value;
        }
        case 0x2007: {
            uint8_t value;
            if (vramAddr >= 0x3F00) {
                value = readMemory(vramAddr);
                readBuffer = readMemory((uint16_t)(vramAddr - 0x1000));
            } else {
                value = readBuffer;
                readBuffer = readMemory(vramAddr);
            }
            vramAddr += (ctrl & 0x04) != 0 ? 32 : 1;
            dataBus = value;
            return value;
        }
        default:
            return dataBus;
    }
}

void PPU::cpuWrite(uint16_t addr, uint8_t data) {
    dataBus = data;
    switch (addr) {
        case 0x2000:
            ctrl = data;
            break;
        case 0x2001:
            mask = data;
            break;
        case 0x2003:
            oamAddr = data;
            break;
        case 0x2004:
            oam[oamAddr] = data;
            oamAddr += 1;
            break;
        case 0x2005:
            if (!addressLatch) {
                scrollX = data;
                addressLatch = true;
            } else {
                scrollY = data;
                addressLatch = false;
            }
            break;
        case 0x2006:
            if (!addressLatch) {
                vramAddr = (uint16_t)(data << 8);
                addressLatch = true;
            } else {
                vramAddr = (uint16_t)((vramAddr & 0xFF00) | data);
                addressLatch = false;
            }
            break;
        case 0x2007:
            writeMemory(vramAddr, data);
            vramAddr += (ctrl & 0x04) != 0 ? 32 : 1;
            break;
        default:
            break;
    }
}

void PPU::tick() {
    nmiRequested = false;
    if (scanline == 241 && cycle == 1) {
        status |= 0x80;
        if ((ctrl & 0x80) != 0) {
            nmiRequested = true;
        }
    }

    if (scanline == 261 && cycle == 1) {
        status &= 0x1F;
    }

    if (cycle == 0 && scanline >= 0 && scanline < 240) {
        renderBackgroundScanline(scanline);
        renderSpritesScanline(scanline);
    }

    cycle += 1;
    if (cycle >= 341) {
        cycle = 0;
        scanline += 1;
        if (scanline >= 262) {
            scanline = 0;
            frameComplete = true;
        }
    }
}

uint8_t PPU::readMemory(uint16_t addr) {
    uint16_t address = addr & 0x3FFF;
    if (address < 0x2000) {
        uint8_t value = 0;
        if (cartridge && cartridge->ppuRead(address, &value)) {
            return value;
        }
        return 0;
    }
    if (address < 0x3F00) {
        int index = mirrorNametable(address);
        return nametableRam[index];
    }
    int paletteIndex = mirrorPalette(address);
    return paletteRam[paletteIndex];
}

void PPU::writeMemory(uint16_t addr, uint8_t data) {
    uint16_t address = addr & 0x3FFF;
    if (address < 0x2000) {
        if (cartridge) {
            (void)cartridge->ppuWrite(address, data);
        }
        return;
    }
    if (address < 0x3F00) {
        int index = mirrorNametable(address);
        nametableRam[index] = data;
        return;
    }
    int paletteIndex = mirrorPalette(address);
    paletteRam[paletteIndex] = data;
}

void PPU::dmaWriteOam(uint8_t data) {
    oam[oamAddr] = data;
    oamAddr += 1;
}
