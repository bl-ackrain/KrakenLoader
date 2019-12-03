#pragma once
#include <cstdint>

class Buttons
{

    static constexpr auto NUM_BTN = 7;
    static std::uint32_t states[];
public:
    static constexpr auto BTN_UP = 1;
    static constexpr auto BTN_RIGHT = 2;
    static constexpr auto BTN_DOWN = 3;
    static constexpr auto BTN_LEFT = 0;
    static constexpr auto BTN_A = 4;
    static constexpr auto BTN_B = 5;
    static constexpr auto BTN_C = 6;

    static void Init();
    static void update();
    static bool pressed(std::uint8_t button);
    static bool released(std::uint8_t button);
    static bool held(std::uint8_t button, std::uint32_t time);
    static bool repeat(std::uint8_t button, std::uint32_t period);
    static std::uint32_t timeHeld(std::uint8_t button);
};