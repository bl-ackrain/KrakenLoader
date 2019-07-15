#include "Loader.h"
#include "LCD.h"
#include <mbed.h>
#include "Palette.h"
#include <cctype>
#include <cstring>
#include "Icons.h"
#include "Pop.h"
#include "iap.h"

bool Loader::m_redraw=true;
SDFileSystem *Loader::sdFs;
char Loader::m_curDir[_MAX_LFN+1];
size_t Loader::m_itemcount;
size_t Loader::m_itemlistcount;
size_t Loader::m_beginitem;
size_t Loader::m_selecteditem;
Item Loader::m_Items[5];

uint16_t Loader::m_indexs[512];

size_t Loader::m_prevSelecteditem[10];
size_t Loader::m_prevBeginitem[10];
uint8_t Loader::m_pPrev=0;

Loader::State Loader::m_state = Loader::State::Browse;
Loader::State Loader::m_oldstate = Loader::State::Browse;

const uint8_t Loader::gbDefaultPal[16]={0xF0, 0xBD, 0x07, 0x00, 0xA8, 0x6B, 0x05, 0x00, 0x60, 0x18, 0x03, 0x00, 0x18, 0xC6, 0x00, 0x00};

void Loader::Init()
{
    settings.Init();
    LCD::Init();
    Buttons::Init();

    char* arg = reinterpret_cast<char*>(0x20000000);

    sdFs = new SDFileSystem( P0_9, P0_8, P0_6, P0_7, 0, NC, SDFileSystem::SWITCH_NONE, 25000000 );

    if(sdFs->card_type()==SDFileSystem::CardType::CARD_UNKNOWN)
    {
        //delete sdFs;
        //sdFs=0;
        LCD::color=popPalette[Colors::RED];
        errorPopup("SD Init Faild");
        while(true)
        {};
    }
    m_redraw=true;
    memset(m_curDir,0,_MAX_LFN);
 

    *(arg+4)=0;
    if(strcmp(arg,"Load")==0)
    {
        strcpy(m_Items[0].fullPath, arg+5);
        char *fileName;
        char *ext;
        fileName=strrchr(arg+5,'/')+1;
        memcpy(m_Items[0].displayName, fileName, 27);

        ext=strrchr(fileName,'.')+1;

        if(strcmp(ext,"pop")==0)
            m_Items[0].Type=ItemType::Pop;
        else if(strcmp(ext,"gb")==0)
            m_Items[0].Type=ItemType::Gameboy;
        else if(strcmp(ext,"bin")==0)
            m_Items[0].Type=ItemType::BinGame;

        if(!loadFile())
            errorPopup("Loading File Failed");
    }
    
    m_itemcount=m_fileCount();
    m_beginitem=0;
    m_selecteditem=0;
    loadItems();

    if(settings.viewmode==Settings::ViewMode::Icons)
        m_state=State::Coverflow;
    else if(settings.viewmode==Settings::ViewMode::List)
        m_state=State::Browse;
}

void Loader::run()
{
    while(1)
    {
        update();
        draw();
    }
}

