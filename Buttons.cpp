#include "Buttons.h"


DigitalIn Buttons::ABtn(P1_9);
DigitalIn Buttons::BBtn(P1_4);
DigitalIn Buttons::CBtn(P1_10);
DigitalIn Buttons::UpBtn(P1_13);
DigitalIn Buttons::DownBtn(P1_3);
DigitalIn Buttons::LeftBtn(P1_25);
DigitalIn Buttons::RightBtn(P1_7);

uint32_t Buttons::states[Buttons::NUM_BTN];

void Buttons::Init()
{
    //enable GPIO AHB clock
    LPC_SYSCON->SYSAHBCLKCTRL |= ((1 << 19) | (1 << 16) | (1 << 7));

    //Disable pin interrupts
    LPC_SYSCON->STARTERP0 = 0;
    LPC_SYSCON->PINTSEL[0] = 0;
    LPC_SYSCON->PINTSEL[1] = 0;
    LPC_SYSCON->PINTSEL[2] = 0;
    LPC_SYSCON->PINTSEL[3] = 0;
    LPC_SYSCON->PINTSEL[4] = 0;
    LPC_SYSCON->PINTSEL[5] = 0;
    LPC_SYSCON->PINTSEL[6] = 0;

    NVIC->ICER[0] = (1 << ((uint32_t)(0) & 0x1F));
    NVIC->ICER[0] = (1 << ((uint32_t)(1) & 0x1F));
    NVIC->ICER[0] = (1 << ((uint32_t)(2) & 0x1F));
    NVIC->ICER[0] = (1 << ((uint32_t)(3) & 0x1F));
    NVIC->ICER[0] = (1 << ((uint32_t)(4) & 0x1F));
    NVIC->ICER[0] = (1 << ((uint32_t)(5) & 0x1F));
    NVIC->ICER[0] = (1 << ((uint32_t)(6) & 0x1F));

    // Clear interrupts
    uint32_t* a;
    a = (uint32_t*)0xA000407F;
    *a = 0;
    //if (!(LPC_GPIO_X->ISEL & ch_bit))
    LPC_PINT->IST = 0x0;

    for(auto i = 0; i < NUM_BTN; i++ )
        states[i]=0;
}

void Buttons::update()
{
    uint16_t s[NUM_BTN];

    s[BTN_LEFT] = LeftBtn;
    s[BTN_UP] = UpBtn;
    s[BTN_RIGHT] = RightBtn;
    s[BTN_DOWN] = DownBtn;
    s[BTN_A] = ABtn;
    s[BTN_B] = BBtn;
    s[BTN_C] = CBtn;

    for (uint16_t thisButton = 0; thisButton < NUM_BTN; thisButton++) {
        if (s[thisButton]==1) { //if button pressed
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

bool Buttons::pressed(uint8_t button) {
    if (states[button] == 1)
        return true;
    else
        return false;
}

bool Buttons::released(uint8_t button) {
    if (states[button] == 0xFFFFFF)
        return true;
    else
        return false;
}

bool Buttons::held(uint8_t button, uint32_t time){
    if(states[button] == (time+1))
        return true;
    else
        return false;
}

bool Buttons::repeat(uint8_t button, uint32_t period) {
    if (period <= 1) {
        if ((states[button] != 0xFFFFFF) && (states[button]))
            return true;
    } else {
        if ((states[button] != 0xFFFFFF) && ((states[button] % period) == 1))
            return true;
    }
    return false;
}

uint32_t Buttons::timeHeld(uint8_t button){
    if(states[button] != 0xFFFFFF)
        return states[button];
    else
        return 0;
    
}