#include "Loader.h"
#include "LCD.h"
//#include "Palette.h"
#include <cctype>
#include <cstring>
//#include "Icons.h"
#include "Pop.h"
#include "iap.h"
#include "Battery.h"

bool Loader::m_redraw=true;
SDFileSystem *Loader::sdFs;
char Loader::m_curDir[_MAX_LFN+1];
std::size_t Loader::m_itemcount;
std::size_t Loader::m_itemlistcount;
std::size_t Loader::m_beginitem;
std::size_t Loader::m_selecteditem;
Item Loader::m_Items[5];

uint16_t Loader::m_indexs[MAX_FILES];

std::size_t Loader::m_prevSelecteditem[10];
std::size_t Loader::m_prevBeginitem[10];
uint8_t Loader::m_pPrev=0;

Loader::State Loader::m_state = Loader::State::Browse;
Loader::State Loader::m_oldstate = Loader::State::Browse;

time_t Loader::m_seconds;

const uint8_t Loader::gbDefaultPal[16]={0xF0, 0xBD, 0x07, 0x00, 0xA8, 0x6B, 0x05, 0x00, 0x60, 0x18, 0x03, 0x00, 0x18, 0xC6, 0x00, 0x00};

Theme Loader::theme;
Settings Loader::settings;

void Loader::Init()
{
    //fix sound problem
    LPC_GPIO_PORT->DIR[1] |= (1 << 17);
    LPC_GPIO_PORT->CLR[1] = (1 << 17);

    LPC_IOCON->PIO0_0 &= ~(0x3); //set reset pin function back to 0 (RESET)

    bool bootsplash=false;

    LCD::Init();
    Buttons::Init();
    Battery::init();

    char* arg = reinterpret_cast<char*>(0x20000000);

    sdFs = new SDFileSystem(); // P0_9, P0_8, P0_6, P0_7, 0, NC, SDFileSystem::SWITCH_NONE, 25000000 );

    if(sdFs->card_type()==SDFileSystem::CardType::CARD_UNKNOWN)
    {
        //delete sdFs;
        //sdFs=0;
        errorPopup("SD Init Faild");
        while(true)
        {};
    }

    settings.Init();
    LCD::LoadFont("kraken/5x7.font", FontSize::Big);

    m_redraw=true;
    memset(m_curDir,0,_MAX_LFN);
 

    *(arg+4)=0;
    if(strcmp(arg,"Load")==0)
    {
        strcpy(m_Items[0].fullPath, arg+5);
        char *fileName;

        fileName=strrchr(arg+5,'/')+1;
        memcpy(m_Items[0].displayName, fileName, 27);
        memset(arg, 0, 5);

        char ext[4];
        m_getExtension(ext,fileName);

        if(strcmp(ext,"pop")==0)
            m_Items[0].Type=ItemType::Pop;
        else if(strcmp(ext,"gb")==0)
            m_Items[0].Type=ItemType::Gameboy;
        else if(strcmp(ext,"bin")==0)
            m_Items[0].Type=ItemType::BinGame;

        if(!loadFile())
            errorPopup("Loading File Failed");
    }


    if(m_drawBootSplash())
    {
        LCD::fillRectangle(0, LCD::HEIGHT-3, LCD::WIDTH, 3, 0x0000);
        bootsplash=true;
    }

    LCD::LoadFont("kraken/3x5.font", FontSize::Small);
    if(!theme.loadTheme(settings.theme))
        errorPopup("Loading theme");

    
    m_itemcount=m_fileCount();
    m_beginitem=0;
    m_selecteditem=0;
    loadItems();

    if(settings.viewmode==Settings::ViewMode::Icons)
        m_state=State::Coverflow;
    else if(settings.viewmode==Settings::ViewMode::List)
        m_state=State::Browse;



    if(bootsplash)
    {
        for(std::size_t i=0;i<0x17FF;i++)
            LCD::fillRectangle(0, LCD::HEIGHT-3, (LCD::WIDTH*i)/0x17FF, 3, 0xFFFF);
    }
            

}

void Loader::run()
{
    while(true)
    {
        update();
        draw();
    }
}

