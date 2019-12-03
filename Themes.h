#pragma once
#include <cstdint>


class Theme
{
    public:
#if 0
    static constexpr uint16_t Background=0x4a8a;
    static constexpr uint16_t Text=0x18e4;
    static constexpr uint16_t SelectedText=0xffff;
    static constexpr uint16_t Text2=0x53e2;
    static constexpr uint16_t Title=0xffff;
    static constexpr uint16_t Author=0xfee7;
    static constexpr uint16_t Boxes=0xffff;
    static constexpr uint16_t BoxBorder=0x8663;
    static constexpr uint16_t Selected=0x3d9b;
    static constexpr uint16_t Error=0xa102;
    static constexpr uint16_t ProgressBar=0x8663;
    static constexpr uint16_t Banner=0xa513;
#endif

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

    bool loadTheme(const char* path);
private:
    bool themeParser(const char* str, std::size_t size);
    std::uint32_t parseColor(const char* str) ;
};
