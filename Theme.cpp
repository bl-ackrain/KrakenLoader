#include "Themes.h"
#include <cstring>
#include <cstdlib>
#include "SDFileSystem.h"
#include "LCD.h"
#include "Loader.h"

std::uint16_t* Theme::icons[5]={nullptr};

bool Theme::loadTheme(const char* name)
{
    char path[_MAX_LFN+1];
    path[0]=0;
    strcat(path, "kraken/Themes/");
    strcat(path, name);
    

    if(!loadIcons(path))
        return false;

    strcat(path, "/colors.txt");
    FIL fh;
    FRESULT res = f_open(&fh, path, FA_READ);
    
    if(res==FR_OK)
    {
        char *data = new char[fh.fsize];
        std::size_t n;
        f_read(&fh, data, fh.fsize, &n);
        f_close(&fh);

        themeParser(data, fh.fsize);
        delete data;
        return true;
    }
    return false;
}

std::uint32_t Theme::parseColor(const char* str) 
{ 
    std::uint32_t res = 0;

    for (auto i = 0; i < 6; i++)
    {
        std::uint8_t value=0;
        if(str[i] >= '0' && str[i]<= '9')
            value=str[i]-'0';
        else if(str[i] >= 'a' && str[i]<= 'f')
            value=(str[i]-'a')+0xA;
        else if(str[i] >= 'A' && str[i]<= 'F')
            value=(str[i]-'A')+0xA;
        else if(str[i] == '\n' || str[i]== ';')
            return res;
        else
            continue;

        res = (res << 4) | (value & 0xF);
    }
    return res; 
}

bool Theme::themeParser(const char* str, std::size_t size)
{
    std::size_t i=0;
    std::size_t colorNum=0;
    std::uint16_t color=0x0000;

    while(i < size)
    {
        if(str[i]==';')
        {
            while(str[i]!='\n')
                i++;
        }
        else if((str[i] >= '0' && str[i]<= '9')|| (str[i] >= 'A' && str[i]<= 'F') || (str[i] >= 'a' && str[i]<= 'f'))
        {
            color=LCD::RGB24toRGB16(parseColor(str+i));

            i+=6;
            switch(colorNum)
            {
                case 0:
                    BackgroundTop = color;
                    break;
                case 1:
                    BackgroundBottom = color;
                    break;
                case 2:
                    Banner = color;
                    break;
                case 3:
                    Boxes = color;
                    break;
                case 4:
                    BoxBorder = color;
                    break;
                case 5:
                    Text = color;
                    break;
                case 6:
                    Text2 = color;
                    break;
                case 7:
                    Title = color;
                    break;
                case 8:
                    SelectedText = color;
                    break;
                case 9:
                    Selected = color;
                    break;
                case 10:
                    Button = color;
                    break;
                case 11:
                    ButtonText = color;
                    break;
                case 12:
                    ProgressBar = color;
                    break;
                case 13:
                    Error = color;
                    break;
            }
            colorNum++;
        }
        else
            i++;
    }
    return true;
}

bool Theme::loadIcons(const char* path)
{
    char iconPath[_MAX_LFN+1];
    
    for(auto i=0;i<5;i++)
    {
        if(icons[i]!=nullptr)
            delete icons[i];

        iconPath[0]=0;
        strcat(iconPath, path);
        strcat(iconPath, "/icons/");
        
        FIL IconFile;
        FRESULT res=FRESULT::FR_NO_PATH;
        if(i==static_cast<int>(ItemType::Folder))
            strcat(iconPath, "Folder.565");
        if(i==static_cast<int>(ItemType::File))
            strcat(iconPath, "File.565");
        if(i==static_cast<int>(ItemType::BinGame) || i==static_cast<int>(ItemType::Pop))
            strcat(iconPath, "Game.565");
        if(i==static_cast<int>(ItemType::Gameboy))
            strcat(iconPath, "GB.565");
        
        res=f_open(&IconFile, iconPath, FA_READ);
        if(res!=FR_OK)
        {
            f_close(&IconFile);
            return false;
        }
        icons[i] = new std::uint16_t[24*24];
        std::size_t count;
        f_read(&IconFile, icons[i], 24*24*2 ,&count);

        f_close(&IconFile);
    }

    return true;
}