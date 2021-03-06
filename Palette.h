#pragma once
#include <cstdlib>

enum Colors
{
    BLACK=0,
    DARK_GRAY,
    LIGHT_GRAY,
    WHITE,
    DARK_BLUE,
    BLUE,
    CAYAN,
    LIGHT_BLUE,
    PURPLE,
    PINK,
    RED,
    ORANGE,
    YELLOW,
    BROWN,
    DARK_GREEN,
    GREEN
};

static constexpr const uint16_t popPalette[16] =
{
    0x18e4,
    0x4a8a,
    0xa513,
    0xffff,
    0x2173,
    0x52bf,
    0x0d11,
    0x3d9b,
    0x8915,
    0xf494,
    0xa102,
    0xfc23,
    0xfee7,
    0x7225,
    0x53e2,
    0x8663
};