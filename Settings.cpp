#include "Settings.h"
#include "Buttons.h"
#include "LCD.h"
#include "Palette.h"
#include "iap.h"
#include "Loader.h"
#include <cstring>
#include <cstdint>

const char *Settings::m_entries[]={"Theme", "View Mode", "Show All Files", "Loader Wait", "Volume Wait", "Time Format" , "Date Format", "Time", "Date"};

void rtc_write(time_t t)
{
    // Disabled RTC
    LPC_RTC->CTRL &= ~(1 << 7);
    // Set count
    LPC_RTC->COUNT = t;
    //Enabled RTC
    LPC_RTC->CTRL |= (1 << 7);
}

void Settings::Init()
{
    //ShowAllFiles=true;
    readEEPROM(reinterpret_cast<uint16_t*>(EESETTINGS_SHOWALLFILES), &ShowAllFiles, 1);
    readEEPROM(reinterpret_cast<uint16_t*>(EESETTINGS_VIEWMODE), reinterpret_cast<uint8_t*>(&viewmode), 1);
    readEEPROM(reinterpret_cast<uint16_t*>(EESETTINGS_LOADERWAIT), &m_loaderwait, 1);
    readEEPROM(reinterpret_cast<uint16_t*>(EESETTINGS_VOLWAIT), &m_volumewait, 1);
    readEEPROM(reinterpret_cast<uint16_t*>(EESETTINGS_TIMEFORMAT), &m_timeformat, 1);
    readEEPROM(reinterpret_cast<uint16_t*>(EESETTINGS_DATEFORMAT), &m_dateformat, 1);

    readEEPROM(reinterpret_cast<uint16_t*>(COOKIE_DATA_ADDRESS), reinterpret_cast<std::uint8_t*>(&theme[0]), 32);

    if(m_loaderwait==0)
    {
        m_loaderwait=3;
        writeEEPROM(reinterpret_cast<uint16_t*>(EESETTINGS_LOADERWAIT), &m_loaderwait,1);
        m_volumewait=10;
        writeEEPROM(reinterpret_cast<uint16_t*>(EESETTINGS_VOLWAIT), &m_volumewait,1);
        std::uint8_t vol=127;
        writeEEPROM(reinterpret_cast<uint16_t*>(EESETTINGS_VOL), &vol,1);
        rtc_write(1575158400);//12/01/2019 @ 12:00am (UTC)
    }
    
    char path[_MAX_LFN+1];
    path[0]=0;
    strcat(path, "kraken/Themes/");
    strcat(path, theme);

    FATFS_DIR dir;
    if((f_opendir(&dir, path)!=FR_OK) || theme[0]==0)
        saveTheme("default");
    
    m_redraw=true;
}