void Loader::update()
{
    time_t tmp_seconds = time(NULL);
    if(m_seconds !=tmp_seconds)
        drawClock();

    Buttons::update();
    if(m_state==State::Settings)
    {
        if(settings.update())
            settings.draw();
        else
        {
            //m_state=m_oldstate;
            if(settings.viewmode==Settings::ViewMode::Icons)
                m_state=State::Coverflow;
            else if(settings.viewmode==Settings::ViewMode::List)
                m_state=State::Browse;
            m_redraw=true;
            m_itemcount=m_fileCount();
            m_selecteditem=0;
            m_beginitem=0;
            loadItems();
            draw();
        }
         return;   
    }

    if(m_state==State::ErrorPopup)
    {
        if(Buttons::pressed(Buttons::BTN_B))
        {
            m_state=m_oldstate;
            drawAllItems();
        }
        return;
    }

    if(m_state==State::InfoPopup)
    {
        if(Buttons::pressed(Buttons::BTN_B))
        {
            //drawRoundRect(5,18,LCD::WIDTH-10,140,popPalette[Colors::DARK_GRAY]);
            LCD::fillRectangle(5+200+5, 15, 5, 144, popPalette[Colors::DARK_GRAY]);
            LCD::fillRectangle(5, 15, 5, 144, popPalette[Colors::DARK_GRAY]);
            m_state=m_oldstate;
            drawAllItems();
        }

        if(Buttons::pressed(Buttons::BTN_A))
        {
            if(!loadFile())
                errorPopup("Loading File Failed");
        }
        
        if(Buttons::pressed(Buttons::BTN_C))
        {
            if(m_Items[m_selecteditem].Type!=ItemType::Pop)
                return;
            Pop::open(m_Items[m_selecteditem].fullPath);
            size_t num=0;
            if(Pop::drawScreenShoot(num))
            {
                while(true)
                {
                    Buttons::update();

                    if(Buttons::pressed(Buttons::BTN_RIGHT)|| Buttons::pressed(Buttons::BTN_A))
                    {
                        num++;
                        if(!Pop::drawScreenShoot(num))
                            num--;
                    }

                    if(Buttons::pressed(Buttons::BTN_LEFT) && num>0 )
                    {
                        num--;
                        Pop::drawScreenShoot(num);
                    }
                        
                    if(Buttons::pressed(Buttons::BTN_B) || Buttons::pressed(Buttons::BTN_C))
                        break;
                }
            }
            Pop::close();
            m_state=m_oldstate;
            m_redraw=true;
            draw();
            infoPopup();
        }

        return;
    }

    if(Buttons::repeat(Buttons::BTN_RIGHT, 10000))
    {
        if(m_state==State::Coverflow)
            m_nextItem();
        if(m_state==State::Browse && m_itemcount > 5)
            m_nextPage();
        return;
    }

    if(Buttons::repeat(Buttons::BTN_LEFT, 10000))
    {
        if(m_state==State::Coverflow)
            m_prevItem();
        if(m_state==State::Browse && m_itemcount > 5 )
            m_prevPage();
        return;
    }

    if(Buttons::repeat(Buttons::BTN_UP, 10000))
    {
        if(m_state==State::Coverflow)
            m_prevPage();
        if(m_state==State::Browse)
            m_prevItem();
       return ;
    }

    if(Buttons::repeat(Buttons::BTN_DOWN, 10000))
    {
        if(m_state==State::Coverflow)
            m_nextPage();
        if(m_state==State::Browse)
            m_nextItem();
        
        return;
    }
    if(Buttons::pressed(Buttons::BTN_A))
    {
        if(m_itemcount < 1)
            return;
        if(m_Items[m_selecteditem].Type==ItemType::Folder)
        {
            strcpy(m_curDir, m_Items[m_selecteditem].fullPath);
            m_itemcount=m_fileCount();

            m_pushPrev();
            m_selecteditem=0;
            m_beginitem=0;
            drawTopBar();
            m_reloadItems(); 
            return;
        }
        else if(m_Items[m_selecteditem].Type==ItemType::Pop || m_Items[m_selecteditem].Type==ItemType::BinGame || m_Items[m_selecteditem].Type==ItemType::Gameboy)
        {
            infoPopup();
        }
        return;
    }

    if(Buttons::pressed(Buttons::BTN_B))
    {
        if(!m_curDir[0])
            return;
        char *slash=strrchr(m_curDir,'/');
        *slash=0;
        m_itemcount=m_fileCount();

        m_pullPrev();
        drawTopBar();
        m_reloadItems(); 
    }

    if(Buttons::pressed(Buttons::BTN_C))
    {
        if(m_curDir[0])
            return;
        drawClock();
        settings.Init();
        m_oldstate=m_state;
        m_state=State::Settings;
    }

}

void Loader::drawRoundRect(int x, int y, uint16_t w, uint16_t h, uint16_t color)
{
    LCD::fillRectangle(x+2, y, w-4, 1, color);
    LCD::fillRectangle(x+1, y+1, w-2, 1, color);

    LCD::fillRectangle(x, y+2, w, h-4, color);

    LCD::fillRectangle(x+1, y+h-2, w-2, 1, color);
    LCD::fillRectangle(x+2, y+h-1, w-4, 1, color);
}

void Loader::drawHollowRoundRect(int x, int y, uint16_t w, uint16_t h, uint16_t color)
{
    LCD::fillRectangle(x+2, y, w+2, 1, color);
    LCD::fillRectangle(x+1, y+1, w+4, 1, color);

    LCD::fillRectangle(x, y+2, 3, h, color);
    LCD::fillRectangle(x+3+w, y+2, 3, h, color);

    LCD::fillRectangle(x+1, y+h+2, w+4, 1, color);
    LCD::fillRectangle(x+2, y+h+3, w+2, 1, color);
}

void Loader::draw()
{
    if(!m_redraw)
        return;

    LCD::Clear(popPalette[Colors::DARK_GRAY]);

    drawTopBar();
    drawScrollBar();
    drawClock();

    drawAllItems();

    drawBottomBar();
    
    m_redraw=false;
}

void Loader::drawTopBar()
{
    size_t len=strlen(m_curDir);
    LCD::fillRectangle(14, 1, LCD::WIDTH-49, 10, popPalette[Colors::DARK_GRAY]);
    size_t w= 21+(len+1)*6;
    if(w >LCD::WIDTH-40)
        w=LCD::WIDTH-40;
    drawRoundRect(2, 1, w, 10, popPalette[Colors::WHITE]);
    LCD::drawBitmap16(6, 2, 8, 8, Icons::SD, popPalette, PINK, WHITE);
    LCD::cursorX=17;
    LCD::cursorY=2;
    LCD::color=popPalette[Colors::BLACK];

    if(len > 24)
    {
        LCD::print("..");
        LCD::print(m_curDir+len-24);
    }  
    else
        LCD::print(m_curDir);
    LCD::print("/");
}

void Loader::drawScrollBar()
{
    if(m_state==State::Coverflow)
        return;
    LCD::fillRectangle(LCD::WIDTH-4, 15, 2, 145, popPalette[Colors::WHITE]);
    if(m_itemcount > 5)
    {
        size_t div=145/((m_itemcount+4)/5);
        size_t div10=1450/((m_itemcount+4)/5);

        size_t start=15+(div*(m_beginitem/5))+(div10*(m_beginitem/5))/100;
        if(start > 145+15-div)
            start=145+15-div;

        LCD::fillRectangle(LCD::WIDTH-4, start , 2, div, popPalette[Colors::LIGHT_BLUE]);
    }
}

