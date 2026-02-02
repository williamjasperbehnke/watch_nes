#ifndef NESC_CPU_H
#define NESC_CPU_H

#include "bus.hpp"
#include <string.h>

typedef enum {
    CPU_FLAG_C = 0x01,
    CPU_FLAG_Z = 0x02,
    CPU_FLAG_I = 0x04,
    CPU_FLAG_D = 0x08,
    CPU_FLAG_B = 0x10,
    CPU_FLAG_U = 0x20,
    CPU_FLAG_V = 0x40,
    CPU_FLAG_N = 0x80
} CPUFlag;

typedef enum {
    ADDR_IMP,
    ADDR_IMM,
    ADDR_ZP0,
    ADDR_ZPX,
    ADDR_ZPY,
    ADDR_ABS,
    ADDR_ABX,
    ADDR_ABY,
    ADDR_IND,
    ADDR_IZX,
    ADDR_IZY,
    ADDR_REL
} AddressingMode;

typedef enum {
    ACCESS_READ,
    ACCESS_WRITE,
    ACCESS_READ_MODIFY_WRITE,
    ACCESS_IMPLIED
} AccessKind;

class CPU;

typedef uint8_t (*cpu_op)(class CPU *cpu);

typedef struct {
    const char *name;
    cpu_op operate;
    cpu_op addrMode;
    AddressingMode mode;
    AccessKind access;
    uint8_t cycles;
} Instruction;

class CPU {
public:
    Bus *bus;
    uint8_t a;
    uint8_t x;
    uint8_t y;
    uint8_t sp;
    uint16_t pc;
    uint8_t status;
    uint8_t fetched;
    uint16_t addrAbs;
    uint16_t addrRel;
    uint8_t opcode;
    uint8_t baseHigh;
    int cycleCounter;
    Instruction instructions[256];

    CPU() {
        memset(this, 0, sizeof(CPU));
    }

    void init();
    void reset();
    void irq();
    void nmi();
    int step();
    uint8_t read(uint16_t addr);
    void write(uint16_t addr, uint8_t data);
    void push(uint8_t value);
    uint8_t pop();
    uint8_t getFlag(CPUFlag flag);
    void setFlag(CPUFlag flag, bool value);
    void setZN(uint8_t value);
    uint8_t fetch();
    void impliedDummyRead();
};

#endif
