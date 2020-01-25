#pragma once
#include <cstdint>


class Theme
{
    public:
    std::uint16_t BackgroundTop=0x2965;
    std::uint16_t Text=0xffff;
    std::uint16_t Text2=0xCE79;
    std::uint16_t SelectedText=0x2965;
    std::uint16_t ButtonText=0xD69A;
    std::uint16_t Button=0x528A;
    std::uint16_t Title=0x1DFC;
    std::uint16_t Boxes=0x18C3;
    std::uint16_t BoxBorder=0x1DFC;
    std::uint16_t Selected=0x1DFC;
    std::uint16_t Error=0xDAEA;
    std::uint16_t ProgressBar=0x3d9b;
    std::uint16_t Banner=0x2965;
    std::uint16_t BackgroundBottom=0x919D;

    bool loadTheme(const char* name);
    static std::uint16_t* icons[5];

private:
    bool themeParser(const char* str, std::size_t size);
    std::uint32_t parseColor(const char* str) ;
    bool loadIcons(const char* path);
};