bool Settings::update()
{
    if(Buttons::repeat(Buttons::BTN_UP,20000))
    {
        if(m_selected > 0 )
           m_selected --;
        drawEntries();
    }else if(Buttons::repeat(Buttons::BTN_DOWN,20000))
    {
         if(m_selected < 8 )
           m_selected ++;
        drawEntries();
    }

    if(Buttons::pressed(Buttons::BTN_A))
    {
        switch(m_selected)
        {
        case 0:
            {
                const char *fullThemePath=Loader::FileMenu("kraken/Themes", false);
                
                if(fullThemePath!=0)
                {
                    char *themeName=strrchr(fullThemePath,'/')+1;
                    saveTheme(themeName);
                    Loader::theme.loadTheme(themeName);
                }
                
            }
            break;
        case 1:
            {
                const char *menu_entries[]={"Icons", "List"};
                uint8_t ret=popupMenu( menu_entries,2);
                if(ret < 2)
                {
                    viewmode = (ViewMode) ret;
                    writeEEPROM(reinterpret_cast<uint16_t*>(EESETTINGS_VIEWMODE), reinterpret_cast<uint8_t*>(&viewmode),1);
                }
            }
            break;
        case 2:
            {
                const char *menu_entries[]={"No", "Yes"};
                size_t ret=popupMenu( menu_entries,2);
                if(ret < 2)
                {
                    ShowAllFiles=ret;
                    writeEEPROM(reinterpret_cast<uint16_t*>(EESETTINGS_SHOWALLFILES), &ShowAllFiles,1);
                }
            }
            break;
        case 3:
            {
                const char *menu_entries[]={"1s" ,"2s","3s","4s","5s"};
                size_t ret=popupMenu( menu_entries, 5);
                if(ret < 5)
                {
                    m_loaderwait=ret+1;
                    writeEEPROM(reinterpret_cast<uint16_t*>(EESETTINGS_LOADERWAIT), &m_loaderwait,1);
                }
            }
            break;
        case 4:
            {
                const char *menu_entries[]={"0s", "1s", "2s", "3s", "4s", "5s", "6s" ,"7s" ,"8s" ,"9s" ,"10s"};
                size_t ret=popupMenu( menu_entries, 11);
                if(ret < 10)
                {
                    m_volumewait=ret;
                    writeEEPROM(reinterpret_cast<uint16_t*>(EESETTINGS_VOLWAIT), &m_volumewait,1);
                }

            }
            break;
        case 5:
            {
                const char *menu_entries[]={"24H", "12H"};
                size_t ret=popupMenu( menu_entries,2);
                if(ret < 2)
                {
                    m_timeformat=ret;
                    writeEEPROM(reinterpret_cast<uint16_t*>(EESETTINGS_TIMEFORMAT), &m_timeformat,1);
                }

            }
            break;
        case 6:
            {
                const char *menu_entries[]={"DMY", "MDY"};
                size_t ret=popupMenu( menu_entries,2);
                if(ret < 2)
                {
                    m_dateformat=ret;
                    writeEEPROM(reinterpret_cast<uint16_t*>(EESETTINGS_DATEFORMAT), &m_dateformat,1);
                }

            }
            break;
        case 7:
            {
                time_t seconds = (time_t)LPC_RTC->COUNT;
                struct tm *tmp = gmtime(&seconds);
                size_t t=timePicker(m_timeformat==0, tmp);
                if(t!=0xFFFFFFFF)
                {
                    rtc_write(time_to_epoch(tmp));
                }
            }
            break;
        case 8:
            {
                time_t seconds = (time_t)LPC_RTC->COUNT;
                struct tm *tmp = gmtime(&seconds);
                size_t t=datePicker(m_dateformat, tmp);
                if(t!=0xFFFFFFFF)
                {
                   // seconds = time(NULL);
                    //struct tm *tmp2 = gmtime(&seconds);
                    //tmp->tm_min=tmp2->tm_min;
                    //tmp->tm_hour=tmp2->tm_hour;
                    
                    rtc_write(time_to_epoch(tmp));
                }
            }
            break;

        }
        m_redraw=true;
        return true;
    }

    if(Buttons::pressed(Buttons::BTN_B))
        return false;
    return true;
}

void Settings::draw()
{
    if(!m_redraw)
        return;
    else
        m_redraw=false;
    
    LCD::Clear(Loader::theme.BackgroundTop);

    Loader::drawRoundRect(2, -2, LCD::WIDTH-4, 10+4, Loader::theme.Boxes);
    LCD::fillRectangle(4, 11, LCD::WIDTH-8, 1, Loader::theme.Boxes+0x2965);


    Loader::drawClock();
    LCD::cursorX=17;
    LCD::cursorY=2;
    LCD::color=Loader::theme.Text;

    LCD::print("Settings");

    drawEntries();

}