void Loader::drawBottomBar()
{
    drawRoundRect(2, LCD::HEIGHT-10, LCD::WIDTH-4, 12, popPalette[WHITE]);
    LCD::cursorX=LCD::WIDTH-50;
    LCD::cursorY=LCD::HEIGHT-9;
    LCD::color=popPalette[Colors::BLACK];
    
    LCD::printNumber((m_itemcount > 0)?m_beginitem+m_selecteditem+1:0);
    LCD::print("/");
    LCD::printNumber(m_itemcount);
    LCD::cursorX=10;
    LCD::print("A:Ok ");
    if(!m_curDir[0])
        LCD::print("C:Settings");
    else
        LCD::print("B:Back");
}

void Loader::drawClock()
{
    //LCD::fillRectangle(LCD::WIDTH-35, 1, 35-2, 10, popPalette[Colors::WHITE]);
    drawRoundRect(LCD::WIDTH-35, 1, 33, 10, popPalette[Colors::WHITE]);
    m_seconds = time(NULL);
    auto hours = (m_seconds/3600)%24;
    auto mins = (m_seconds/60)%60;

    LCD::cursorX=LCD::WIDTH-33;
    LCD::cursorY=2;
    LCD::color=popPalette[Colors::BLACK];

    if(hours < 10)
        LCD::write('0');
    LCD::printNumber(hours);
    LCD::write(':');
    if(mins < 10)
        LCD::write('0');
    LCD::printNumber(mins);
}

size_t Loader::m_fileCount()
{
    size_t c=0;
    uint16_t idx=0;
    FATDirHandle *dir = sdFs->opendir(m_curDir);

    if(!dir)
        return 0;
    
    if(settings.ShowAllFiles)
    {
        while(dir->readdir())
            c++;
        return c;
    }

    while(c < 512)
    {
        dirent *ent=dir->readdir();
        if(!ent)
            break;
        
        char *fileName=ent->d_name;

        if(dir->finfo.fattrib & AM_DIR)
        {
            m_indexs[c++]=idx;
        }
        else
        {
            char extension[4];
            char *ext=strrchr(fileName,'.')+1;
            for(auto k=0;k<3;k++)
                extension[k]=tolower(*(ext+k));
            extension[3]=0;
            if(strcmp(extension,"bin")==0 || strcmp(extension,"pop")==0 || strcmp(extension,"gb")==0)
            {
                m_indexs[c++]=idx;
            }
        }
        idx++;
    }

    dir->closedir();
    return c;
}

void Loader::loadItems()
{
    m_itemlistcount=0;
    if(m_itemcount < 1)
        return;

    size_t itemcount=5;
    if(m_itemcount-m_beginitem < 5)
        itemcount = m_itemcount-m_beginitem;

    FATDirHandle *dir = sdFs->opendir(m_curDir);

    if(settings.ShowAllFiles)
        for(size_t j=0;j<m_beginitem;j++)
            dir->readdir();

    for(size_t i=0;i<itemcount;i++)
    {
        m_Items[i].hasIcon=false;
        
        char *fileName;
        char *ext;
        if(!settings.ShowAllFiles)
        {
            dir->closedir();
            dir = sdFs->opendir(m_curDir);
            for(size_t j=0;j<m_indexs[m_beginitem+i];j++)
                dir->readdir();
        }
        
        fileName=dir->readdir()->d_name;
        if(!fileName)
            break;
        m_itemlistcount++;
        memcpy(m_Items[i].displayName, fileName, 27);
        m_Items[i].fullPath[0]=0;
        strcat(m_Items[i].fullPath, m_curDir);
        strcat(m_Items[i].fullPath, "/");
        strcat(m_Items[i].fullPath, fileName);

        m_Items[i].Size=dir->finfo.fsize;

        if(dir->finfo.fattrib & AM_DIR)
        {
            memcpy(m_Items[i].Icon, Icons::Folder, 24*12);
            m_Items[i].Type=ItemType::Folder;
        }    
        else
        {
            ext=strrchr(fileName,'.')+1;

            if(strcmp(ext,"bin")==0)
            {
                FIL fh;
                FRESULT res = f_open(&fh, m_Items[i].fullPath, FA_READ);
                if(res==FR_OK)
                {
                    uint32_t magic=0;
                    size_t n;
                    f_read(&fh, &magic, 4, &n);
                    if(magic==0X10008000)
                    {
                        memcpy(m_Items[i].Icon, Icons::Game, 24*12);
                        m_Items[i].Type=ItemType::BinGame;
                    }    
                    else
                        memcpy(m_Items[i].Icon, Icons::File, 24*12);
                    f_close(&fh);
                }

            }
            else if(strcmp(ext,"pop")==0)
            {
                m_Items[i].Type=ItemType::Pop;
                Pop::open(m_Items[i].fullPath);
                if(Pop::findTag(PopTag::TAG_IMG_24X24_4BPP))
                {
                    size_t count=0;
                    f_read(Pop::popFile, m_Items[i].Icon, 12*24, &count);
                    m_Items[i].hasIcon=true;
                }
                else
                    memcpy(m_Items[i].Icon, Icons::Game, 24*12);
                Pop::close();
            }else if(strcmp(ext,"gb")==0)
            {
                memcpy(m_Items[i].Icon, Icons::GameBoy, 24*12);
                m_Items[i].Type=ItemType::Gameboy;
            }
            else
            {
                memcpy(m_Items[i].Icon, Icons::File, 24*12);
                m_Items[i].Type=ItemType::File;
            }
        }
            
    }
    dir->closedir();
}

