#pragma once

#include <cstdint>
#include <ctime>
#include "SDFileSystem.h"
#include "Buttons.h"
#include "Settings.h"
#include "Themes.h"

#define MAX_FILES 512

enum class ItemType
{
    Folder=0,
    File,
    BinGame,
    Pop,
    Gameboy
};

struct Item
{
    char fullPath[_MAX_LFN];
    char displayName[27+1];
    size_t Size;
    bool hasIcon;
    ItemType Type;
};

class Loader
{
    enum class State
    {
        Browse=0,
        Settings,
        Coverflow,
        InfoPopup,
        ErrorPopup
    };

    static constexpr const uint32_t GBMAGIC=0xEFBEADDE;
    static constexpr const uint32_t BINMAGIC=0X10008000;
    static const uint8_t gbDefaultPal[16];
public:
    void Init();
    void run();
    void update();
    void draw();
    static void drawClock();
    void drawTopBar();
    void drawScrollBar();
    void drawBottomBar(bool loading=false);
    void drawItem(size_t num);
    void drawItemCoverflow(size_t num);
    void drawAllItems();
    void drawAllItemsCoverflow();
    static void drawRoundRect(int x, int y, uint16_t w, uint16_t h, uint16_t color);
    static void drawHollowRoundRect(int x, int y, uint16_t w, uint16_t h, uint16_t color);
    void infoPopup();
    static void errorPopup(const char *txt);
    static bool loadFile();
    static void drawLoadScreen();
    void loadItems();
    static char* FileMenu(const char *Folder, bool FileOrFolder=true);
    static SDFileSystem *sdFs;
    static Settings settings;
    static Theme theme;
private:
    void m_nextItem();
    void m_prevItem();
    void m_nextPage();
    void m_prevPage();
    void processGB(uint8_t *data);
    void m_pushPrev();
    void m_pullPrev();
    static Item m_Items[5];
    size_t m_fileCount();
    void m_reloadItems();
    static bool m_drawDefaultBanner(uint16_t x, uint16_t y, ItemType Type);
    static bool m_drawBootSplash();
    static void m_getExtension(char* ext, char *fileName);

    static bool m_scanFolderForGames(const char* currentDir, const char *name);
    static bool m_binIsGame(const char* path);
    static size_t m_itemcount;
    static size_t m_itemlistcount;
    static size_t m_beginitem;
    static size_t m_selecteditem;
    static bool m_redraw;
    static char m_curDir[_MAX_LFN+1];
    static uint16_t m_indexs[512];
    static time_t m_seconds;
    static State m_state;
    static State m_oldstate;
    static size_t m_prevSelecteditem[10];
    static size_t m_prevBeginitem[10];
    static uint8_t m_pPrev;

};