void Loader::update()
{
    time_t tmp_seconds = (time_t)LPC_RTC->COUNT;
    if(tmp_seconds!=m_seconds)
    {
        if((tmp_seconds%5)==0)
            drawClock();
    }
        
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
            m_redraw=true;
            draw();
        }
        return;
    }

    if(m_state==State::InfoPopup)
    {
        if(Buttons::pressed(Buttons::BTN_B))
        {
            LCD::fillRectangle(5+200+5, 15, 5, 80, theme.BackgroundTop);
            LCD::fillRectangle(5+200+5, 15+80, 5, 65, theme.BackgroundTop);
            LCD::fillRectangle(5, 14, 5, 82, theme.BackgroundTop);
            //LCD::drawRectangle(9, 14, 202, 82, theme.Banner);
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
            std::size_t num=0;
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
        else if(m_state==State::Browse && m_itemcount > 5)
            m_nextPage();
        return;
    }

    if(Buttons::repeat(Buttons::BTN_LEFT, 10000))
    {
        if(m_state==State::Coverflow)
            m_prevItem();
        else if(m_state==State::Browse && m_itemcount > 5 )
            m_prevPage();
        return;
    }

    if(Buttons::repeat(Buttons::BTN_UP, 10000))
    {
        if(m_state==State::Coverflow)
            m_prevPage();
        else if(m_state==State::Browse)
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
            drawBottomBar(true);
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
        drawBottomBar(true);
        m_itemcount=m_fileCount();

        m_pullPrev();
        drawTopBar();
        m_reloadItems(); 
    }

    if(Buttons::pressed(Buttons::BTN_C))
    {
        if(m_curDir[0])
            return;
        settings.Init();
        m_oldstate=m_state;
        m_state=State::Settings;
        settings.draw();
        drawClock();
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

    if(m_state==State::Browse)
        LCD::Clear(theme.BackgroundTop);
    else
    {
        LCD::fillRectangle(0, 0, LCD::WIDTH, LCD::HEIGHT-70, theme.BackgroundTop);
        LCD::fillRectangle(0, LCD::HEIGHT-70, LCD::WIDTH, 70,  theme.BackgroundBottom);
    }

    drawTopBar();
    drawScrollBar();
    drawClock();

    drawAllItems();

    drawBottomBar();
    
    m_redraw=false;
}

void Loader::drawTopBar()
{
    std::size_t len=strlen(m_curDir);

    drawRoundRect(2, -2, LCD::WIDTH-4, 12, theme.Boxes);
    LCD::fillRectangle(4, 10, LCD::WIDTH-8, 1, theme.Boxes+0x2965);
    LCD::selectedFont=FontSize::Big;
    drawClock();

    LCD::selectedFont=FontSize::Small;
    LCD::cursorX=8;
    LCD::cursorY=3;
    LCD::color=theme.Text;

    if(len > 32)
    {
        LCD::print("..");
        LCD::print(m_curDir+len-32);
    }  
    else
        LCD::print(m_curDir);
    LCD::write('/');
    LCD::selectedFont=FontSize::Big;
}

void Loader::drawScrollBar()
{
    if(m_state==State::Coverflow)
        return;
    LCD::fillRectangle(LCD::WIDTH-4, 15, 2, 145, theme.Boxes);
    if(m_itemcount > 5)
    {
        std::size_t div=145/((m_itemcount+4)/5);
        std::size_t div10=1450/((m_itemcount+4)/5);

        std::size_t start=15+(div*(m_beginitem/5))+(div10*(m_beginitem/5))/100;
        if(start > 145+15-div)
            start=145+15-div;

        LCD::fillRectangle(LCD::WIDTH-4, start , 2, div, theme.Selected);
    }
}

void Loader::drawBottomBar(bool loading)
{
    LCD::selectedFont=FontSize::Small;
    drawRoundRect(2, LCD::HEIGHT-9, LCD::WIDTH-4, 11, theme.Boxes);
    LCD::fillRectangle(4, LCD::HEIGHT-9, LCD::WIDTH-8, 1, theme.Boxes+0x2965);
    
    LCD::color=theme.Text;
    LCD::cursorY=LCD::HEIGHT-7;
    if(loading)
    {
        LCD::cursorX=8;
        LCD::print("Loading...");
        return;
    }

    LCD::cursorX=LCD::WIDTH-(m_itemcount>10?50:40);

    LCD::print("\x11 ");
    LCD::printNumber((m_itemcount > 0)?m_beginitem+m_selecteditem+1:0);
    LCD::print("/");
    LCD::printNumber(m_itemcount);
    LCD::print(" \x10");

    LCD::cursorX=16;
    LCD::print("Ok");
    char button;
    if(!m_curDir[0])
    {
        LCD::cursorX=35;
        LCD::print("Settings");
        button='\x17';
    }
    else
    {
        LCD::cursorX=35;
        LCD::print("Back");
        button='\x16';
    }
    LCD::selectedFont=FontSize::Big;
    LCD::cursorY=LCD::HEIGHT-8;
    LCD::cursorX=8;
    LCD::write('\x15');
    LCD::cursorX=27;
    LCD::write(button);
}

void Loader::drawClock()
{
    //drawRoundRect(LCD::WIDTH-35-30, 1, 33+30, 10, theme.Boxes);
    //LCD::fillRectangle(LCD::WIDTH-33-30, 10, 33+30-4, 1, theme.Boxes+0x2965);

    LCD::fillRectangle(LCD::WIDTH-35-30+1, 2, 33+30-2, 7, theme.Boxes);

    m_seconds = static_cast<time_t>(LPC_RTC->COUNT);
    auto hours = (m_seconds/3600)%24;
    auto mins = (m_seconds/60)%60;

    LCD::cursorX=LCD::WIDTH-33;
    LCD::cursorY=2;
    LCD::color=theme.Text;

    if(hours < 10)
        LCD::write('0');
    LCD::printNumber(hours);
    LCD::write(':');
    if(mins < 10)
        LCD::write('0');
    LCD::printNumber(mins);

    uint8_t bat=Battery::getPercentage();
    LCD::cursorX=bat<100?(LCD::WIDTH-33-26):(LCD::WIDTH-33-30);
    LCD::cursorY=2;
    LCD::printNumber(bat);
    LCD::write('\x07');

}

std::size_t Loader::m_fileCount()
{
    std::size_t c=0;
    uint16_t idx=0;

    static FATFS_DIR dir;
    static FILINFO finfo;
    FRESULT res=f_opendir(&dir, m_curDir);

    if(res!=FR_OK)
        return 0;
    
    if(settings.ShowAllFiles)
    {
        while(true)
        {
            res=f_readdir(&dir, &finfo);
            if(res!=FR_OK || finfo.fname[0]==0)
                break;
            c++;
        }
        return c;
    }

    char nameBuffer[_MAX_LFN+1];
    finfo.lfname=nameBuffer;
    finfo.lfsize=_MAX_LFN;

    while(c < MAX_FILES)
    {
        char *fileName=0;
        res=f_readdir(&dir, &finfo);
        if(res!=FR_OK || finfo.fname[0]==0)
            break;
        
        fileName=finfo.lfname[0]!=0?finfo.lfname:finfo.fname;

        if(finfo.fattrib & AM_DIR)
        {
            if(strcmp(fileName,"kraken")!=0 && fileName[0]!='.')
            {
                if(m_scanFolderForGames(m_curDir, fileName))
                    m_indexs[c++]=idx;
            }
        }
        else
        {
            char extension[4];
            m_getExtension(extension,fileName);

            if(strcmp(extension,"pop")==0 || strcmp(extension,"gb")==0)
                m_indexs[c++]=idx;

            else if(strcmp(extension,"bin")==0)
            {
                char fPath[_MAX_LFN+1];
                fPath[0]=0;
                strcat(fPath, m_curDir);
                strcat(fPath, "/");
                strcat(fPath, fileName);

                if(m_binIsGame(fPath))
                    m_indexs[c++]=idx;
            }
            
        }
        idx++;
    }
    return c;
}

void Loader::loadItems()
{
    m_itemlistcount=0;
    if(m_itemcount < 1)
        return;

    std::size_t itemcount=5;
    if(m_itemcount-m_beginitem < 5)
        itemcount = m_itemcount-m_beginitem;

    FATFS_DIR dir;
    static FILINFO finfo;
    FRESULT res=f_opendir(&dir, m_curDir);

    char nameBuffer[_MAX_LFN+1];
    finfo.lfname=nameBuffer;
    finfo.lfsize=_MAX_LFN;

    if(res!=FR_OK)
        return;

    if(settings.ShowAllFiles)
        for(std::size_t j=0;j<m_beginitem;j++)
            f_readdir(&dir, &finfo);

    for(std::size_t i=0;i<itemcount;i++)
    {
        m_Items[i].hasIcon=false;
        
        char *fileName=0;
        if(!settings.ShowAllFiles)
        {
            res=f_opendir(&dir, m_curDir);
            for(std::size_t j=0;j<m_indexs[m_beginitem+i];j++)
                f_readdir(&dir, &finfo);
        }
        
        res = f_readdir(&dir, &finfo);
        if(res!=FR_OK || finfo.fname[0]==0)
            break;

        if(finfo.lfname[0]!=0)
            fileName=finfo.lfname;
        else
            fileName=finfo.fname;

        m_itemlistcount++;
        memcpy(m_Items[i].displayName, fileName, 27);
        m_Items[i].fullPath[0]=0;
        strcat(m_Items[i].fullPath, m_curDir);
        strcat(m_Items[i].fullPath, "/");
        strcat(m_Items[i].fullPath, fileName);

        m_Items[i].Size=finfo.fsize;

        if(finfo.fattrib & AM_DIR)
            m_Items[i].Type=ItemType::Folder;  
        else
        {
            char ext[4];
            m_getExtension(ext,fileName);

            if(strcmp(ext,"bin")==0)
            {
                if(m_binIsGame(m_Items[i].fullPath))
                    m_Items[i].Type=ItemType::BinGame;
                else
                    m_Items[i].Type=ItemType::File;
            }
            else if(strcmp(ext,"pop")==0)
            {
                m_Items[i].Type=ItemType::Pop;
                Pop::open(m_Items[i].fullPath);
                if(Pop::findTag(PopTag::TAG_IMG_24X24_4BPP))
                    m_Items[i].hasIcon=true;

                Pop::close();
            }
            else if(strcmp(ext,"gb")==0)
                m_Items[i].Type=ItemType::Gameboy;
            else
                m_Items[i].Type=ItemType::File;
        }
            
    }
}

void Loader::drawItem(std::size_t num)
{
    num%=5;

    if(m_state==State::Coverflow)
    {
        drawItemCoverflow(num);
        return;
    }

    drawHollowRoundRect(2, 13+num*30, 24, 24, (m_selecteditem==num)?theme.Selected:theme.Boxes);
    LCD::fillRectangle(2+27, 13+num*30, LCD::WIDTH-8-27-4, 28, (m_selecteditem==num)?theme.Selected:theme.Boxes);
    drawRoundRect(LCD::WIDTH-8-4, 13+num*30, 4, 28, (m_selecteditem==num)?theme.Selected:theme.Boxes);
    
    LCD::cursorX=35;
    LCD::cursorY=13+ num*30+4;
    if(m_selecteditem==num)
        LCD::color=theme.SelectedText;
    else
        LCD::color=theme.Text;

    LCD::print(m_Items[num].displayName);

    if(m_Items[num].Type!=ItemType::Folder)
    {
        if(m_selecteditem==num)
            LCD::color=theme.SelectedText;
        else
            LCD::color=theme.Text2;
        LCD::cursorX=LCD::WIDTH-70;
        LCD::cursorY=13+ num*30+18;
        LCD::printNumber(m_Items[num].Size/1024);
        LCD::print(".");
        LCD::printNumber((m_Items[num].Size%1024)/102);
        LCD::print(" KB");       
    }
    if(m_Items[num].hasIcon)
    {
        Pop::open(m_Items[num].fullPath);
        Pop::drawIcon24(5, 15+num*30);
    }
    else
        LCD::drawBitmap565(5, 15+num*30, 24, 24, theme.icons[static_cast<int>(m_Items[num].Type)]);
}

void Loader::drawAllItems()
{
    if(m_state==State::Coverflow)
        drawAllItemsCoverflow();
    else
    {
        for(std::size_t i=0;i<5;i++)
            LCD::fillRectangle(2,11+i*30, LCD::WIDTH-8, 2, theme.BackgroundTop);

        for(std::size_t i=0;i<m_itemlistcount;i++)
            drawItem(i);
        for(std::size_t i=m_itemlistcount;i<5;i++)
            LCD::fillRectangle(2,13+i*30, LCD::WIDTH-8, 28, theme.BackgroundTop);
    }
    if(m_itemcount==0)
    {
        LCD::color=theme.Text2;
        LCD::cursorX = 46;
        LCD::cursorY = m_state==State::Coverflow?140:20;
        LCD::print("This Folder Is Empty");
    }
}

void Loader::drawItemCoverflow(std::size_t num)
{
    if(m_Items[num].Type==ItemType::Pop)
        Pop::open(m_Items[num].fullPath);

    if(m_selecteditem==num)
    {
        //clear name and author
        LCD::fillRectangle(0, 92+3, LCD::WIDTH, 24, theme.BackgroundTop);

        LCD::cursorY=95+3;
        LCD::color=theme.Title;

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

            LCD::color=theme.Text2;
            name = Pop::getAString(PopTag::TAG_AUTHOR);
            
            LCD::cursorY=105+3+1;

            if(name)
            {
                if(strlen(name)>32)
                {
                    name[30]='.';
                    name[31]='.';
                    name[32]=0;
                }
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
            
            m_drawDefaultBanner(10, 15, m_Items[num].Type);
        }
    }
    LCD::color=theme.Text;

    drawHollowRoundRect(2+num*43, 121+1, 38, 38, m_selecteditem==num?theme.Selected:theme.BackgroundBottom);
    LCD::drawRectangle(5+num*43, 123+1, 38, 38, theme.BackgroundTop);
    //LCD::drawRectangle(4+num*43, 122, 40, 40, theme.Background);

    if(m_Items[num].Type==ItemType::Pop)
    {
        
        if(!Pop::drawIcon36(6+num*43, 124+1))
        {
            LCD::fillRectangle(6+num*43, 124+1, 36, 36, theme.Boxes);
            LCD::drawBitmap565(6+num*43+6, 124+6+1, 24, 24, theme.icons[static_cast<int>(m_Items[num].Type)]);
        }
        
        if(m_selecteditem==num)
        {
            if(!Pop::drawBanner(10, 15))
                m_drawDefaultBanner(10, 15, m_Items[num].Type);
            
            LCD::selectedFont=FontSize::Small;
            LCD::color=theme.Text;
            char *version = Pop::getAString(PopTag::TAG_VERSION);
            LCD::cursorY=84+3;
            if(version)
            {
                drawRoundRect(LCD::WIDTH-25-strlen(version)*4, 83+3, 10+strlen(version)*4, 7, theme.Boxes);
                LCD::cursorX=LCD::WIDTH-20-strlen(version)*4;
                LCD::print(version);
                delete version;
            }
            LCD::selectedFont=FontSize::Big;
        }
    }
    else
    {
        LCD::fillRectangle(6+num*43, 124+1, 36, 36, theme.Boxes);
        LCD::drawBitmap565(6+num*43+6, 124+1+1, 24, 24, theme.icons[static_cast<int>(m_Items[num].Type)]);
        
        LCD::selectedFont=FontSize::Small;

        char name[10];
        name[9]=0;

        for(auto i=0;i<9;i++)
        {
            if(m_Items[num].displayName[i]==0 || m_Items[num].displayName[i]=='.')
            {
                name[i]=0;
                break;
            }

            name[i]=m_Items[num].displayName[i];

            if(i==8 && m_Items[num].displayName[i+1]!=0)
                name[i]='.';
            
        }
        int offset=0;
        if(strlen(name)<9)
            offset=((9-strlen(name))*(LCD::fonts[static_cast<std::uint8_t>(LCD::selectedFont)].width+1))>>1;

        LCD::cursorX = 6+num*43+1+offset;
        LCD::cursorY = 151+1;
        LCD::print(name);
        LCD::selectedFont=FontSize::Big;
    }
    if(m_Items[num].Type==ItemType::Pop)
        Pop::close();
}

void Loader::drawAllItemsCoverflow()
{
    LCD::fillRectangle(0, 92+24+1, LCD::WIDTH, 7, theme.BackgroundBottom);

    for(std::size_t i=m_itemlistcount;i<5;i++)
        drawRoundRect(2+i*43, 120+1, 44, 44 , theme.BackgroundBottom);
    if(m_itemlistcount==0)
        return;
    for(std::size_t i=0;i<m_itemlistcount;i++)
    {
        if(i==m_selecteditem)
            continue;
        drawItemCoverflow(i);
    }
    drawItemCoverflow(m_selecteditem);
        
}

void Loader::infoPopup()
{
    drawRoundRect(5,18,LCD::WIDTH-10,140, theme.BoxBorder);
    LCD::fillRectangle(7, 18+2, LCD::WIDTH-14, 140-4, theme.Boxes);

    std::size_t but_offset=0;

    if(m_Items[m_selecteditem].Type==ItemType::Pop)
    {
        Pop::open(m_Items[m_selecteditem].fullPath);
        if(!Pop::drawIcon36(10, 25))
            LCD::drawBitmap565(15, 30, 24, 24, theme.icons[static_cast<int>(m_Items[m_selecteditem].Type)]);
        else
            LCD::drawRectangle(9, 24, 38, 38, theme.Text);

        char *longName=Pop::getAString(PopTag::TAG_LONGNAME);
        char *author=Pop::getAString(PopTag::TAG_AUTHOR);
        char *desc=Pop::getAString(PopTag::TAG_DESCRIPTION);
        char *version=Pop::getAString(PopTag::TAG_VERSION);

        LCD::color=theme.Text;
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
            LCD::color=theme.Text2;
            LCD::cursorX=50;
            LCD::cursorY=25+10;
            LCD::print("by ");
            LCD::print(author);
            delete author;
        }

        if(version)
        {
            LCD::color=theme.Text2;
            LCD::cursorX=50;
            LCD::cursorY=25+20;
            LCD::print("Version: ");
            LCD::print(version);
            delete version;
        }

        if(desc)
        {
            LCD::color=theme.Text;
            LCD::cursorX=10;
            LCD::cursorY=65;
            LCD::printWraped(10, LCD::WIDTH-20, desc);
            delete desc;
        }

        LCD::color=theme.Text;
        LCD::cursorX=50;
        LCD::cursorY=25+30;

        size_t size=(m_Items[m_selecteditem].Type==ItemType::Pop)?(Pop::seekToCode()):m_Items[m_selecteditem].Size;
        LCD::printNumber(size/1024);
        LCD::write('.');
        LCD::printNumber((size%1024)/102);
        LCD::print(" KB");
            
        Pop::close();

        drawRoundRect(120,130+10,85,15, theme.Button);
        LCD::color=theme.ButtonText;
        LCD::cursorX=125;
        LCD::cursorY=133+10;
        LCD::print("\x17:Screenshots");
        
    }
    else
    {
        but_offset=50;

        LCD::drawBitmap565(15, 30, 24, 24, theme.icons[static_cast<int>(m_Items[m_selecteditem].Type)]);
        LCD::color=theme.Text;
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
                std::size_t count;
                f_read(&GBFile, title, 16, &count);
                title[16]=0;
                LCD::cursorX=50;
                LCD::cursorY=25+20;
                LCD::print("Title: ");
                LCD::print(title);
                f_close(&GBFile);
            }
        }

    }
    drawRoundRect(15+but_offset,130+10,45,15, theme.Button);
    LCD::color=theme.ButtonText;
    LCD::cursorX=20+but_offset;
    LCD::cursorY=133+10;
    LCD::print("\x15:Load");

    drawRoundRect(65+but_offset,130+10,50,15, theme.Button);
    LCD::color=theme.ButtonText;
    LCD::cursorX=70+but_offset;
    LCD::cursorY=133+10;
    LCD::print("\x16:Back");
    m_oldstate=m_state;
    m_state = State::InfoPopup;
}