void Loader::drawItem(size_t num)
{
    num%=5;

    if(m_state==State::Coverflow)
    {
        drawItemCoverflow(num);
        return;
    }

    drawHollowRoundRect(2, 13+num*30, 24, 24, popPalette[(m_selecteditem==num)?LIGHT_BLUE:WHITE]);
    LCD::fillRectangle(2+27, 13+num*30, LCD::WIDTH-8-27-4, 28, popPalette[(m_selecteditem==num)?LIGHT_BLUE:WHITE]);
    drawRoundRect(LCD::WIDTH-8-4, 13+num*30, 4, 28, popPalette[(m_selecteditem==num)?LIGHT_BLUE:WHITE]);
    
    LCD::cursorX=35;
    LCD::cursorY=13+ num*30+4;
    if(m_selecteditem==num)
        LCD::color=popPalette[Colors::WHITE];
    else
        LCD::color=popPalette[Colors::BLACK];
    LCD::print(m_Items[num].displayName);

    if(m_Items[num].Type!=ItemType::Folder)
    {
        if(m_selecteditem==num)
            LCD::color=popPalette[Colors::YELLOW];
        else
            LCD::color=popPalette[Colors::PURPLE];
        LCD::cursorX=LCD::WIDTH-70;
        LCD::cursorY=13+ num*30+18;
        LCD::printNumber(m_Items[num].Size/1024);
        LCD::print(".");
        LCD::printNumber((m_Items[num].Size%1024)/102);
        LCD::print(" KB");       
    }
    if(m_Items[num].hasIcon)
        LCD::drawBitmap16(5, 15+num*30, 24, 24,  m_Items[num].Icon, popPalette, 0xFF, 0);
    else
        LCD::drawBitmap16(5, 15+num*30, 24, 24,  m_Items[num].Icon, popPalette, Colors::PINK, (m_selecteditem==num)?LIGHT_BLUE:WHITE);

}

void Loader::drawAllItems()
{
    if(m_state==State::Coverflow)
        drawAllItemsCoverflow();
    else
    {
        for(size_t i=0;i<5;i++)
            LCD::fillRectangle(2,11+i*30, LCD::WIDTH-8, 2, popPalette[Colors::DARK_GRAY]);

        for(size_t i=0;i<m_itemlistcount;i++)
            drawItem(i);
        for(size_t i=m_itemlistcount;i<5;i++)
            LCD::fillRectangle(2,13+i*30, LCD::WIDTH-8, 28, popPalette[Colors::DARK_GRAY]);
    }
    if(m_itemcount==0)
    {
        LCD::color=popPalette[Colors::LIGHT_GRAY];
        LCD::cursorX = 46;
        LCD::cursorY = m_state==State::Coverflow?140:20;
        LCD::print("This Folder Is Empty");

    }
}

