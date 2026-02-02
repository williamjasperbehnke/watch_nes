#ifndef NESC_PPU_H
#define NESC_PPU_H

#include "cartridge.hpp"
#include <string.h>

class PPU {
public:
    FrameBuffer frameBuffer;
    Cartridge *cartridge;
    Mirroring mirroring;
    uint8_t dataBus;
    uint8_t ctrl;
    uint8_t mask;
    uint8_t status;
    uint8_t oamAddr;
    uint8_t oam[256];
    uint8_t bgColorIndex[NES_WIDTH * NES_HEIGHT];
    uint8_t scrollX;
    uint8_t scrollY;
    bool addressLatch;
    uint16_t vramAddr;
    uint8_t readBuffer;
    int cycle;
    int scanline;
    bool frameComplete;
    bool nmiRequested;
    uint8_t nametableRam[2048];
    uint8_t paletteRam[32];

    PPU() {
        memset(this, 0, sizeof(PPU));
    }

    void connectCartridge(Cartridge *cart);
    void resetFrame();
    uint8_t cpuRead(uint16_t addr);
    void cpuWrite(uint16_t addr, uint8_t data);
    void tick();
    void dmaWriteOam(uint8_t data);

private:
    uint8_t readMemory(uint16_t addr);
    void writeMemory(uint16_t addr, uint8_t data);
    int mirrorNametable(uint16_t addr);
    int mirrorPalette(uint16_t addr);
    uint32_t paletteColor(int palette, int color);
    uint32_t spritePaletteColor(int palette, int color);
    void renderBackgroundScanline(int y);
    void renderSpritesScanline(int y);
};

#endif
