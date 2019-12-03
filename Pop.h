#pragma once
#include "SDFileSystem.h"
#include <cstdint>

enum class PopTag {
    TAG_PADDING = 0,
    TAG_INDEX,
    TAG_OFFSETADDRESS,
    TAG_CODECHUNK,
    TAG_ENTRYPOINT,
    TAG_CRC,
    TAG_HAT,
    TAG_LONGNAME,
    TAG_AUTHOR,
    TAG_DESCRIPTION,
    TAG_IMG_36X36_4BPP,
    TAG_IMG_24X24_4BPP,
    TAG_IMG_100X24_4BPP,
    TAG_IMG_110X88_4BPP,

    TAG_IMG_36X36_565,
    TAG_IMG_24X24_565,
    TAG_IMG_100X24_565,
    TAG_IMG_110X88_565,

    TAG_IMG_220X176_565,

    TAG_IMG_200x80_4BPP,
    TAG_IMG_200x80_565,

    TAG_VERSION,

    TAG_CODE = 0X10008000,
};

class Pop
{
public:
    static bool open(char* path);
    static void close();
    static char* getAString(const PopTag targetTag);

    static bool drawIcon24(const uint16_t x, const uint16_t y);
    static bool drawIcon36(const uint16_t x, const uint16_t y);
    static bool findTag(const PopTag targetTag, bool rewind=true);
    static bool drawScreenShoot(const std::size_t number);
    static bool drawBanner(uint16_t x, uint16_t y);
    static std::size_t seekToCode();
    static FIL *popFile;

    static struct Tag{
        PopTag id;
        uint32_t lenght;
    } m_Tag;
private:
    static bool m_drawIcon(const uint16_t x, const uint16_t y, const uint16_t size);
};