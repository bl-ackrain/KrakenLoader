#include "Buttons.h"
#include <LPC11U6x.h>

inline bool aBtn(){ return *((volatile char*)(0xA0000000 + 1*0x20 + 9)); };
inline bool bBtn(){ return *((volatile char*)(0xA0000000 + 1*0x20 + 4)); };
inline bool cBtn(){ return *((volatile char*)(0xA0000000 + 1*0x20 + 10)); };
inline bool upBtn(){ return *((volatile char*)(0xA0000000 + 1*0x20 + 13)); };
inline bool downBtn(){ return *((volatile char*)(0xA0000000 + 1*0x20 + 3)); };
inline bool leftBtn(){ return *((volatile char*)(0xA0000000 + 1*0x20 + 25)); };
inline bool rightBtn(){ return *((volatile char*)(0xA0000000 + 1*0x20 + 7)); };


std::uint32_t Buttons::states[Buttons::NUM_BTN];

void Buttons::Init()
{
    //enable GPIO AHB clock
    LPC_SYSCON->SYSAHBCLKCTRL |= ((1 << 19) | (1 << 16) | (1 << 7) | (1 << 30));

    //Disable pin interrupts
    LPC_SYSCON->STARTERP0 = 0;
    LPC_SYSCON->PINTSEL[0] = 0;
    LPC_SYSCON->PINTSEL[1] = 0;
    LPC_SYSCON->PINTSEL[2] = 0;
    LPC_SYSCON->PINTSEL[3] = 0;
    LPC_SYSCON->PINTSEL[4] = 0;
    LPC_SYSCON->PINTSEL[5] = 0;
    LPC_SYSCON->PINTSEL[6] = 0;

    NVIC->ICER[0] = (1 << ((std::uint32_t)(0) & 0x1F));
    NVIC->ICER[0] = (1 << ((std::uint32_t)(1) & 0x1F));
    NVIC->ICER[0] = (1 << ((std::uint32_t)(2) & 0x1F));
    NVIC->ICER[0] = (1 << ((std::uint32_t)(3) & 0x1F));
    NVIC->ICER[0] = (1 << ((std::uint32_t)(4) & 0x1F));
    NVIC->ICER[0] = (1 << ((std::uint32_t)(5) & 0x1F));
    NVIC->ICER[0] = (1 << ((std::uint32_t)(6) & 0x1F));

    // Clear interrupts
    std::uint32_t* a;
    a = (std::uint32_t*)0xA000407F;
    *a = 0;
    //if (!(LPC_GPIO_X->ISEL & ch_bit))
    LPC_PINT->IST = 0x0;

    for(auto i = 0; i < NUM_BTN; i++ )
        states[i]=0;
}

void Buttons::update()
{
    bool s[NUM_BTN];

    s[BTN_LEFT] = leftBtn();
    s[BTN_UP] = upBtn();
    s[BTN_RIGHT] = rightBtn();
    s[BTN_DOWN] = downBtn();
    s[BTN_A] = aBtn();
    s[BTN_B] = bBtn();
    s[BTN_C] = cBtn();

    for (std::uint16_t thisButton = 0; thisButton < NUM_BTN; thisButton++) {
        if (s[thisButton]) { //if button pressed
            states[thisButton]++; //increase button hold time
        } else {
            if (states[thisButton] == 0)//button idle
                continue;
            if (states[thisButton] == 0xFFFFFF)//if previously released
                states[thisButton] = 0; //set to idle
            else
                states[thisButton] = 0xFFFFFF; //button just released
        }    
    }
}

bool Buttons::pressed(std::uint8_t button) {
    if (states[button] == 1)
        return true;
    else
        return false;
}

bool Buttons::released(std::uint8_t button) {
    if (states[button] == 0xFFFFFF)
        return true;
    else
        return false;
}

bool Buttons::held(std::uint8_t button, std::uint32_t time){
    if(states[button] == (time+1))
        return true;
    else
        return false;
}

bool Buttons::repeat(std::uint8_t button, std::uint32_t period) {
    if (period <= 1) {
        if ((states[button] != 0xFFFFFF) && (states[button]))
            return true;
    } else {
        if ((states[button] != 0xFFFFFF) && ((states[button] % period) == 1))
            return true;
    }
    return false;
}

std::uint32_t Buttons::timeHeld(std::uint8_t button){
    if(states[button] != 0xFFFFFF)
        return states[button];
    else
        return 0;
    
}