void Settings::drawEntries()
{
    for(size_t i = 0 ;i < sizeof(m_entries)/sizeof(void*); i++)
    {

        LCD::fillRectangle(5,15+i*15, LCD::WIDTH-10,15-2, m_selected==i?Loader::theme.Selected:Loader::theme.Boxes);
        if(m_selected==i)
            LCD::color=Loader::theme.SelectedText;
        else
            LCD::color=Loader::theme.Text;

        LCD::cursorX = 20;
        LCD::cursorY = 10+8+i*15;
        LCD::print(m_entries[i]);

        LCD::cursorX=120;
        //LCD::cursorY=15+11+i*20;
        time_t seconds = (time_t)LPC_RTC->COUNT;
        struct tm *tmp = gmtime(&seconds);
        switch(i)
        {
        case 0:
                for(std::size_t i=0; i<15;i++)
                {
                    if(theme[i]==0)
                        break;
                    LCD::write(theme[i]);
                    if(i==14)
                        LCD::write('.');
                }
            break;
        case 1:
            if(viewmode==ViewMode::Icons)
                LCD::print("Icons");
            else if(viewmode==ViewMode::List)
                LCD::print("List");
            break;
        case 2:
            if(ShowAllFiles)
                LCD::print("Yes");
            else
                LCD::print("No");
            break;
        case 3:
            
            LCD::printNumber(m_loaderwait);
            LCD::print(" Sec");
            break;
        case 4:
 
            LCD::printNumber(m_volumewait);
            LCD::print(" Sec");
            break;
        case 5:

            if(m_timeformat==0)
                LCD::printNumber(24);
            if(m_timeformat==1)
                LCD::printNumber(12);
            LCD::write('H');
            break;
        case 6:

            if(m_dateformat==0)
                LCD::print("DM");
            if(m_dateformat==1)
                LCD::print("MD");
            LCD::write('Y');
            break;
        case 7:

            if(m_timeformat==0)
            {
                LCD::printNumber(tmp->tm_hour);
                LCD::write(':');
                LCD::printNumber(tmp->tm_min);
            }
            if(m_timeformat==1)
            {
                LCD::printNumber(tmp->tm_hour>12?tmp->tm_hour-12:(tmp->tm_hour==0?12:tmp->tm_hour));
                LCD::write(':');
                LCD::printNumber(tmp->tm_min);
                LCD::write(' ');
                LCD::write(tmp->tm_hour < 12?'A':'P');
                LCD::write('M');
            }
            break;
        case 8:

            if(m_dateformat==0)
            {
                LCD::printNumber(tmp->tm_mday);
                LCD::write('-');
                LCD::printNumber(tmp->tm_mon+1);
            }

            if(m_dateformat==1)
            {
                LCD::printNumber(tmp->tm_mon+1);
                LCD::write('-');
                LCD::printNumber(tmp->tm_mday);
            }
            LCD::write('-');
            LCD::printNumber(tmp->tm_year+1900);

            break;
        }
    }
}

int32_t Settings::popupMenu(const char *entries[], size_t NumOfEntries, size_t EntriesStart, size_t TotalEntries, size_t EntrieSelected)
{
    size_t selected=EntrieSelected;
    bool redraw=true;
    size_t menuYOffset=(176-10*NumOfEntries)/2;
    Loader::drawRoundRect(10,menuYOffset-10,LCD::WIDTH-20,20+10*NumOfEntries, Loader::theme.BoxBorder);
    LCD::fillRectangle(12, menuYOffset-10+2, LCD::WIDTH-24, 20+10*NumOfEntries-4, Loader::theme.Boxes);

    if(TotalEntries>15)
    {
        menuYOffset=13;
        Loader::drawRoundRect(10,menuYOffset-10,LCD::WIDTH-20,20+10*15, Loader::theme.BoxBorder);
        LCD::fillRectangle(12, menuYOffset-10+2, LCD::WIDTH-24, 20+10*15-4, Loader::theme.Boxes);
    }
    LCD::color=Loader::theme.Text;
    LCD::cursorX=20;
    LCD::cursorY=menuYOffset-7;
    LCD::print("Choose value");
    LCD::fillRectangle(15,menuYOffset+3,LCD::WIDTH-30,1, Loader::theme.BoxBorder);

    //scroll Bar

    if(TotalEntries > 15)
    {
        std::size_t div=150/((TotalEntries+14)/15);
        std::size_t div10=1500/((TotalEntries+14)/15);

        std::size_t start=15+5+(div*(EntriesStart/15))+(div10*(EntriesStart/15))/100;
        if(start > 150+15-div)
            start=150+15-div;

        LCD::fillRectangle(LCD::WIDTH-16, start , 3, div, Loader::theme.Selected);
    }


    while(true)
    {
        Buttons::update();

        if(redraw)
        {
            for(size_t i=0;i<NumOfEntries;i++)
            {
                LCD::fillRectangle(11+2,5+10*i+menuYOffset,LCD::WIDTH-22-4-5,10, selected==i?Loader::theme.Selected:Loader::theme.Boxes);
                LCD::color=selected==i?Loader::theme.SelectedText:Loader::theme.Text;

                LCD::cursorX=35;
                LCD::cursorY=7+10*i+menuYOffset;
                LCD::print(entries[i]);
            }
            redraw=false;
        }

        if(Buttons::repeat(Buttons::BTN_UP, 20000))
        {
            if(selected == 0 && EntriesStart > 0)
                return -2;
            if(selected > 0)
                selected--;
            redraw=true;
        }

        if(Buttons::repeat(Buttons::BTN_DOWN, 20000))
        {
            if((selected == (NumOfEntries-1))&&  (NumOfEntries+EntriesStart)< TotalEntries  )
                return -3;
            if(selected < NumOfEntries-1)
                selected++;
            redraw=true;
        }

        if(Buttons::pressed(Buttons::BTN_A))
            return selected;
        if(Buttons::pressed(Buttons::BTN_B))
            return -1;
    }
}

