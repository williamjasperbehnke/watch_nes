#include "../include/apu.hpp"

#include <math.h>
#include <string.h>

static const uint8_t apu_length_table[32] = {
    10, 254, 20, 2, 40, 4, 80, 6,
    160, 8, 60, 10, 14, 12, 26, 14,
    12, 16, 24, 18, 48, 20, 96, 22,
    192, 24, 72, 26, 16, 28, 32, 30
};

static const uint16_t noise_period_table[16] = {
    4, 8, 16, 32, 64, 96, 128, 160,
    202, 254, 380, 508, 762, 1016, 2034, 4068
};

static const uint16_t dmc_rate_table[16] = {
    428, 380, 340, 320, 286, 254, 226, 214,
    190, 160, 142, 128, 106, 85, 72, 54
};

static const uint8_t triangle_sequence[32] = {
    15, 14, 13, 12, 11, 10, 9, 8,
    7, 6, 5, 4, 3, 2, 1, 0,
    0, 1, 2, 3, 4, 5, 6, 7,
    8, 9, 10, 11, 12, 13, 14, 15
};

static const double apu_cpu_clock = 1789773.0;

void PulseChannel::writeControl(uint8_t data) {
    control = data;
    envStart = true;
}

void PulseChannel::writeSweep(uint8_t data) {
    sweepEnabled = (data & 0x80) != 0;
    sweepPeriod = (data >> 4) & 0x07;
    sweepNegate = (data & 0x08) != 0;
    sweepShift = data & 0x07;
    sweepReload = true;
}

void PulseChannel::writeTimerLow(uint8_t data) {
    timer = (uint16_t)((timer & 0xFF00) | data);
}

void PulseChannel::writeTimerHigh(uint8_t data) {
    timer = (uint16_t)((timer & 0x00FF) | ((data & 0x07) << 8));
    uint8_t lengthIndex = (data >> 3) & 0x1F;
    lengthCounter = apu_length_table[lengthIndex];
    envStart = true;
}

void PulseChannel::setEnabled(bool value) {
    enabled = value;
    if (!enabled) {
        lengthCounter = 0;
    }
}

void PulseChannel::tickLength() {
    bool lengthHalt = (control & 0x20) != 0;
    if (!lengthHalt && lengthCounter > 0) {
        lengthCounter -= 1;
    }
}

void PulseChannel::tickEnvelope() {
    uint8_t volume = control & 0x0F;
    bool loop = (control & 0x20) != 0;
    if (envStart) {
        envStart = false;
        envDecay = 15;
        envDivider = volume;
        return;
    }
    if (envDivider == 0) {
        envDivider = volume;
        if (envDecay > 0) {
            envDecay -= 1;
        } else if (loop) {
            envDecay = 15;
        }
    } else {
        envDivider -= 1;
    }
}

void PulseChannel::applySweep() {
    if (!sweepEnabled || sweepShift == 0) {
        sweepMute = false;
        return;
    }
    uint16_t change = (uint16_t)(timer >> sweepShift);
    uint16_t target;
    if (sweepNegate) {
        target = (uint16_t)(timer - change - (sweepOnesComplement ? 1 : 0));
    } else {
        target = (uint16_t)(timer + change);
    }
    sweepMute = (target > 0x7FF) || (timer < 8);
    if (!sweepMute) {
        timer = target;
    }
}

void PulseChannel::tickSweep() {
    if (sweepReload) {
        sweepReload = false;
        sweepDivider = sweepPeriod;
        if (sweepEnabled) {
            applySweep();
        }
        return;
    }
    if (sweepDivider == 0) {
        sweepDivider = sweepPeriod;
        if (sweepEnabled) {
            applySweep();
        }
    } else {
        sweepDivider -= 1;
    }
}

double PulseChannel::sample(double sampleRate) {
    if (!enabled || lengthCounter == 0) {
        return 0.0;
    }
    if (timer < 8 || sweepMute) {
        return 0.0;
    }

    uint8_t duty = (uint8_t)((control >> 6) & 0x03);
    double dutyCycle = 0.75;
    switch (duty) {
        case 0: dutyCycle = 0.125; break;
        case 1: dutyCycle = 0.25; break;
        case 2: dutyCycle = 0.5; break;
        default: dutyCycle = 0.75; break;
    }

    double frequency = apu_cpu_clock / (16.0 * ((double)timer + 1.0));
    if (!isfinite(frequency) || frequency <= 0.0) {
        return 0.0;
    }

    phase += frequency / sampleRate;
    if (phase >= 1.0) {
        phase -= floor(phase);
    }

    bool constantVolume = (control & 0x10) != 0;
    uint8_t env = constantVolume ? (control & 0x0F) : envDecay;
    double volume = (double)env;
    return (phase < dutyCycle) ? volume : 0.0;
}