void Loader::drawItemCoverflow(size_t num)
{
    if(m_Items[num].Type==ItemType::Pop)
        Pop::open(m_Items[num].fullPath);

    if(m_selecteditem==num)
    {
        LCD::fillRectangle(0, 95, LCD::WIDTH, 28, popPalette[Colors::DARK_GRAY]);
        //LCD::fillRectangle(5+200+5, 15, 5, 80, popPalette[Colors::DARK_GRAY]);
        //LCD::fillRectangle(5, 15, 5, 80, popPalette[Colors::DARK_GRAY]);

        LCD::cursorY=98;
        LCD::color=popPalette[Colors::WHITE];

        if(m_Items[num].Type==ItemType::Pop)
        {
            char *name = Pop::getAString(PopTag::TAG_LONGNAME);
            LCD::cursorX=(LCD::WIDTH-strlen(name)*6)/2;
            if(name)
            {
                LCD::print(name);
                delete name;
            }
            else
            {
                LCD::cursorX=(LCD::WIDTH-strlen(m_Items[num].displayName)*6)/2;
                LCD::print(m_Items[num].displayName);
            }

            LCD::color=popPalette[Colors::YELLOW];
            name = Pop::getAString(PopTag::TAG_AUTHOR);
            
            LCD::cursorY=110;

            if(name)
            {
                LCD::cursorX=(LCD::WIDTH-(strlen(name)+3)*6)/2;
                LCD::print("by ");
                LCD::print(name);
                delete name;
            }      
        }
        else
        {
            LCD::cursorX=(LCD::WIDTH-strlen(m_Items[num].displayName)*6)/2;
            LCD::print(m_Items[num].displayName);
            
            if(!m_drawDefaultBanner(10, 15, m_Items[num].Type))
                LCD::fillRectangle(10, 15, 200, 80, popPalette[Colors::LIGHT_GRAY]);

        }

    }
    LCD::color=popPalette[Colors::LIGHT_GRAY];

    drawHollowRoundRect(2+num*43, 121, 38, 38, popPalette[m_selecteditem==num?Colors::LIGHT_BLUE:Colors::DARK_GRAY]);
    //LCD::drawRectangle(5+num*43, 123, 38, 38, popPalette[Colors::BLACK]);
    //LCD::drawRectangle(9, 14, 202, 82, popPalette[Colors::BLACK]);
    LCD::drawRectangle(5+num*43, 123, 38, 38, popPalette[Colors::BLACK]);
    if(m_Items[num].Type==ItemType::Pop)
    {
        
        if(!Pop::drawIcon36(6+num*43, 124))
        {
            LCD::fillRectangle(6+num*43, 124, 36, 36, popPalette[Colors::WHITE]);
            LCD::drawBitmap16(6+num*43+6, 124+6, 24, 24,  m_Items[num].Icon, popPalette, Colors::PINK, Colors::WHITE);
        }
        
        if(m_selecteditem==num)
        {
            if(!Pop::drawBanner(10, 15))
                LCD::fillRectangle(10, 15, 200, 80, popPalette[Colors::LIGHT_GRAY]);
            
            LCD::color=popPalette[Colors::BLACK];
            char *name = Pop::getAString(PopTag::TAG_VERSION);
            LCD::cursorY=85;
            if(name)
            {
                drawRoundRect(LCD::WIDTH-25-strlen(name)*6, 83, 10+strlen(name)*6, 10, popPalette[Colors::WHITE]);
                LCD::cursorX=LCD::WIDTH-20-strlen(name)*6;
                LCD::print(name);
                delete name;
            }
        }
    }
    else
    {
        LCD::fillRectangle(6+num*43, 124, 36, 36, popPalette[Colors::WHITE]);
        LCD::drawBitmap16(6+num*43+6, 124+1, 24, 24,  m_Items[num].Icon, popPalette, Colors::PINK, Colors::WHITE);
        
        LCD::cursorX = 6+num*43+1;
        LCD::cursorY = 151;
        for(auto i=0;i<5;i++)
        {
            if(m_Items[num].displayName[i]==0 || m_Items[num].displayName[i]=='.')
                break;

            LCD::write(m_Items[num].displayName[i]);
            if(i==4 && m_Items[num].displayName[i+1]!=0)
                LCD::write('.');
        }
        
    }
    if(m_Items[num].Type==ItemType::Pop)
        Pop::close();
}

void Loader::drawAllItemsCoverflow()
{
    for(size_t i=m_itemlistcount;i<5;i++)
        drawRoundRect(2+i*43, 120, 44, 44 ,popPalette[Colors::DARK_GRAY]);
    if(m_itemlistcount==0)
        return;
    for(size_t i=0;i<m_itemlistcount;i++)
    {
        if(i==m_selecteditem)
            continue;
        drawItemCoverflow(i);
    }
    drawItemCoverflow(m_selecteditem);
        
}

