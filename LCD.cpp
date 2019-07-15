#include "LCD.h"

#include <pinmap.h>
#include "font5x7.h"

constexpr const  uint8_t LCD::m_lcdbootcmd[LCDBOOTCMDSIZE];
constexpr const  uint16_t LCD::m_lcdbootdata[LCDBOOTCMDSIZE];

uint8_t LCD::cursorX=0, LCD::cursorY=0;
uint8_t *LCD::font=0;
uint16_t LCD::color=0;
uint8_t LCD::m_fontWidth=0, LCD::m_fontHeight=0;

void LCD::SetupGPIO()
{
    /** control lines **/
    LPC_GPIO_PORT->DIR[LCD_CD_PORT] |= (1  << LCD_CD_PIN );
    LPC_GPIO_PORT->DIR[LCD_WR_PORT] |= (1  << LCD_WR_PIN );
    LPC_GPIO_PORT->DIR[LCD_RD_PORT] |= (1  << LCD_RD_PIN );
    LPC_GPIO_PORT->DIR[LCD_RES_PORT] |= (1  << LCD_RES_PIN );
    /** data lines **/
    LPC_GPIO_PORT->DIR[2] |= (0xFFFF  << 3);  // P2_3...P2_18 as output

    pin_mode(P2_3,PullNone); // turn off pull-up
    pin_mode(P2_4,PullNone); // turn off pull-up
    pin_mode(P2_5,PullNone); // turn off pull-up
    pin_mode(P2_6,PullNone); // turn off pull-up

    pin_mode(P2_7,PullNone); // turn off pull-up
    pin_mode(P2_8,PullNone); // turn off pull-up
    pin_mode(P2_9,PullNone); // turn off pull-up
    pin_mode(P2_10,PullNone); // turn off pull-up

    pin_mode(P2_11,PullNone); // turn off pull-up
    pin_mode(P2_12,PullNone); // turn off pull-up
    pin_mode(P2_13,PullNone); // turn off pull-up
    pin_mode(P2_14,PullNone); // turn off pull-up

    pin_mode(P2_15,PullNone); // turn off pull-up
    pin_mode(P2_16,PullNone); // turn off pull-up
    pin_mode(P2_17,PullNone); // turn off pull-up
    pin_mode(P2_18,PullNone); // turn off pull-up


}

void LCD::SetupData(uint16_t data)
{
    SET_MASK_P2;
    LPC_GPIO_PORT->MPIN[2] = data << 3; // write bits to port
    CLR_MASK_P2;
}

void LCD::WriteCommand(uint16_t command)
{
    CLR_CS;
    CLR_CD;
    SET_RD;
    SetupData(command); 
    CLR_WR;
    SET_WR;
    SET_CS;
}

void LCD::WriteData(uint16_t data)
{
   CLR_CS;
   SET_CD;
   SET_RD;
   SetupData(data);
   CLR_WR;
   SET_WR;
   SET_CS;
}

void LCD::setWindow(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2) {
	WriteCommand(0x37); WriteData(x1); // x1 0
	WriteCommand(0x36); WriteData(x2); // x2 175
	WriteCommand(0x39); WriteData(y1); // y1  0
	WriteCommand(0x38); WriteData(y2); // y2 219

	WriteCommand(0x20); WriteData(x1); // hor
	WriteCommand(0x21); WriteData(y1); // vert
    WriteCommand(0x22);
}

void LCD::Init()
{
    SetupGPIO();
    
    SET_RESET;
    wait_ms(10);
    CLR_RESET;
    wait_ms(10);
    SET_RESET;
    wait_ms(10);

    for(auto i=0;i<LCDBOOTCMDSIZE;i++)
    {
        WriteCommand(m_lcdbootcmd[i]);
        WriteData(m_lcdbootdata[i]);
        if(i==9 || i==19)
            wait_ms(10);
    }
    WriteCommand(0x22);
    Clear(0x4a8a);

    cursorX = 0;
    cursorY = 0;
    font = const_cast<uint8_t*>(font5x7+3);
    color = 0xFFFF;
    m_fontWidth = font5x7[0];
    m_fontHeight = font5x7[1];

    //SETUP BACKLIGHT
    pin_function(P2_2,0);//set pin function back to 0
    LPC_GPIO_PORT->DIR[2] |= (1  << 2 );
    LPC_GPIO_PORT->SET[2] = 1 << 2; // full background light, smaller file size
    wait_ms(200);
}

void LCD::Clear(uint16_t color)
{
    WriteCommand(0x20);
    WriteData(0);
    WriteCommand(0x21);
    WriteData(0);
    WriteCommand(0x22);
    SetupData(color);
    CLR_CS_SET_CD_RD_WR;

    for(auto i=0;i<WIDTH*HEIGHT;i++)
    {
        CLR_WR;
        SET_WR;
    }
}

void LCD::drawPixel(int x, int y, uint16_t color)
{
    if((x < 0) || (x >= WIDTH) || (y < 0) || (y >= HEIGHT))
	    return;

	WriteCommand(0x20);
    WriteData(y);
    WriteCommand(0x21);
    WriteData(x);
    WriteCommand(0x22);
    CLR_CS_SET_CD_RD_WR;
    SetupData(color);
    CLR_WR;SET_WR;
}