void TriangleChannel::writeControl(uint8_t data) {
    linearControl = (data & 0x80) != 0;
    linearReload = data & 0x7F;
}

void TriangleChannel::writeTimerLow(uint8_t data) {
    timer = (uint16_t)((timer & 0xFF00) | data);
}

void TriangleChannel::writeTimerHigh(uint8_t data) {
    timer = (uint16_t)((timer & 0x00FF) | ((data & 0x07) << 8));
    uint8_t lengthIndex = (data >> 3) & 0x1F;
    lengthCounter = apu_length_table[lengthIndex];
    linearReloadFlag = true;
}

void TriangleChannel::setEnabled(bool value) {
    enabled = value;
    if (!enabled) {
        lengthCounter = 0;
    }
}

void TriangleChannel::tickLength() {
    if (!linearControl && lengthCounter > 0) {
        lengthCounter -= 1;
    }
}

void TriangleChannel::tickLinear() {
    if (linearReloadFlag) {
        linearCounter = linearReload;
    } else if (linearCounter > 0) {
        linearCounter -= 1;
    }
    if (!linearControl) {
        linearReloadFlag = false;
    }
}

void TriangleChannel::tickTimer() {
    if (timerCounter == 0) {
        timerCounter = timer;
        if (lengthCounter > 0 && linearCounter > 0) {
            sequencePos = (uint8_t)((sequencePos + 1) & 0x1F);
        }
    } else {
        timerCounter -= 1;
    }
}

double TriangleChannel::sample() const {
    if (!enabled || lengthCounter == 0 || linearCounter == 0) {
        return 0.0;
    }
    return (double)triangle_sequence[sequencePos];
}

void NoiseChannel::writeControl(uint8_t data) {
    control = data;
    envStart = true;
}

void NoiseChannel::writePeriod(uint8_t data) {
    uint8_t idx = data & 0x0F;
    timer = noise_period_table[idx];
    control = (uint8_t)((control & 0x7F) | (data & 0x80));
}

void NoiseChannel::writeLength(uint8_t data) {
    uint8_t lengthIndex = (data >> 3) & 0x1F;
    lengthCounter = apu_length_table[lengthIndex];
    envStart = true;
}

void NoiseChannel::setEnabled(bool value) {
    enabled = value;
    if (!enabled) {
        lengthCounter = 0;
    }
}

void NoiseChannel::tickLength() {
    bool lengthHalt = (control & 0x20) != 0;
    if (!lengthHalt && lengthCounter > 0) {
        lengthCounter -= 1;
    }
}

void NoiseChannel::tickEnvelope() {
    uint8_t volume = control & 0x0F;
    bool loop = (control & 0x20) != 0;
    if (envStart) {
        envStart = false;
        envDecay = 15;
        envDivider = volume;
        return;
    }
    if (envDivider == 0) {
        envDivider = volume;
        if (envDecay > 0) {
            envDecay -= 1;
        } else if (loop) {
            envDecay = 15;
        }
    } else {
        envDivider -= 1;
    }
}

void NoiseChannel::tickTimer() {
    if (timerCounter == 0) {
        timerCounter = timer;
        uint16_t feedback;
        bool mode = (control & 0x80) != 0;
        if (mode) {
            feedback = (uint16_t)(((lfsr & 0x0001) ^ ((lfsr >> 6) & 0x0001)) & 0x0001);
        } else {
            feedback = (uint16_t)(((lfsr & 0x0001) ^ ((lfsr >> 1) & 0x0001)) & 0x0001);
        }
        lfsr = (uint16_t)((lfsr >> 1) | (feedback << 14));
    } else {
        timerCounter -= 1;
    }
}

double NoiseChannel::sample() const {
    if (!enabled || lengthCounter == 0) {
        return 0.0;
    }
    if (lfsr & 0x0001) {
        return 0.0;
    }
    bool constantVolume = (control & 0x10) != 0;
    uint8_t env = constantVolume ? (control & 0x0F) : envDecay;
    return (double)env;
}

void DmcChannel::restart() {
    currentAddress = sampleAddress;
    bytesRemaining = sampleLength;
}

void DmcChannel::setEnabled(bool value) {
    enabled = value;
    if (!enabled) {
        bytesRemaining = 0;
    } else if (bytesRemaining == 0) {
        restart();
    }
}

void DmcChannel::writeControl(uint8_t data) {
    control = data;
    irqEnabled = (data & 0x80) != 0;
    loop = (data & 0x40) != 0;
    timer = dmc_rate_table[data & 0x0F];
}

