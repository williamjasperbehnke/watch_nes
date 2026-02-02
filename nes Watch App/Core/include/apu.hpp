#ifndef NESC_APU_H
#define NESC_APU_H

#include "types.hpp"
#include <pthread.h>

typedef uint8_t (*ApuReadFunc)(void *context, uint16_t addr);

class PulseChannel {
public:
    uint8_t control;
    uint16_t timer;
    uint8_t lengthCounter;
    bool enabled;
    double phase;
    uint8_t envDivider;
    uint8_t envDecay;
    bool envStart;
    uint8_t sweepDivider;
    uint8_t sweepPeriod;
    uint8_t sweepShift;
    bool sweepEnabled;
    bool sweepNegate;
    bool sweepReload;
    bool sweepMute;
    bool sweepOnesComplement;

    void writeControl(uint8_t data);
    void writeSweep(uint8_t data);
    void writeTimerLow(uint8_t data);
    void writeTimerHigh(uint8_t data);
    void setEnabled(bool value);
    void tickLength();
    void tickEnvelope();
    void tickSweep();
    double sample(double sampleRate);

private:
    void applySweep();
};

class TriangleChannel {
public:
    uint8_t control;
    uint16_t timer;
    uint16_t timerCounter;
    uint8_t lengthCounter;
    bool enabled;
    uint8_t linearCounter;
    uint8_t linearReload;
    bool linearControl;
    bool linearReloadFlag;
    uint8_t sequencePos;

    void writeControl(uint8_t data);
    void writeTimerLow(uint8_t data);
    void writeTimerHigh(uint8_t data);
    void setEnabled(bool value);
    void tickLength();
    void tickLinear();
    void tickTimer();
    double sample() const;
};

class NoiseChannel {
public:
    uint8_t control;
    uint16_t timer;
    uint16_t timerCounter;
    uint8_t lengthCounter;
    bool enabled;
    uint16_t lfsr;
    uint8_t envDivider;
    uint8_t envDecay;
    bool envStart;

    void writeControl(uint8_t data);
    void writePeriod(uint8_t data);
    void writeLength(uint8_t data);
    void setEnabled(bool value);
    void tickLength();
    void tickEnvelope();
    void tickTimer();
    double sample() const;
};

class DmcChannel {
public:
    uint8_t control;
    uint8_t directLoad;
    uint16_t sampleAddress;
    uint16_t sampleLength;
    uint16_t currentAddress;
    uint16_t bytesRemaining;
    uint8_t shiftRegister;
    uint8_t bitCount;
    uint8_t outputLevel;
    bool enabled;
    bool irqEnabled;
    bool loop;
    bool sampleBufferEmpty;
    uint8_t sampleBuffer;
    uint16_t timer;
    uint16_t timerCounter;

    void writeControl(uint8_t data);
    void writeDirectLoad(uint8_t data);
    void writeSampleAddress(uint8_t data);
    void writeSampleLength(uint8_t data);
    void setEnabled(bool value);
    void fetchSample(ApuReadFunc read, void *context);
    void tickTimer();
    double sample() const;

private:
    void restart();
};

class APU {
public:
    PulseChannel pulse1;
    PulseChannel pulse2;
    TriangleChannel triangle;
    NoiseChannel noise;
    DmcChannel dmc;
    int frameCounterCycle;
    bool frameCounterMode;
    bool frameIrqInhibit;
    double outputFilter;
    double sampleCycleRemainder;
    pthread_mutex_t mutex;
    ApuReadFunc read;
    void *readContext;

    void init();
    void reset();
    void setReadCallback(ApuReadFunc readFunc, void *context);
    void cpuWrite(uint16_t addr, uint8_t data);
    uint8_t readStatus();
    void step(int cycles);
    float nextSample(double sample_rate);
    void fillBuffer(double sample_rate, float *out, int count);

private:
    void quarterFrame();
    void halfFrame();
    void stepCyclesUnlocked(int cycles);
    float nextSampleUnlocked(double sample_rate);
};

#endif
