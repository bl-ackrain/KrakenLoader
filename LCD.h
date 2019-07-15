#pragma once
#include <mbed.h>
#include <cstdint>
#include "SDFileSystem.h"

static constexpr uint8_t LCD_CD_PORT = 0;
static constexpr uint8_t LCD_CD_PIN = 2;
static constexpr uint8_t LCD_WR_PORT = 1;
static constexpr uint8_t LCD_WR_PIN = 12;
static constexpr uint8_t LCD_RD_PORT = 1;
static constexpr uint8_t LCD_RD_PIN = 24;
static constexpr uint8_t LCD_RES_PORT = 1;
static constexpr uint8_t LCD_RES_PIN = 0;

/**************************************************************************/
/**                          LCD CONTROL MACROS                          **/
/**************************************************************************/

#define CLR_RESET LPC_GPIO_PORT->CLR[LCD_RES_PORT] = 1 << LCD_RES_PIN; //RST = (0); // Clear pin
#define SET_RESET LPC_GPIO_PORT->SET[LCD_RES_PORT] = 1 << LCD_RES_PIN; // RST = (1); // Set pin

#define CLR_CD  LPC_GPIO_PORT->CLR[LCD_CD_PORT] = 1 << LCD_CD_PIN; // RS = (0); // Clear pin
#define SET_CD LPC_GPIO_PORT->SET[LCD_CD_PORT] = 1 << LCD_CD_PIN; // RS = (1); // Set pin

#define CLR_WR { LPC_GPIO_PORT->CLR[LCD_WR_PORT] = 1 << LCD_WR_PIN; __asm("nop");}//WR = (0); // Clear pin
#define SET_WR LPC_GPIO_PORT->SET[LCD_WR_PORT] = 1 << LCD_WR_PIN; //WR = (1); // Set pin

#define CLR_RD LPC_GPIO_PORT->CLR[LCD_RD_PORT] = 1 << LCD_RD_PIN; //RD = (0); // Clear pin
#define SET_RD LPC_GPIO_PORT->SET[LCD_RD_PORT] = 1 << LCD_RD_PIN; //RD = (1); // Set pin

#define SET_CS  //CS tied to ground
#define CLR_CS

#define CLR_CS_CD_SET_RD_WR {CLR_CD; SET_RD; SET_WR;}
#define CLR_CS_SET_CD_RD_WR {SET_CD; SET_RD; SET_WR;}
#define SET_CD_RD_WR {SET_CD; SET_RD; SET_WR;}
#define SET_WR_CS SET_WR;

#define SET_MASK_P2 LPC_GPIO_PORT->MASK[2] = ~(0x7FFF8); //mask P2_3 ...P2_18
#define CLR_MASK_P2 LPC_GPIO_PORT->MASK[2] = 0; // all on

class LCD
{
    static constexpr auto LCDBOOTCMDSIZE=34;

public:
    static constexpr int WIDTH = 220;
    static constexpr int HEIGHT = 176;

    static void SetupGPIO();
    static void Init();
    static void SetupData(uint16_t data);
    static void WriteCommand(uint16_t data);
    static void WriteData(uint16_t data);
    static void setWindow(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);

    static void Clear(uint16_t color);
    static void drawPixel(int x, int y, uint16_t color);
    static void fillRectangle(int x, int y, int w, int h, uint16_t color);
    static void drawRectangle(int x, int y, int w, int h, uint16_t color);
    static void drawBitmap16(int x, int y, uint16_t w, uint16_t h, const uint8_t bitmap[], const uint16_t palette[], uint8_t transparentColor, uint8_t replaceWith);
    static void drawBitmap565File(FIL* file, int x, int y, uint16_t w, uint16_t h, size_t Xoffset=0);
    static void drawChar(int x, int y, uint8_t c);
    static void write(uint8_t c);
    static void print(const char txt[]);
    static void printNumber(uint32_t n);
    static void printWraped(size_t margin, size_t width ,const char txt[]);
    static uint8_t cursorX, cursorY;
    static uint8_t *font;
    static uint16_t color;
private:
    static constexpr const uint8_t m_lcdbootcmd[LCDBOOTCMDSIZE]={
        0x01, 0x02, 0x03, 0x08, 0x0C, 0x0F, 0x20, 0x21, 0x10, 0x11, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0xff, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0xB0, 0xFF, 0x07};

    static constexpr const uint16_t m_lcdbootdata[LCDBOOTCMDSIZE]={
        0x011C, 0x0100, 0x1038, 0x0808, 0x0000, 0x0001, 0x0000, 0x0000, 0x0000, 0x1000, 0x0000, 0x00DB, 0x0000, 0x0000, 0x00DB, 0x0000, 0x00AF, 0x0000, 0x00DB, 0x0000, 0x0003, 0x0203, 0x0A09, 0x0005, 0x1021, 0x0602, 0x0003, 0x0703, 0x0507, 0x1021, 0x0703, 0x2501, 0x0000, 0x1017};
    static uint8_t m_fontWidth, m_fontHeight;

};