size_t Settings::timePicker(bool format24, struct tm *ltm)
{
    size_t selected=0;
    bool redraw=true;

    m_drawPickerFrame(PickerType::Time);
    while(true)
    {
        Buttons::update();

        if(redraw)
        {
            redraw=false;
            LCD::fillRectangle(12, 72, LCD::WIDTH-24, 70-20, Loader::theme.Boxes);
            LCD::color=selected==0?Loader::theme.Selected:Loader::theme.Text;

            LCD::cursorX=90;
            LCD::cursorY=80;
            LCD::printNumber(format24?ltm->tm_hour:(ltm->tm_hour>12?ltm->tm_hour-12:(ltm->tm_hour==0?12:ltm->tm_hour)));

            LCD::color=Loader::theme.Text;
    
            LCD::cursorX=107;

            LCD::write(':');

            LCD::color=selected==1?Loader::theme.Selected:Loader::theme.Text;
            LCD::cursorX=118;

            LCD::printNumber(ltm->tm_min);
        }



        if(!format24)
        {
            LCD::color=Loader::theme.Text;
            LCD::cursorX=150;
            LCD::cursorY=80;

            if(ltm->tm_hour > 11)
                LCD::print("PM");
            else
                LCD::print("AM");
        }


        if(Buttons::repeat(Buttons::BTN_UP,30000))
        {
            if(selected==0)
                ltm->tm_hour++;
            if(selected==1)
                ltm->tm_min++;

            if(ltm->tm_hour==24)
                ltm->tm_hour=0;
            if(ltm->tm_min==60)
                ltm->tm_min=0;
            redraw=true;

        }else if(Buttons::repeat(Buttons::BTN_DOWN,30000))
        {
             if(selected==0)
                ltm->tm_hour--;
            if(selected==1)
                ltm->tm_min--;

            if(ltm->tm_hour < 0)
                ltm->tm_hour=23;
            if(ltm->tm_min < 0)
                ltm->tm_min=59;
            redraw=true;
        }

        if(Buttons::pressed(Buttons::BTN_RIGHT))
        {
            if(selected < 1)
                selected++;
            redraw=true;

        }else if(Buttons::pressed(Buttons::BTN_LEFT))
        {
             if(selected > 0)
                selected--;
            redraw=true;
        }

        if(Buttons::pressed(Buttons::BTN_A))
            return 0;
        if(Buttons::pressed(Buttons::BTN_B))
            return 0xFFFFFFFF;
    }
}

