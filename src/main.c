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
#include "conv.h"
#include "repeat.h"

void init()
{
    P0 = 0xff;
    //timer init
    {
        TMOD = 0x22;
        TH2 = 0xff;
        TL2 = 0xa4;
        RCAP2H = 0xff;
        RCAP2L = 0xa4;
        TH1 = 0xfd;
        TL1 = 0xfd;
    }
    {
        SCON = 0x40; // mode 1
        TI = 1;
    }
    //interrupt init
    {
        IT1 = 1;
        EX1 = 1;
        ET2 = 1;
    }
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
    TR1 = 1;
}

void update()
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
    display_uchar(0x0c, result.send);
    display_uchar(0x44, result.key);
    display_uchar(0x4d, result.user);
}
void finish()
{
#ifndef DISABLE_REPEAT
    static unsigned char ign, count;
    if (result.type == SIRC)
    {
#ifdef DEBUG_IGNORE
        sec(0x33);
        send(ign);
        send(count);
#endif
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
    send(result.send);
    update();
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
#ifdef DEBUG_TIM
            sec(0xaa);
#endif
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