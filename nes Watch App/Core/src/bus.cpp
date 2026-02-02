#include "../include/bus.hpp"

uint8_t Bus::cpuReadInternal(uint16_t addr) {
    if (cartridge) {
        uint8_t cartData = 0;
        if (cartridge->cpuRead(addr, &cartData)) {
            dataBus = cartData;
            return cartData;
        }
    }

    if (addr <= 0x1FFF) {
        uint8_t value = cpuRam[addr & 0x07FF];
        dataBus = value;
        return value;
    }
    if (addr >= 0x6000 && addr <= 0x7FFF) {
        uint8_t value = prgRam[addr & 0x1FFF];
        dataBus = value;
        return value;
    }
    if (addr <= 0x3FFF) {
        uint16_t reg = (uint16_t)(0x2000 + (addr & 0x0007));
        uint8_t value = ppu ? ppu->cpuRead(reg) : dataBus;
        dataBus = value;
        return value;
    }
    if (addr == 0x4016) {
        uint8_t value = controller.read();
        uint8_t result = (uint8_t)((dataBus & 0xE0) | (value & 0x01));
        dataBus = result;
        return result;
    }
    if (addr == 0x4017) {
        uint8_t value = dataBus;
        dataBus = value;
        return value;
    }
    if (addr == 0x4015) {
        uint8_t value = apu ? apu->readStatus() : dataBus;
        dataBus = value;
        return value;
    }
    return dataBus;
}

uint8_t Bus::cpuRead(uint16_t addr) {
    return cpuReadInternal(addr);
}

uint8_t Bus::cpuReadOpcode(uint16_t addr) {
    return cpuReadInternal(addr);
}

void Bus::startDma(uint8_t page) {
    dmaActive = true;
    dmaPage = page;
    dmaIndex = 0;
    dmaCycle = 0;
}

void Bus::stepDma() {
    if (!dmaActive) {
        return;
    }
    if (dmaCycle % 2 == 0) {
        uint16_t addr = (uint16_t)(dmaPage << 8) | dmaIndex;
        dmaData = cpuRead(addr);
    } else {
        if (ppu) {
            ppu->dmaWriteOam(dmaData);
        }
        dmaIndex += 1;
        if (dmaIndex == 256) {
            dmaActive = false;
        }
    }
    dmaCycle += 1;
}

void Bus::requestStall(int cycles) {
    if (cycles > stallCycles) {
        stallCycles = cycles;
    }
}

bool Bus::consumeStall() {
    if (stallCycles > 0) {
        stallCycles -= 1;
        stepDma();
        return true;
    }
    return false;
}

void Bus::cpuWrite(uint16_t addr, uint8_t data) {
    dataBus = data;

    if (cartridge && cartridge->cpuWrite(addr, data)) {
        return;
    }

    if (addr <= 0x1FFF) {
        cpuRam[addr & 0x07FF] = data;
        return;
    }
    if (addr >= 0x6000 && addr <= 0x7FFF) {
        prgRam[addr & 0x1FFF] = data;
        return;
    }
    if (addr <= 0x3FFF) {
        if (ppu) {
            ppu->cpuWrite((uint16_t)(0x2000 + (addr & 0x0007)), data);
        }
        return;
    }
    if (addr == 0x4014) {
        startDma(data);
        return;
    }
    if (addr == 0x4016) {
        controller.write(data);
        return;
    }
    if ((addr >= 0x4000 && addr <= 0x4013) || addr == 0x4015 || addr == 0x4017) {
        if (apu) {
            apu->cpuWrite(addr, data);
        }
        if (addr == 0x4017) {
            bool irqEnabled = (data & 0x40) == 0;
            irqPending = irqEnabled;
        }
        return;
    }
}

bool Bus::isIrqPending() {
    return irqPending;
}

void Bus::ackIrq() {
    irqPending = false;
}

void Bus::tick(int cycles) {
    (void)cycles;
}

void Bus::setCpuBus(uint8_t value) {
    dataBus = value;
}
