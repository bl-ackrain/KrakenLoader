#include "Pop.h"
#include "LCD.h"
#include "Palette.h"
#include "Loader.h"

FIL *Pop::popFile=nullptr;
Pop::Tag Pop::m_Tag;

bool Pop::open(char* path)
{
    if(popFile)
        close();
    popFile= new FIL();
    f_open(popFile, path, FA_READ);

    return (popFile!=0);
}

void Pop::close()
{
    if(popFile)
    {
        f_close(popFile);
        delete popFile;
        popFile=nullptr;
    }
}

bool Pop::findTag(const PopTag targetTag, bool rewind)
{
    if( !popFile )
        return false;
    if(rewind)
        f_lseek(popFile, 0);

    while(true)
    {
        std::size_t count=0;
        f_read(popFile, &m_Tag, sizeof(m_Tag), &count);
        if(count==0)
            break;
        if( m_Tag.id == targetTag )
            return true;
        else if( m_Tag.id == PopTag::TAG_CODE )
            break;
        else
            f_lseek(popFile, popFile->fptr+m_Tag.lenght);

    }
    return false;
}

char* Pop::getAString(const PopTag targetTag)
{
    if(!findTag(targetTag))
        return 0;

    char *string=new char[256+8+1];
    std::size_t i=0;
    while(i<256+8)
    {
        char c=0;
        std::size_t count=0;
        f_read(popFile, &c, 1, &count);
        if(!c)
            break;
        string[i++]=c;
    }
    string[i]=0;
    return string;
}

bool Pop::drawIcon24(uint16_t x, uint16_t y)
{
    return m_drawIcon(x, y, 24);
}

bool Pop::drawIcon36(uint16_t x, uint16_t y)
{
    return m_drawIcon(x, y, 36);
}

bool Pop::drawScreenShoot(std::size_t number)
{
    if(!findTag(PopTag::TAG_IMG_220X176_565))
        return false;

    for(std::size_t i=0;i<number;i++)
    {
        f_lseek(popFile, popFile->fptr+m_Tag.lenght);
        if(!findTag(PopTag::TAG_IMG_220X176_565, false))
            return false;
    }

    LCD::drawBitmap565File(popFile, 0, 0, LCD::WIDTH, LCD::HEIGHT);
    return true;
}

bool Pop::drawBanner(uint16_t x, uint16_t y)
{
    bool noBanner=false;
    if(!findTag(PopTag::TAG_IMG_200x80_565))
    {
        noBanner=true;
        if(!findTag(PopTag::TAG_IMG_220X176_565))
            return false;
    }
    if(noBanner)
        f_lseek(popFile, popFile->fptr+(10*LCD::WIDTH*2)+20);

    LCD::drawBitmap565File(popFile, x, y, 200, 80, noBanner?20:0);
    return true;
}

bool Pop::m_drawIcon(const uint16_t x, const uint16_t y, const uint16_t size)
{
    PopTag tag=(size==24)?PopTag::TAG_IMG_24X24_565:PopTag::TAG_IMG_36X36_565;
    if(!findTag(tag))
    {
        tag=(size==24)?PopTag::TAG_IMG_24X24_4BPP:PopTag::TAG_IMG_36X36_4BPP;
        if(!findTag(tag))
            return false;
        else
        {
            uint8_t buff[(size>1)*size];

            std::size_t count=0;
            f_read(popFile, buff, sizeof(buff), &count);
            if(count < sizeof(buff))
                return false;
            
            LCD::drawBitmap16(x, y, size, size,  buff, popPalette, 0xFF, 0);
            return true;
        }

        return false;
    }

    LCD::drawBitmap565File(popFile, x, y, size, size);
    return true;
}

std::size_t Pop::seekToCode()
{
    if(!findTag(PopTag::TAG_CODE))
        return false;

    std::size_t fsize = f_size(popFile);
    uint32_t pos=popFile->fptr;

    f_lseek(popFile, pos-8);

    return (fsize-(pos-8));
}