void DmcChannel::writeDirectLoad(uint8_t data) {
    directLoad = data & 0x7F;
    outputLevel = directLoad;
}

void DmcChannel::writeSampleAddress(uint8_t data) {
    sampleAddress = (uint16_t)(0xC000 | ((uint16_t)data << 6));
}

void DmcChannel::writeSampleLength(uint8_t data) {
    sampleLength = (uint16_t)((uint16_t)data * 16 + 1);
}

void DmcChannel::fetchSample(ApuReadFunc readFunc, void *context) {
    if (!sampleBufferEmpty || bytesRemaining == 0) {
        return;
    }
    if (readFunc) {
        sampleBuffer = readFunc(context, currentAddress);
    } else {
        sampleBuffer = 0;
    }
    sampleBufferEmpty = false;
    currentAddress = (uint16_t)(currentAddress + 1);
    if (currentAddress == 0) {
        currentAddress = 0x8000;
    }
    if (bytesRemaining > 0) {
        bytesRemaining -= 1;
    }
    if (bytesRemaining == 0 && loop) {
        restart();
    }
}

void DmcChannel::tickTimer() {
    if (timerCounter == 0) {
        timerCounter = timer;
        if (bitCount == 0) {
            if (!sampleBufferEmpty) {
                shiftRegister = sampleBuffer;
                sampleBufferEmpty = true;
                bitCount = 8;
            } else {
                return;
            }
        }
        if (bitCount > 0) {
            if (shiftRegister & 0x01) {
                if (outputLevel <= 125) {
                    outputLevel = (uint8_t)(outputLevel + 2);
                }
            } else {
                if (outputLevel >= 2) {
                    outputLevel = (uint8_t)(outputLevel - 2);
                }
            }
            shiftRegister >>= 1;
            bitCount -= 1;
        }
    } else {
        timerCounter -= 1;
    }
}

double DmcChannel::sample() const {
    return (double)outputLevel;
}

void APU::init() {
    memset(this, 0, sizeof(*this));
    pthread_mutex_init(&mutex, NULL);
    pulse1.sweepOnesComplement = true;
    noise.lfsr = 1;
    dmc.sampleBufferEmpty = true;
    outputFilter = 0.0;
    sampleCycleRemainder = 0.0;
}

void APU::reset() {
    ApuReadFunc readFunc = read;
    void *context = readContext;
    pthread_mutex_destroy(&mutex);
    memset(this, 0, sizeof(*this));
    pthread_mutex_init(&mutex, NULL);
    read = readFunc;
    readContext = context;
    pulse1.sweepOnesComplement = true;
    noise.lfsr = 1;
    dmc.sampleBufferEmpty = true;
    outputFilter = 0.0;
    sampleCycleRemainder = 0.0;
}

void APU::setReadCallback(ApuReadFunc readFunc, void *context) {
    read = readFunc;
    readContext = context;
}

void APU::cpuWrite(uint16_t addr, uint8_t data) {
    pthread_mutex_lock(&mutex);
    switch (addr) {
        case 0x4000: pulse1.writeControl(data); break;
        case 0x4001: pulse1.writeSweep(data); break;
        case 0x4002: pulse1.writeTimerLow(data); break;
        case 0x4003: pulse1.writeTimerHigh(data); break;
        case 0x4004: pulse2.writeControl(data); break;
        case 0x4005: pulse2.writeSweep(data); break;
        case 0x4006: pulse2.writeTimerLow(data); break;
        case 0x4007: pulse2.writeTimerHigh(data); break;
        case 0x4008: triangle.writeControl(data); break;
        case 0x400A: triangle.writeTimerLow(data); break;
        case 0x400B: triangle.writeTimerHigh(data); break;
        case 0x400C: noise.writeControl(data); break;
        case 0x400E: noise.writePeriod(data); break;
        case 0x400F: noise.writeLength(data); break;
        case 0x4010: dmc.writeControl(data); break;
        case 0x4011: dmc.writeDirectLoad(data); break;
        case 0x4012: dmc.writeSampleAddress(data); break;
        case 0x4013: dmc.writeSampleLength(data); break;
        case 0x4015:
            pulse1.setEnabled((data & 0x01) != 0);
            pulse2.setEnabled((data & 0x02) != 0);
            triangle.setEnabled((data & 0x04) != 0);
            noise.setEnabled((data & 0x08) != 0);
            dmc.setEnabled((data & 0x10) != 0);
            break;
        case 0x4017:
            frameCounterMode = (data & 0x80) != 0;
            frameIrqInhibit = (data & 0x40) != 0;
            frameCounterCycle = 0;
            if (frameCounterMode) {
                pulse1.tickLength();
                pulse2.tickLength();
                triangle.tickLength();
                noise.tickLength();
                pulse1.tickSweep();
                pulse2.tickSweep();
                pulse1.tickEnvelope();
                pulse2.tickEnvelope();
                noise.tickEnvelope();
                triangle.tickLinear();
            }
            break;
        default:
            break;
    }
    pthread_mutex_unlock(&mutex);
}

