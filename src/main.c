// clang-format off
#include<lint.h>
// clang-format on
#define ISR
#include <8052.h>
#include <stdbool.h>
#include "lcd.h"
#include "decode.h"
#include "serial.h"
#include "debug.h"
#include "util.h"

#ifdef STC11F
__sfr __at(0x9c) BRT;
__sfr __at(0x8e) AUXR;
#endif

void init()
{
    P0 = 0xff;
    //timer init
    {
        TMOD = 0x20;
#ifdef STC89C
        TH2 = 0xff;
        TL2 = 0xdc;
        RCAP2H = 0xff;
        RCAP2L = 0xdc;
        T2CON = 0x34;
#endif
        TH1 = 0xa4;
        TL1 = 0xa4;
    }
#ifdef STC11F
    P1_0 = 0; // close ir led
#endif
    {
        SCON = 0x40; // mode 1
#ifdef STC11F
        BRT = 250;
        AUXR = 0x11;
#endif
        TI = 1;
    }
    //interrupt init
    {
        IT1 = 1;
        EX1 = 1;
        ET1 = 1;
    }
#ifdef DISPLAY
    // lcd init
    {
        lcd_write(0, 0x38);
        lcd_write(0, 0x0c);
        lcd_write(0, 0x06);
        lcd_write(0, 0x01);
        display_str(0x07, 5, "Send:");
        display_str(0x40, 4, "Key:");
        display_str(0x48, 5, "User:");
    }
#endif
}

#ifdef DISPLAY
void update(unsigned char s)
{
    switch (result.type)
    {
    case NUL:
        return;
    case NEC:
        display_str(0x00, 4, "NEC ");
        break;
    case SIRC:
        display_str(0x00, 4, "SIRC");
        break;
    }
    if (s)
    {
        display_uchar(0x0c, result.send);
    }
    else
    {
        display_str(0x0c, 2, "--");
    }
    display_uchar(0x44, result.key);
    display_uchar(0x4d, result.user);
}
#endif
void finish()
{
#ifndef DISABLE_REPEAT
    static unsigned char ign, count;
    if (result.type == SIRC)
    {
        count++;
        if (count == 1)
        {
            ign = getRepeat();
            goto ok;
        }
        if (count == ign)
        {
            count = 0;
        }
        reset_result();
        return;
    }
ok:;
#endif
    conv();
    _Bool f = filter();
    if (f)
        send(result.send);
#ifdef DISPLAY
    update(f);
#endif
    reset_result();
}

void main()
{
    init();
    reset_recv();
    reset_result();
    while (1)
    {
        if (rx)
        {
            decode();
            if (complete)
            {
                finish();
            }
        }
        else if (timeout)
        {
            reset_recv();
            if (sirc_type())
            {
                finish();
                continue;
            }
            reset_result();
        }
    }
}