#pragma once
#include <cstdint>

class Battery
{
private:
    static std::uint32_t m_average;
    static std::uint32_t readADC();
public:
    static void init();
    static std::uint8_t getPercentage();
};