void Loader::infoPopup()
{
    //drawRoundRect(10,26,LCD::WIDTH-20,124,popPalette[CAYAN]);
    //LCD::fillRectangle(12, 26+2, LCD::WIDTH-24, 120, popPalette[WHITE]);

    drawRoundRect(5,18,LCD::WIDTH-10,140,popPalette[Colors::CAYAN]);
    LCD::fillRectangle(7, 18+2, LCD::WIDTH-14, 140-4, popPalette[WHITE]);

    LCD::color=popPalette[Colors::BLACK];
    LCD::cursorX=50;
    LCD::cursorY=25+30;
    LCD::printNumber(m_Items[m_selecteditem].Size/1024);
    LCD::write('.');
    LCD::printNumber((m_Items[m_selecteditem].Size%1024)/102);
    LCD::print(" KB");

    size_t but_offset=0;

    if(m_Items[m_selecteditem].Type==ItemType::Pop)
    {
        Pop::open(m_Items[m_selecteditem].fullPath);
        if(!Pop::drawIcon36(10, 25))
            LCD::drawBitmap16(15, 30, 24, 24,  m_Items[m_selecteditem].Icon, popPalette, PINK, WHITE);
        else
            LCD::drawRectangle(9, 24, 38, 38, popPalette[Colors::BLACK]);

        char *longName=Pop::getAString(PopTag::TAG_LONGNAME);
        char *author=Pop::getAString(PopTag::TAG_AUTHOR);
        char *desc=Pop::getAString(PopTag::TAG_DESCRIPTION);
        char *version=Pop::getAString(PopTag::TAG_VERSION);

        LCD::color=popPalette[Colors::BLACK];
        LCD::cursorX=50;
        LCD::cursorY=25;

        if(longName)
        {
            if(strlen(longName) > 26)
                longName[26]=0;

            LCD::print(longName);
            delete longName;
        }
        else
           LCD::print(m_Items[m_selecteditem].displayName);
        
            
        if(author)
        {
            if(strlen(author) > 23)
                author[23]=0;
            LCD::color=popPalette[Colors::PURPLE];
            LCD::cursorX=50;
            LCD::cursorY=25+10;
            LCD::print("by ");
            LCD::print(author);
            delete author;
        }

        if(version)
        {
            LCD::color=popPalette[Colors::PURPLE];
            LCD::cursorX=50;
            LCD::cursorY=25+20;
            LCD::print("Version: ");
            LCD::print(version);
            delete version;
        }

        if(desc)
        {
            LCD::color=popPalette[Colors::BLACK];
            LCD::cursorX=10;
            LCD::cursorY=65;
            LCD::printWraped(10, LCD::WIDTH-20, desc);
            delete desc;
        }
        
        Pop::close();

        drawRoundRect(120,130+10,85,15,popPalette[BLUE]);
        LCD::color=popPalette[Colors::WHITE];
        LCD::cursorX=125;
        LCD::cursorY=133+10;
        LCD::print("C:Screenshots");
        
    }
    else
    {
        but_offset=50;
        LCD::drawBitmap16(15, 30, 24, 24,  m_Items[m_selecteditem].Icon, popPalette, PINK, WHITE);
        LCD::color=popPalette[Colors::BLACK];
        LCD::cursorX=50;
        LCD::cursorY=27;
        LCD::print(m_Items[m_selecteditem].displayName);

        if(m_Items[m_selecteditem].Type==ItemType::Gameboy)
        {
            FIL GBFile;
            FRESULT res=f_open(&GBFile, m_Items[m_selecteditem].fullPath, FA_READ);
            if(res==FR_OK)
            {
                f_lseek(&GBFile, 0x134);
                char title[16+1];
                size_t count;
                f_read(&GBFile, title, 16, &count);
                title[16]=0;
                LCD::cursorX=15;
                LCD::cursorY=80;
                LCD::print("Title: ");
                LCD::print(title);
                f_close(&GBFile);
            }
        }

    }
    drawRoundRect(15+but_offset,130+10,45,15,popPalette[GREEN]);
    LCD::color=popPalette[Colors::WHITE];
    LCD::cursorX=20+but_offset;
    LCD::cursorY=133+10;
    LCD::print("A:Load");

    drawRoundRect(65+but_offset,130+10,50,15,popPalette[RED]);
    LCD::color=popPalette[Colors::WHITE];
    LCD::cursorX=70+but_offset;
    LCD::cursorY=133+10;
    LCD::print("B:Close");
    m_oldstate=m_state;
    m_state = State::InfoPopup;
}

void Loader::errorPopup(const char *txt)
{
    drawRoundRect(10,26,LCD::WIDTH-20,124,popPalette[Colors::RED]);
    LCD::fillRectangle(12, 26+2, LCD::WIDTH-24, 120, popPalette[WHITE]);

    LCD::color=popPalette[Colors::RED];
    LCD::cursorX=95;
    LCD::cursorY=34;
    LCD::print("Error");
    LCD::fillRectangle(20, 48,LCD::WIDTH-40, 1, popPalette[Colors::RED]);

    LCD::color=popPalette[Colors::BLACK];
    LCD::cursorX=15;
    LCD::cursorY=55;
    LCD::printWraped(15, LCD::WIDTH-30,txt);

    drawRoundRect(85,130,50,15,popPalette[RED]);
    LCD::color=popPalette[Colors::WHITE];
    LCD::cursorX=90;
    LCD::cursorY=133;
    LCD::print("B:Close");
    m_oldstate=m_state;
    m_state=State::ErrorPopup;
}