void LCD::fillRectangle(int x, int y, int w, int h, uint16_t color)
{
	if(x > WIDTH)
        return;
	if(y > HEIGHT)
        return;
    if((x+w) > WIDTH)
        w=WIDTH-x;
	if((y+h) > HEIGHT)
        h=HEIGHT-y;
	if(x < 0)
        x=0;
	if(y < 0)
        y=0;

    setWindow(y, x, y+h-1, x+w-1);

    SetupData(color);
    CLR_CS_SET_CD_RD_WR;

    for(auto i=0;i<w*h;i++)
    {
        CLR_WR;
        SET_WR;
    }
    setWindow(0, 0, HEIGHT-1, WIDTH-1);
}

void LCD::drawRectangle(int x, int y, int w, int h, uint16_t color)
{
    fillRectangle(x, y, w, 1, color);
    fillRectangle(x, y+h-1, w, 1, color);
    fillRectangle(x, y, 1, h, color);
    fillRectangle(x+w-1, y, 1, h, color);

}

void LCD::drawBitmap16(int x, int y, uint16_t w, uint16_t h, const uint8_t bitmap[], const uint16_t palette[], uint8_t transparentColor, uint8_t replaceWith)
{
    for(auto i=0;i<h;i++)
        for(auto j=0;j<w>>1;j++)
        {
            uint8_t d=bitmap[(i*(w>>1))+j];

            drawPixel(x+j*2, y+i, palette[((d>>4)!=transparentColor)?d>>4:replaceWith]);
            drawPixel(x+1+j*2, y+i, palette[((d&0x0F)!=transparentColor)?d&0x0F:replaceWith]);
        }
}

void LCD::drawBitmap565File(FIL* file, int x, int y, uint16_t w, uint16_t h, size_t Xoffset)
{

    uint16_t buff[w];
    setWindow(y, x, y+h-1, x+w-1);

    SET_MASK_P2;
    CLR_CS_SET_CD_RD_WR;
    for(size_t j=0;j<h;j++)
    {
        size_t count=0;
        f_read(file, buff, w*2, &count);
        if(Xoffset)
            f_lseek(file, file->fptr + Xoffset*2);
        uint16_t *ptr=buff;
        for(auto i=0;i<w>>2;i++)
        {
            LPC_GPIO_PORT->MPIN[2] = (*ptr++)<<3;
            CLR_WR;
            SET_WR;
            LPC_GPIO_PORT->MPIN[2] = (*ptr++)<<3;
            CLR_WR;
            SET_WR;
            LPC_GPIO_PORT->MPIN[2] = (*ptr++)<<3;
            CLR_WR;
            SET_WR;
            LPC_GPIO_PORT->MPIN[2] = (*ptr++)<<3;
            CLR_WR;
            SET_WR;
        }

    }
    
    CLR_MASK_P2;
    SET_CS;
    setWindow(0, 0, HEIGHT-1, WIDTH-1);
}

void LCD::drawChar(int x, int y, uint8_t c)
{
	for(auto i=0; i < m_fontWidth; i++ ) {
        uint8_t line;
        if(i == m_fontWidth)
            line = 0x0;
        else
            line = *(font+((c-32)*m_fontWidth)+i);
                
        for(auto j=0; j < m_fontHeight; j++) {
            if(line & 0x1)
            {
                auto xx=x+i;
                auto yy=y+j;

                drawPixel(xx, yy, color);

            }   
            line >>= 1;
        }
    }
}

void LCD::write(uint8_t c)
{
    if (c == '\n') {
    cursorY += m_fontHeight+1;
    cursorX = 0;
    } else if (c == '\r') {
        // skip em
    } else {
        drawChar(cursorX, cursorY, c);
        cursorX += m_fontWidth+1;
        if ((cursorX > (WIDTH - m_fontWidth-1))) {
            cursorY +=  m_fontHeight+1;
            cursorX = 0;
        }
    }
}

void LCD::print(const char txt[])
{
    while (*txt)
        write(*txt++);
}

void LCD::printWraped(size_t margin, size_t width ,const char txt[])
{
    cursorX = margin;
    while (*txt)
    {
        char c=*txt++;
        if (c == '\n') {
        cursorY += m_fontHeight+1;
        cursorX = margin;
        } else if (c == '\r') {
            // skip em
        } else {
            drawChar(cursorX, cursorY, c);
            cursorX += m_fontWidth+1;
            if ((cursorX > (width+margin - m_fontWidth-1))) {
                cursorY +=  m_fontHeight+1;
                cursorX = margin;
            }
        }
    }
}

void LCD::printNumber(uint32_t n)
{
  uint8_t buf[16 * sizeof(uint32_t)]; // Assumes 8-bit chars.
  uint32_t i = 0;

  if (n == 0) {
    write('0');
    return;
  }

  while (n > 0) {
    buf[i++] = n % 10;
    n /= 10;
  }

  for (; i > 0; i--)
    write((uint8_t) (buf[i - 1] < 10 ?
      '0' + buf[i - 1] :
      'A' + buf[i - 1] - 10));
} 