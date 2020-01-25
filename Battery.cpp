#include "Battery.h"
#include <LPC11U6x.h>

std::uint32_t Battery::m_average;

void Battery::init()
{
    LPC_IOCON->PIO0_23 |= 0x1;     //set up pin for ADC use
    LPC_IOCON->PIO0_23 &= ~(1 << 7);

    volatile std::uint32_t tmp = (LPC_SYSCON->PDRUNCFG & 0x000025FFL);
    tmp &= ~((1 << 4) & 0x000025FFL);
    LPC_SYSCON->PDRUNCFG = (tmp | 0x0000C800L);

    // Enable clock for ADC
    LPC_SYSCON->SYSAHBCLKCTRL |= (1 << 13);

    // Determine the clock divider for a 500kHz ADC clock during calibration
    std::uint32_t clkdiv = (SystemCoreClock / 500000) - 1;
    
    // Perform a self-calibration
    LPC_ADC->CTRL = (1 << 30) | (clkdiv & 0xFF);
    while ((LPC_ADC->CTRL & (1 << 30)) != 0);

    // Sampling clock: SystemClock divided by 1
    LPC_ADC->CTRL = 0;

    m_average=0;
    for(auto i=0;i<50;i++)
        m_average+= readADC();

    m_average/=50;
}

std::uint32_t Battery::readADC()
{
    // select channel
    LPC_ADC->SEQA_CTRL &= ~(0xFFF);
    LPC_ADC->SEQA_CTRL |= (1 << 1);

    // start conversion, sequence enable with async mode
    LPC_ADC->SEQA_CTRL |= ((1 << 26) | (1 << 31) | (1 << 19));
    
    // Repeatedly get the sample data until DONE bit
    volatile std::uint32_t data;
    do
        data = LPC_ADC->SEQA_GDAT;
    while
        ((data & (1 << 31)) == 0);

    data = (LPC_ADC->DAT[1] >>4 ) & 0xFFF;
    
    // Stop conversion
    LPC_ADC->SEQA_CTRL &= ~(1 << 31);

    return data;
}

static inline void order(std::uint32_t &a, std::uint32_t &b) {
    if (a > b) {
        std::uint32_t t = a;
        a = b;
        b = t;
    }
}

std::uint8_t Battery::getPercentage()
{
    std::uint32_t value = 0;
    for(auto i=0;i<50;i++)
        value+= readADC();

    value/=50;

    m_average = ( m_average * 29 + value ) /30;

    std::uint32_t vBattery=(m_average*660)/0xFFF;
    if(vBattery < 335)
        vBattery = 335;

    std::uint8_t  batPercentage=((vBattery-335)*100)/(412-335);

    if(batPercentage > 100)
        batPercentage = 100;

    return batPercentage;
}