bool Loader::loadFile()
{
    drawLoadScreen();
    
    FIL *loadFile;
 
    size_t fsize=0;
    uint32_t counter=0;
    uint8_t data[0x100];

    // gameboy stuffs
    uint32_t *u32;
    bool magicfound=false;
    FIL GBFile;
    int gbsize=0;
    bool border=false;
    char * paletteFile=nullptr;

    size_t bytes_left_count=0;
    uint8_t bytes_left[64];

    bool noBanner=false;
    if(m_Items[m_selecteditem].Type==ItemType::Pop)
    {
        Pop::open(m_Items[m_selecteditem].fullPath);
        if(!Pop::popFile)
            return false;
            
        loadFile=Pop::popFile;

        if(Pop::findTag(PopTag::TAG_IMG_200x80_565))
            Pop::drawBanner(10, 42);
        else
        {
            noBanner=true;
            //if(!Pop::drawIcon36(92, 70))
            //    LCD::drawBitmap16(98, 76, 24, 24, m_Items[m_selecteditem].Icon, popPalette, 0xFF, WHITE);
        }
        
        if(!Pop::findTag(PopTag::TAG_CODE))
            return false;

        fsize = loadFile->fsize;
        uint32_t pos=loadFile->fptr;

        f_lseek(loadFile, pos-8);
        fsize=fsize-(pos-8);
    }
    else if(m_Items[m_selecteditem].Type==ItemType::Gameboy)
    {
        FRESULT res1 =f_open(&GBFile, m_Items[m_selecteditem].fullPath, FA_READ);
        if(res1!=FR_OK)
            return false;

        gbsize = GBFile.fsize;
        if(gbsize > 1024*128)
            return false;

        f_lseek(&GBFile, 0x147);
        uint8_t mapperId;
        size_t n;
        f_read(&GBFile,&mapperId,1,&n);
        if( mapperId == 6 || mapperId == 3 )
            mapperId = 1;

        if(mapperId > 1)
            return false;

        f_lseek(&GBFile, 0);
        char Emu[128];
        memset(Emu,0,128);
        strcat(Emu, ".loader/GameBoy/Bin/");
        if(mapperId==0)
            strcat(Emu, "mbc0");
        else if(mapperId==1)
            strcat(Emu, "mbc1");

        const char *entries[]={"Expand to Fit", "Pixel Perfect 1:1"};
        size_t ret=Settings::popupMenu(entries, 2);
        if(ret > 1)
            ret=0;

        border=ret;

        if(ret==0)
            strcat(Emu, "s.bin");
        else if(ret==1)
            strcat(Emu, ".bin");
        drawLoadScreen();
        paletteFile=FileMenu(".loader/GameBoy/Palettes");

        drawLoadScreen();

        loadFile=new FIL;
        FRESULT res = f_open(loadFile, Emu, FA_READ);
        if(res!=FR_OK)
            return false;
        fsize = loadFile->fsize;
        u32=reinterpret_cast<uint32_t*>(data);
    }
    else if(m_Items[m_selecteditem].Type==ItemType::BinGame)
    {
        loadFile=new FIL;
        FRESULT res = f_open(loadFile, m_Items[m_selecteditem].fullPath, FA_READ);
        if(res!=FR_OK)
            return false;
        fsize = loadFile->fsize;
    }
    
    m_drawDefaultBanner(10, 42, noBanner?ItemType::BinGame:m_Items[m_selecteditem].Type);

    
    while (1)
    {
        if (counter >= fsize)
            break;

        if(magicfound)
        {
            if(gbsize < 0x100)
            {
                f_lseek(loadFile, gbsize+loadFile->fptr);
                size_t n;
                f_read(loadFile, data+gbsize, 0x100-gbsize, &n);
            }
            else
                f_lseek(loadFile, loadFile->fptr+0x100);
            
            size_t n;
            f_read(&GBFile, data, 0x100, &n);
            gbsize-=n;
            if(gbsize <= 0)
                magicfound=false;
            
        }
        else
        {
            size_t n;
            f_read(loadFile, data, 0x100, &n);
            if(n < 0x100)
            {
                if(fsize-counter > 0x100) 
                    return false;
            }
        }

        if(m_Items[m_selecteditem].Type==ItemType::Gameboy && !magicfound)
        {
            if(bytes_left_count > 0)
            {
                memcpy(data, bytes_left, bytes_left_count);
                bytes_left_count=0;
            }
            for(size_t i=0;i<64;i++)
            {
                if(u32[i]==GBMAGIC)
                {
                    uint8_t n=data[(i+1)<<2];
                    if(i==63)
                    {
                        size_t count;
                        f_read(loadFile, &n, 1, &count);
                        f_lseek(loadFile, loadFile->fptr-1);
                    }
                    if(n==0x01)
                    {
                        magicfound=true;
                        size_t count;
                        f_read(&GBFile, data+(i<<2), 0x100-(i<<2), &count);
                        gbsize-=count;
                    }

                    if(n==0x02)
                    {
                        if(paletteFile)
                        {
                            uint8_t gbpal[64];
                            FIL palFile;
                            FRESULT res = f_open(&palFile, paletteFile, FA_READ);
                            if(res)
                                return false;
                                
                            size_t n;
                            f_read(&palFile, gbpal, 64, &n);
                            f_close(&palFile);
                            //palFile->close();
                            if(0x100-(i<<2) < 64)
                            {
                                bytes_left_count=64-(0x100-(i<<2));
                                memcpy(gbpal+(64-bytes_left_count), bytes_left, bytes_left_count);
                            }

                            memcpy(data+(i<<2), gbpal, 64-bytes_left_count);
                        }
                        else
                        {
                            for(auto j=0;j<4;j++)
                                memcpy(data+(i<<2)+16*j, gbDefaultPal, 16);
                        }
                        
                    }
                    if(border)
                    {
                        if(n==0x07)
                            memset(data+(i<<2), 0, 5);
                    }
                }
            }

        }

        if (counter < 0x39000 || counter >= 0x3F000){ //write also bytes ABOVE the bootloader, again because of mbed
            if (CopyPageToFlash(counter, data)) {
                return false;
           }
            else
                counter += 0x100;
        }
        else
            counter += 0x100;

        drawRoundRect(10 ,110+13 ,((LCD::WIDTH-20)*counter)/fsize, 20, popPalette[GREEN]);
        LCD::fillRectangle(10, 133+13, 40, 8, popPalette[Colors::WHITE]);
        LCD::cursorX=10;
        LCD::cursorY=133+13;
        LCD::printNumber((100*counter)/fsize);
        LCD::print(" %");
    }

    SCB->AIRCR = 0x05FA0004; //issue system reset
    //NVIC_SystemReset();
    return true;
}

