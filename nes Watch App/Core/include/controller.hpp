#ifndef NESC_CONTROLLER_H
#define NESC_CONTROLLER_H

#include "types.hpp"

class Controller {
public:
    uint8_t state;
    uint8_t shift;
    bool strobe;

    Controller() : state(0), shift(0), strobe(false) {}

    void setButton(uint8_t button, bool pressed);
    void write(uint8_t data);
    uint8_t read();
};

#endif
