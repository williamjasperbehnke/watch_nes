#include "../include/nesc.hpp"

#include <string.h>

#include "../include/nes_internal.hpp"

static uint8_t nes_bus_read(void *context, uint16_t addr) {
    return ((Bus *)context)->cpuRead(addr);
}

NES::NES() : hasCart(false) {
    apu.init();
    bus.cpu = &cpu;
    bus.ppu = &ppu;
    bus.apu = &apu;
    cpu.init();
    cpu.bus = &bus;
    apu.setReadCallback(nes_bus_read, &bus);
}

NES::~NES() {
    cart.free();
}

bool NES::loadRom(const uint8_t *data, size_t size) {
    cart.free();
    if (!cart.load(data, size)) {
        cart.free();
        return false;
    }
    bus.cartridge = &cart;
    ppu.connectCartridge(&cart);
    hasCart = true;
    reset();
    return true;
}

void NES::reset() {
    apu.reset();
    cpu.reset();
}

void NES::stepFrame() {
    if (!hasCart) {
        return;
    }
    ppu.resetFrame();
    while (!ppu.frameComplete) {
        int cycles = cpu.step();
        if (cycles > 0) {
            for (int i = 0; i < cycles * 3; i++) {
                ppu.tick();
                if (ppu.nmiRequested) {
                    cpu.nmi();
                }
            }
        }
    }
}

NESRef nes_create(void) {
    return new NES();
}

void nes_destroy(NESRef nes) {
    delete nes;
}

bool nes_load_rom(NESRef nes, const uint8_t *data, size_t size) {
    if (!nes) {
        return false;
    }
    return nes->loadRom(data, size);
}

void nes_reset(NESRef nes) {
    if (!nes) {
        return;
    }
    nes->reset();
}

void nes_step_frame(NESRef nes) {
    if (!nes) {
        return;
    }
    nes->stepFrame();
}

const uint32_t *nes_framebuffer(NESRef nes) {
    if (!nes) {
        return NULL;
    }
    return nes->ppu.frameBuffer.pixels;
}

int nes_framebuffer_width(void) { return NES_WIDTH; }
int nes_framebuffer_height(void) { return NES_HEIGHT; }

void nes_set_button(NESRef nes, uint8_t button, bool pressed) {
    if (!nes) {
        return;
    }
    nes->bus.controller.setButton(button, pressed);
}

float nes_apu_next_sample(NESRef nes, double sample_rate) {
    if (!nes) {
        return 0.0f;
    }
    return nes->apu.nextSample(sample_rate);
}

void nes_apu_fill_buffer(NESRef nes, double sample_rate, float *out, int count) {
    if (!nes) {
        return;
    }
    nes->apu.fillBuffer(sample_rate, out, count);
}
