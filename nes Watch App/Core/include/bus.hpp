#ifndef NESC_BUS_H
#define NESC_BUS_H

#include "apu.hpp"
#include "controller.hpp"
#include "ppu.hpp"
#include <string.h>

class CPU;

class Bus {
public:
    CPU *cpu;
    PPU *ppu;
    APU *apu;
    Cartridge *cartridge;
    Controller controller;

    uint8_t cpuRam[2048];
    uint8_t prgRam[8192];
    uint8_t dataBus;

    bool irqPending;
    int stallCycles;

    bool dmaActive;
    uint8_t dmaPage;
    uint16_t dmaIndex;
    int dmaCycle;
    uint8_t dmaData;

    Bus() {
        memset(this, 0, sizeof(Bus));
    }

    uint8_t cpuRead(uint16_t addr);
    uint8_t cpuReadOpcode(uint16_t addr);
    void cpuWrite(uint16_t addr, uint8_t data);

    bool isIrqPending();
    void ackIrq();
    void tick(int cycles);

    void requestStall(int cycles);
    bool consumeStall();

    void setCpuBus(uint8_t value);

private:
    uint8_t cpuReadInternal(uint16_t addr);
    void startDma(uint8_t page);
    void stepDma();
};

#endif