uint8_t APU::readStatus() {
    pthread_mutex_lock(&mutex);
    uint8_t value = 0;
    if (pulse1.enabled && pulse1.lengthCounter > 0) {
        value |= 0x01;
    }
    if (pulse2.enabled && pulse2.lengthCounter > 0) {
        value |= 0x02;
    }
    if (triangle.enabled && triangle.lengthCounter > 0) {
        value |= 0x04;
    }
    if (noise.enabled && noise.lengthCounter > 0) {
        value |= 0x08;
    }
    if (dmc.bytesRemaining > 0) {
        value |= 0x10;
    }
    pthread_mutex_unlock(&mutex);
    return value;
}

void APU::quarterFrame() {
    pulse1.tickEnvelope();
    pulse2.tickEnvelope();
    noise.tickEnvelope();
    triangle.tickLinear();
}

void APU::halfFrame() {
    pulse1.tickLength();
    pulse2.tickLength();
    triangle.tickLength();
    noise.tickLength();
    pulse1.tickSweep();
    pulse2.tickSweep();
}

void APU::stepCyclesUnlocked(int cycles) {
    for (int i = 0; i < cycles; i++) {
        frameCounterCycle += 1;
        if (!frameCounterMode) {
            if (frameCounterCycle == 3729) {
                quarterFrame();
            } else if (frameCounterCycle == 7457) {
                quarterFrame();
                halfFrame();
            } else if (frameCounterCycle == 11186) {
                quarterFrame();
            } else if (frameCounterCycle == 14915) {
                quarterFrame();
                halfFrame();
                frameCounterCycle = 0;
            }
        } else {
            if (frameCounterCycle == 3729) {
                quarterFrame();
            } else if (frameCounterCycle == 7457) {
                quarterFrame();
                halfFrame();
            } else if (frameCounterCycle == 11186) {
                quarterFrame();
            } else if (frameCounterCycle == 14915) {
                quarterFrame();
                halfFrame();
            } else if (frameCounterCycle == 18641) {
                frameCounterCycle = 0;
            }
        }

        triangle.tickTimer();
        noise.tickTimer();
        dmc.tickTimer();
        dmc.fetchSample(read, readContext);
    }
}

void APU::step(int cycles) {
    pthread_mutex_lock(&mutex);
    stepCyclesUnlocked(cycles);
    pthread_mutex_unlock(&mutex);
}

float APU::nextSampleUnlocked(double sample_rate) {
    double p1 = pulse1.sample(sample_rate);
    double p2 = pulse2.sample(sample_rate);
    double t = triangle.sample();
    double n = noise.sample();
    double d = dmc.sample();

    double pulseOut = 0.0;
    if (p1 + p2 > 0.0) {
        pulseOut = 95.88 / ((8128.0 / (p1 + p2)) + 100.0);
    }
    double tndOut = 0.0;
    double tnd = (t / 8227.0) + (n / 12241.0) + (d / 22638.0);
    if (tnd > 0.0) {
        tndOut = 159.79 / ((1.0 / tnd) + 100.0);
    }
    double mixed = pulseOut + tndOut;
    double cutoff = 12000.0;
    double rc = 1.0 / (2.0 * 3.141592653589793 * cutoff);
    double dt = 1.0 / sample_rate;
    double alpha = dt / (rc + dt);
    outputFilter += alpha * (mixed - outputFilter);
    return (float)outputFilter;
}

float APU::nextSample(double sample_rate) {
    float sampleValue = 0.0f;
    pthread_mutex_lock(&mutex);
    sampleValue = nextSampleUnlocked(sample_rate);
    pthread_mutex_unlock(&mutex);
    return sampleValue;
}

void APU::fillBuffer(double sample_rate, float *out, int count) {
    if (!out || count <= 0) {
        return;
    }
    double cyclesPerSample = apu_cpu_clock / sample_rate;
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < count; i++) {
        sampleCycleRemainder += cyclesPerSample;
        int cycles = (int)sampleCycleRemainder;
        if (cycles > 0) {
            stepCyclesUnlocked(cycles);
            sampleCycleRemainder -= (double)cycles;
        }
        out[i] = nextSampleUnlocked(sample_rate);
    }
    pthread_mutex_unlock(&mutex);
}
