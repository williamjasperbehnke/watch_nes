#ifndef NESC_INTERNAL_H
#define NESC_INTERNAL_H

#include "apu.hpp"
#include "bus.hpp"
#include "cartridge.hpp"
#include "cpu.hpp"
#include "ppu.hpp"

class NES {
public:
    Bus bus;
    CPU cpu;
    PPU ppu;
    APU apu;
    Cartridge cart;
    bool hasCart;

    NES();
    ~NES();
    bool loadRom(const uint8_t *data, size_t size);
    void reset();
    void stepFrame();
};

#endif