void Loader::drawLoadScreen()
{
    drawRoundRect(5,18,LCD::WIDTH-10,140,popPalette[Colors::GREEN]);
    LCD::fillRectangle(7, 18+2, LCD::WIDTH-14, 140-4, popPalette[WHITE]);

    LCD::color=popPalette[Colors::BLACK];
    LCD::cursorX=80;
    LCD::cursorY=25;
    LCD::print("Loading...");
    LCD::fillRectangle(20, 39,LCD::WIDTH-40, 1, popPalette[Colors::GREEN]);
    LCD::cursorX=(LCD::WIDTH-strlen(m_Items[m_selecteditem].displayName)*6)/2;
    LCD::cursorY=46;
    LCD::print(m_Items[m_selecteditem].displayName);

    drawRoundRect(10 ,110+13 ,LCD::WIDTH-20, 20, popPalette[LIGHT_GRAY]);

    if(m_Items[m_selecteditem].Type==ItemType::Gameboy)
        LCD::drawBitmap16(98, 76, 24, 24, Icons::GameBoy, popPalette, PINK, WHITE);
    else if(m_Items[m_selecteditem].Type==ItemType::BinGame)
        LCD::drawBitmap16(98, 76, 24, 24, Icons::Game, popPalette, PINK, WHITE);

}

void Loader::m_pushPrev()
{
    m_prevSelecteditem[m_pPrev]=m_selecteditem;
    m_prevBeginitem[m_pPrev]=m_beginitem;
    if(m_pPrev < 8)
        m_pPrev++;
}

void Loader::m_pullPrev()
{
    if(m_pPrev > 0)
        m_pPrev--;
    
    m_selecteditem=m_prevSelecteditem[m_pPrev];
    m_beginitem=m_prevBeginitem[m_pPrev];
}

char* Loader::FileMenu(const char *Folder)
{
    FATDirHandle *dir = sdFs->opendir(Folder);
    if(!dir)
        return 0;
    char* filemenu[15];
    size_t fcount=0;
    for(auto i=0;i<15;)
    {
        char *fileName=dir->readdir()->d_name;

        if(!fileName)
            break;

        if(dir->finfo.fattrib & AM_ARC)
        {
            filemenu[i] = new char[128];
            filemenu[i][0]=0;
            strcpy(filemenu[i], fileName);
            fcount++;
            i++;
        }

    }
    dir->closedir();
    if(fcount==0)
        return 0;

    size_t ret=Settings::popupMenu(const_cast<const char**>(filemenu), fcount);
    if(ret < fcount)
    {
        char *path=new char[128];
        path[0]=0;
        strcat(path, Folder);
        strcat(path, "/");
        strcat(path, filemenu[ret]);

        for(size_t i=0; i<fcount ; i++)
        {
            delete filemenu[i];
        }

        return path;
    }
    return 0;
}

void Loader::m_nextItem()
{
    if(m_itemcount < 1)
        return;
    if(m_selecteditem+m_beginitem >= m_itemcount-1)
    {
        m_selecteditem=0;
        m_beginitem=0;
        m_reloadItems();
        return;
    }

    m_selecteditem++;

    if(m_selecteditem == 5)
    {
        m_selecteditem=0;
        m_beginitem+=5;
        m_reloadItems();
    }
    else
    {
        drawBottomBar();

        drawItem(m_selecteditem-1);
        drawItem(m_selecteditem);

    }
}
void Loader::m_prevItem()
{
    if(m_itemcount < 1)
        return;
    if(m_selecteditem==0)
    {
        if(m_beginitem >= 5)
        {
            m_selecteditem=4;
            m_beginitem-=5;    
        }
        else
        {
            m_beginitem=((m_itemcount-1)/5)*5;
            m_selecteditem=(m_itemcount-1)%5;
        }
        m_reloadItems();
    }
    else
    {
        m_selecteditem--;

        drawItem(m_selecteditem+1);
        drawItem(m_selecteditem);
        drawBottomBar();
    }
}
void Loader::m_nextPage()
{
    if(m_itemcount < 5)
        return;
    if(m_beginitem < m_itemcount-5 )
        m_beginitem+=5;
    else
        m_beginitem=0;
    m_selecteditem=0;
    m_reloadItems();
}
void Loader::m_prevPage()
{
    if(m_itemcount < 5)
        return;
    if(m_beginitem > 0)
        m_beginitem -= 5;    
    else
        m_beginitem = ((m_itemcount-1)/5)*5;
    m_selecteditem=0;
    m_reloadItems(); 
}

void Loader::m_reloadItems()
{
    loadItems();
    drawAllItems();
    drawScrollBar();
    drawBottomBar(); 
}

bool Loader::m_drawDefaultBanner(uint16_t x, uint16_t y, ItemType Type)
{
    FIL BNFile;

    FRESULT res=FRESULT::FR_NO_PATH;
    if(Type==ItemType::Gameboy)
        res=f_open(&BNFile, ".loader/GameBoy/banner", FA_READ);
    if(Type==ItemType::BinGame)
        res=f_open(&BNFile, ".loader/banner", FA_READ);

    if(res==FR_OK)
    {
        LCD::drawBitmap565File(&BNFile, x, y, 200, 80);
        f_close(&BNFile);
        return true;
    }
    return false;
}