size_t Settings::datePicker(uint8_t dateFormat, struct tm *ltm)
{
    size_t selected=0;
    bool redraw=true;
    m_drawPickerFrame(PickerType::Date);
    while(true)
    {
        Buttons::update();
        if(redraw)
        {
            redraw=false;
            LCD::fillRectangle(12, 72, LCD::WIDTH-24, 70-20, Loader::theme.Boxes);
            LCD::color=selected==0?Loader::theme.Selected:Loader::theme.Text;
            LCD::cursorX=83;
            LCD::cursorY=80;
            LCD::printNumber(dateFormat==0?ltm->tm_mday:ltm->tm_mon+1);

            LCD::color=selected==1?Loader::theme.Selected:Loader::theme.Text;
            LCD::cursorX=98;

            LCD::printNumber(dateFormat==0?ltm->tm_mon+1:ltm->tm_mday);

            LCD::color=selected==2?Loader::theme.Selected:Loader::theme.Text;
            LCD::cursorX=113;

            LCD::printNumber(ltm->tm_year+1900);
        }
        
        if(Buttons::repeat(Buttons::BTN_UP,30000))
        {
            if(selected==0)
                dateFormat==0?ltm->tm_mday++:ltm->tm_mon++;
            if(selected==1)
                dateFormat==0?ltm->tm_mon++:ltm->tm_mday++;
            if(selected==2)
                ltm->tm_year++;

            if(ltm->tm_mday==32)
                ltm->tm_mday=1;
            if(ltm->tm_mon==12)
                ltm->tm_mon=0;
            redraw=true;

        }else if(Buttons::repeat(Buttons::BTN_DOWN,30000))
        {
            if(selected==0)
                dateFormat==0?ltm->tm_mday--:ltm->tm_mon--;
            if(selected==1)
                dateFormat==0?ltm->tm_mon--:ltm->tm_mday--;
             if(selected==2)
                ltm->tm_year--;

            if(ltm->tm_mday==0)
                ltm->tm_mday=31;
            if(ltm->tm_mon< 0)
                ltm->tm_mon=11;
            if(ltm->tm_year< 0)
                ltm->tm_year=9999;
            redraw=true;
        }

        if(Buttons::pressed(Buttons::BTN_RIGHT))
        {
            if(selected < 2)
                selected++;
            redraw=true;

        }else if(Buttons::pressed(Buttons::BTN_LEFT))
        {
             if(selected > 0)
                selected--;
            redraw=true;
        }

        if(Buttons::pressed(Buttons::BTN_A))
            return 0;
        if(Buttons::pressed(Buttons::BTN_B))
            return 0xFFFFFFFF;
    }
}

time_t Settings::time_to_epoch( const struct tm *ltm )
{
   constexpr int mon_days [] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
   long tyears, tdays, leaps, utc_hrs;
   int i;

   tyears = ltm->tm_year - 70;  // tm->tm_year is from 1900.
   leaps = (tyears + 2) / 4; // no of next two lines until year 2100.
   //i = (ltm->tm_year – 100) / 100;
   //leaps -= ( (i/4)*3 + i%4 );
   tdays = 0;
   for (i=0; i < ltm->tm_mon; i++) tdays += mon_days[i];

   tdays += ltm->tm_mday-1; // days of month passed.
   tdays = tdays + (tyears * 365) + leaps;

   utc_hrs = ltm->tm_hour ; // for your time zone.
   return (tdays * 86400) + (utc_hrs * 3600) + (ltm->tm_min * 60) + ltm->tm_sec;
}

void Settings::saveTheme(const char* name)
{
    std::uint8_t cookieName[8];
    memcpy(cookieName, "Kraken  ", 8);
    writeEEPROM(reinterpret_cast<uint16_t*>(COOKIE_NAME_ADDRESS), &cookieName[0], 8);
    std::uint8_t value=0x7F;
    writeEEPROM(reinterpret_cast<uint16_t*>(COOKIE_KEY_ADDRESS), &value, 1);
    memcpy(theme, name, strlen(name)+1);

    writeEEPROM(reinterpret_cast<uint16_t*>(COOKIE_DATA_ADDRESS), reinterpret_cast<uint8_t*>(&theme[0]), 32);
}

void Settings::m_drawPickerFrame(PickerType type)
{
    Loader::drawRoundRect(10,51,LCD::WIDTH-20, 74, Loader::theme.BoxBorder);
    LCD::fillRectangle(12, 51+2, LCD::WIDTH-24, 70, Loader::theme.Boxes);
    LCD::color=Loader::theme.Text;
    LCD::cursorX=86;
    LCD::cursorY=57;
    LCD::print("Set ");
    LCD::print((type==PickerType::Date)?"Date":"Time");
    LCD::fillRectangle(15,70,LCD::WIDTH-30,1, Loader::theme.BoxBorder);
}