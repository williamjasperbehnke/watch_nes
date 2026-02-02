#include "../include/controller.hpp"

void Controller::setButton(uint8_t button, bool pressed) {
    if (pressed) {
        state |= button;
    } else {
        state &= (uint8_t)~button;
    }
}

void Controller::write(uint8_t data) {
    strobe = (data & 0x01) != 0;
    if (strobe) {
        shift = state;
    }
}

uint8_t Controller::read() {
    if (strobe) {
        return (uint8_t)(state & 0x01);
    }
    uint8_t value = (uint8_t)(shift & 0x01);
    shift >>= 1;
    return value;
}
