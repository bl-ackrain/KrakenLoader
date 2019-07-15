#pragma once
#include <mbed.h>


class Buttons
{
    static constexpr auto NUM_BTN = 7;
    static uint32_t states[];
public:
    static constexpr auto BTN_UP = 1;
    static constexpr auto BTN_RIGHT = 2;
    static constexpr auto BTN_DOWN = 3;
    static constexpr auto BTN_LEFT = 0;
    static constexpr auto BTN_A = 4;
    static constexpr auto BTN_B = 5;
    static constexpr auto BTN_C = 6;

    static DigitalIn ABtn;
    static DigitalIn BBtn;
    static DigitalIn CBtn;
    static DigitalIn UpBtn;
    static DigitalIn DownBtn;
    static DigitalIn LeftBtn;
    static DigitalIn RightBtn;

    static void Init();
    static void update();
    static bool pressed(uint8_t button);
    static bool released(uint8_t button);
    static bool held(uint8_t button, uint32_t time);
    static bool repeat(uint8_t button, uint32_t period);
    static uint32_t timeHeld(uint8_t button);
};