void Loader::errorPopup(const char *txt)
{
    drawRoundRect(10,26,LCD::WIDTH-20,124, theme.Error);
    LCD::fillRectangle(12, 26+2, LCD::WIDTH-24, 120, theme.Boxes);

    LCD::color=theme.Error;
    LCD::cursorX=95;
    LCD::cursorY=34;
    LCD::print("Error");
    LCD::fillRectangle(20, 48,LCD::WIDTH-40, 1, theme.Error);

    LCD::color=theme.Text;
    LCD::cursorX=15;
    LCD::cursorY=55;
    LCD::printWraped(15, LCD::WIDTH-30,txt);

    drawRoundRect(85,130,50,15, theme.Button);
    LCD::color=theme.ButtonText;
    LCD::cursorX=90;
    LCD::cursorY=133;
    LCD::print("\x16:Close");
    m_oldstate=m_state;
    m_state=State::ErrorPopup;
}

bool Loader::loadFile()
{
    drawLoadScreen();
    
    FIL *loadFile=0;
 
    std::size_t fsize=0;
    uint32_t counter=0;
    uint8_t data[0x100];

    // gameboy stuffs
    uint32_t *u32;
    bool magicfound=false;
    FIL GBFile;
    int gbsize=0;
    bool border=false;
    char * paletteFile=nullptr;

    std::size_t bytes_left_count=0;
    uint8_t bytes_left[64];

    bool noBanner=true;
    if(m_Items[m_selecteditem].Type==ItemType::Pop)
    {
        Pop::open(m_Items[m_selecteditem].fullPath);
        if(!Pop::popFile)
            return false;
            
        loadFile=Pop::popFile;

        if(Pop::findTag(PopTag::TAG_IMG_200x80_565))
        {
            Pop::drawBanner(10, 42);
            noBanner=false;
        }

        fsize=Pop::seekToCode();
    }
    else if(m_Items[m_selecteditem].Type==ItemType::Gameboy)
    {
        FRESULT res1 =f_open(&GBFile, m_Items[m_selecteditem].fullPath, FA_READ);
        if(res1!=FR_OK)
            return false;

        gbsize = f_size(&GBFile);
        if(gbsize > 1024*128)
            return false;

        f_lseek(&GBFile, 0x147);
        uint8_t mapperId;
        std::size_t n;
        f_read(&GBFile,&mapperId,1,&n);
        if( mapperId == 6 || mapperId == 3 )
            mapperId = 1;

        if(mapperId > 1)
            return false;

        f_lseek(&GBFile, 0);
        char Emu[128];
        memset(Emu,0,128);
        strcat(Emu, "kraken/GameBoy/Bin/");
        if(mapperId==0)
            strcat(Emu, "mbc0");
        else if(mapperId==1)
            strcat(Emu, "mbc1");

        const char *entries[]={"Expand to Fit", "Pixel Perfect 1:1"};
        std::size_t ret=Settings::popupMenu(entries, 2);
        if(ret > 1)
            ret=0;

        border=ret;

        if(ret==0)
            strcat(Emu, "s.bin");
        else if(ret==1)
            strcat(Emu, ".bin");
        drawLoadScreen();
        paletteFile=FileMenu("kraken/GameBoy/Palettes");

        drawLoadScreen();

        loadFile=new FIL;
        FRESULT res = f_open(loadFile, Emu, FA_READ);
        if(res!=FR_OK)
            return false;
        fsize = f_size(loadFile);
        u32=reinterpret_cast<uint32_t*>(data);
    }
    else if(m_Items[m_selecteditem].Type==ItemType::BinGame)
    {
        loadFile=new FIL;
        FRESULT res = f_open(loadFile, m_Items[m_selecteditem].fullPath, FA_READ);
        if(res!=FR_OK)
            return false;
        fsize = f_size(loadFile);
        
    }

    if(fsize > 1024*228)
        return false;
    
    if(noBanner)
        m_drawDefaultBanner(10, 42, m_Items[m_selecteditem].Type);

    while (true)
    {
        if (counter >= fsize)
            break;

        if(magicfound)
        {
            if(gbsize < 0x100)
            {
                f_lseek(loadFile, gbsize+loadFile->fptr);
                std::size_t n;
                f_read(loadFile, data+gbsize, 0x100-gbsize, &n);
            }
            else
                f_lseek(loadFile, loadFile->fptr+0x100);
            
            std::size_t n;
            f_read(&GBFile, data, 0x100, &n);
            gbsize-=n;
            if(gbsize <= 0)
                magicfound=false;
            
        }
        else
        {
            std::size_t n;
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
            for(std::size_t i=0;i<64;i++)
            {
                if(u32[i]==GBMAGIC)
                {
                    uint8_t n=data[(i+1)<<2];
                    if(i==63)
                    {
                        std::size_t count=0;
                        f_read(loadFile, &n, 1, &count);
                        f_lseek(loadFile, loadFile->fptr-1);
                    }
                    if(n==0x01)
                    {
                        magicfound=true;
                        std::size_t count;
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
                                
                            std::size_t n;
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

        drawRoundRect(10 ,110+13 ,((LCD::WIDTH-20)*counter)/fsize, 20, theme.ProgressBar);
        LCD::fillRectangle(10, 133+13, 40, 8, theme.Boxes);
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
    drawRoundRect(5,18,LCD::WIDTH-10,140, theme.BoxBorder);
    LCD::fillRectangle(7, 18+2, LCD::WIDTH-14, 140-4, theme.Boxes);

    LCD::color=theme.Text;
    LCD::cursorX=80;
    LCD::cursorY=25;
    LCD::print("Loading...");
    LCD::fillRectangle(20, 39,LCD::WIDTH-40, 1, theme.BoxBorder);
    LCD::cursorX=(LCD::WIDTH-strlen(m_Items[m_selecteditem].displayName)*6)/2;
    LCD::cursorY=46;
    LCD::print(m_Items[m_selecteditem].displayName);

    drawRoundRect(10 ,110+13 ,LCD::WIDTH-20, 20, theme.Banner);

    LCD::drawBitmap565(98, 76, 24, 24, theme.icons[static_cast<int>(m_Items[m_selecteditem].Type)]);
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

char* Loader::FileMenu(const char *Folder, bool FileOrFolder)
{
    char * ret=0;
    static FATFS_DIR dir;
    static FILINFO finfo;
    FRESULT res=f_opendir(&dir, Folder);

    char nameBuffer[_MAX_LFN+1];
    finfo.lfname=nameBuffer;
    finfo.lfsize=_MAX_LFN;

    size_t index=0;
    size_t selected=0;

    if(res!=FR_OK)
        return 0;

    char* filemenu[15];
    std::int32_t fcount=0;
    std::size_t totalCount=0;

    while(true)
    {
        res = f_readdir(&dir, &finfo);
        if(res!=FR_OK || finfo.fname[0]==0)
            break;

        if(finfo.fattrib & (FileOrFolder?AM_ARC:AM_DIR))
            totalCount++;
    }
    

    while (true)
    {
        f_opendir(&dir, Folder);
        fcount=0;

        for(std::size_t i=0;i<index;i++)
            f_readdir(&dir, &finfo);
    
        for(size_t i=0;i<15;)
        {
            char *fileName=0;
            res = f_readdir(&dir, &finfo);
            if(res!=FR_OK || finfo.fname[0]==0)
                break;
            if(finfo.lfname[0]!=0)
                fileName=finfo.lfname;
            else
                fileName=finfo.fname;

            if(finfo.fattrib & (FileOrFolder?AM_ARC:AM_DIR))
            {
                filemenu[i] = new char[28];
                filemenu[i][0]=0;
                memcpy(filemenu[i], fileName, 28);
                filemenu[i][27]=0;
                fcount++;
                i++;
            }

        }
        if(fcount==0)
            return 0;

        std::int32_t value=Settings::popupMenu(const_cast<const char**>(filemenu), fcount, index, totalCount, selected);
        if(value >= 0 && value < fcount )
        {
            char *path=new char[128];
            path[0]=0;
            strcat(path, Folder);
            strcat(path, "/");
            strcat(path, filemenu[value]);
            
            ret= path;
        }
        for(auto i=0; i<fcount ; i++)
            delete filemenu[i];

        if(value==-1 || ret!=0)
            break;

        if(value==-2)
        {
            if(index >= 15)
            {
                index-=15;
                selected=14;
            }
        }
        if(value==-3)
        {
            if(index+15<totalCount)
            {
                index+=15;
                selected=0;
            }
        }
    }

    return ret;
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
    char path[_MAX_LFN+1];
    path[0]=0;
    strcat(path, "kraken/Themes/");
    strcat(path, settings.theme);
    strcat(path, "/banners/");

    FIL BNFile;

    FRESULT res=FRESULT::FR_NO_PATH;
    if(Type==ItemType::Gameboy)
        strcat(path, "gb.565");
    else if(Type==ItemType::BinGame || Type==ItemType::Pop)
        strcat(path, "game.565");
    else if(Type==ItemType::Folder)
    {
        size_t len=strlen(path);
        char *folderName;
        folderName=strrchr(m_Items[m_selecteditem].fullPath,'/')+1;
        strcat(path, folderName);
        strcat(path, ".565");
        res=f_open(&BNFile, path, FA_READ);
        if(res!=FR_OK)
        {
            path[len]=0;
            strcat(path, "default.565");
        }
        else
            f_close(&BNFile);
    }
    else
        strcat(path, "default.565");

    res=f_open(&BNFile, path, FA_READ);
    if(res==FR_OK)
    {
        LCD::drawBitmap565File(&BNFile, x, y, 200, 80);
        f_close(&BNFile);
        return true;
    }
    else
        LCD::fillRectangle(x, y, 200, 80, theme.Banner);
    
    return false;
}

void Loader::m_getExtension(char* ext, char *fileName)
{
    char *extstr=strrchr(fileName,'.')+1;
    for(auto k=0;k<3;k++)
        ext[k]=tolower(*(extstr+k));
    ext[3]=0;
}

bool Loader::m_drawBootSplash()
{
    char path[_MAX_LFN+1];
    path[0]=0;
    strcat(path, "kraken/Themes/");
    strcat(path, settings.theme);
    strcat(path, "/bootsplash.565");

    FIL BNFile;

    FRESULT res=FRESULT::FR_NO_PATH;
    res=f_open(&BNFile, path, FA_READ);
    if(res==FR_OK)
    {
        LCD::drawBitmap565File(&BNFile, 0, 0, 220, 176);
        f_close(&BNFile);
        return true;
    }
    return false;
}

bool Loader::m_scanFolderForGames(const char* currentDir, const char *name)
{
    if(name[0]=='.')
        return false;

    FATFS_DIR dir;
    FILINFO finfo;

    char nameBuffer[_MAX_LFN+1];
    finfo.lfname = nameBuffer;
    finfo.lfsize = _MAX_LFN;
    
    char path[_MAX_LFN+1];
    path[0]=0;
    strcat(path, currentDir);
    strcat(path, "/");
    strcat(path, name);
    char* fileName;

    if(f_opendir(&dir, path)==FR_OK)
    {
        while(f_readdir(&dir, &finfo)==FR_OK && finfo.fname[0]!=0)
        {
            fileName=finfo.lfname[0]!=0?finfo.lfname:finfo.fname;
            if((finfo.fattrib & AM_DIR ))
            {
                if(m_scanFolderForGames(path, fileName))
                    return true;
            }
            if(finfo.fattrib & AM_ARC)
            {
                char extension[4];
                m_getExtension(extension, fileName);
                if((strcmp(extension, "pop")==0) || (strcmp(extension, "gb")==0))
                    return true;

                if(strcmp(extension,"bin")==0)
                {
                    char fPath[_MAX_LFN+1];
                    fPath[0]=0;
                    strcat(fPath, path);
                    strcat(fPath, "/");
                    strcat(fPath, fileName);
                    
                    if(m_binIsGame(fPath))
                        return true;
                }
            }
        }
    }
    return false;
}

bool Loader::m_binIsGame(const char* path)
{
    FIL fh;
    FRESULT res = f_open(&fh, path, FA_READ);
    if(res==FR_OK)
    {
        uint32_t magic;
        std::size_t n;
        f_read(&fh, &magic, 4, &n);
        if(magic==BINMAGIC)
        {
            f_close(&fh);
            return true;
        }
        f_close(&fh);    
    }
    return false;
}