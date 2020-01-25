#pragma once
#include <cstdint>
#include <cctype>
#include <ctime>

/** SYSTEM SETTINGS ADDRESSES IN EEPROM **/
#define EESETTINGS_FILENAME         3980 // 0xF8C 20bytes last filename requested
#define EESETTINGS_VOL              4000 // 0xFA0 Volume
#define EESETTINGS_DEFAULTVOL       4001 // 0xFA1 Default volume
#define EESETTINGS_LOADERWAIT       4002 //	0xFA2 Loader wait in sec
#define EESETTINGS_VOLWAIT          4003 // 0xFA3 Volume screen wait in sec
#define EESETTINGS_TIMEFORMAT       4004 // 0xFA4 Time format (0=24 hrs, 1 = 12 hrs)
#define EESETTINGS_LASTHOURSSET     4005 // 0xFA5 Last time set in hours
#define EESETTINGS_LASTMINUTESSET   4006 // 0xFA6 Last time set in minutes
#define EESETTINGS_DATEFORMAT       4007 // 0xFA7 Date format (0=D/M/Y, 1 = M/D/Y)
#define EESETTINGS_LASTDAYSET       4008 // 0xFA8 Last Day set
#define EESETTINGS_LASTMONTHSET     4009 // 0xFA9 Last Month set
#define EESETTINGS_LASTYEARSET      4010 // 0xFAA Last Year set (counting from 2000)
#define EESETTINGS_RTCALARMMODE     4011 // 0xFAB RTC alarm mode (0=disabled, 1=enabled, 3 = enabled with sound)
#define EESETTINGS_RESERVED         4012 // 0xFAC 4bytes reserved (additional sleep configuration)
#define EESETTINGS_WAKEUPTIME       4016 // 0xFB0 Wake-up time as 32bit value for 1Hz RTC clock
#define EESETTINGS_SKIPINTRO	    4020 // 0xFB4 Show Logo (0-Yes, 1-No, 2-Yes then switch to 0, 3-No, then switch to 1)
#define EESETTINGS_SHOWALLFILES	    4021 // 0xFB5 Show All Files on the loader
#define EESETTINGS_VIEWMODE  	    4022 // 0xFB6 View Mode of the loader

#define COOKIE_NAME_ADDRESS   0x178
#define COOKIE_KEY_ADDRESS    0x1EF
#define COOKIE_DATA_ADDRESS   0xE00

class Settings
{
public:
    enum ViewMode
    {
        Icons=0,
        List
    };

    enum PickerType
    {
        Time=0,
        Date
    };

    void Init();
    bool update();
    void draw();
    void drawEntries();
    uint8_t ShowAllFiles=false;
    ViewMode viewmode=ViewMode::Icons;
    static int32_t popupMenu(const char *entries[], size_t NumOfEntries, size_t EntriesStart=0, size_t TotalEntries=0, size_t EntrieSelected=0);
    static size_t timePicker(bool format24, struct tm *ltm);
    static size_t datePicker(uint8_t dateFormat, struct tm *ltm);
    static time_t time_to_epoch(const struct tm *ltm);
    char theme[32];
    void saveTheme(const char* name);
private:
    static void m_drawPickerFrame(PickerType type);
    static const char *m_entries[];
    bool m_redraw=true;
    size_t m_selected=0;
    uint8_t m_loaderwait=5;
    uint8_t m_volumewait=10;
    uint8_t m_timeformat=0;
    uint8_t m_dateformat=0;
};