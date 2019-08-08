// clang-format off
#include<lint.h>
// clang-format on
#include <8052.h>
#include <stdbool.h>
#include "lcd.h"

#define comp_dif 20

struct config
{
    unsigned char start; //start bit
    unsigned char num[2];
    unsigned char address[2];
    unsigned char command[2];
};
const __code struct config conf[2] = {
    { 135, { 11, 23 }, { 8, 16 }, { 24, 32 } },
    { 30, { 12, 18 }, { 5, 5 }, { 12, 12 } }
};

struct
{
    enum
    {
        NEC = 0,
        SIRC = 1,
        NUL
    } type;
    unsigned char user;
    unsigned char key;
    unsigned char send;
} result;
volatile unsigned char tim;
volatile unsigned char current = 0;
volatile unsigned char rx;

__bit deco = 0, complete; // if is decoding
unsigned char digit;

void init()
{
    P0 = 0xff;
    //timer init
    {
        TMOD = 0x22;
        TH0 = 164;
        TL0 = 164;
        TH1 = 0xfd;
        TL1 = 0xfd;
    }
    {
        SCON = 0x40; // mode 1
    }
    //interrupt init
    {
        IT1 = 1;
        EX1 = 1;
        ET0 = 1;
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
    EA = 1;
    TR0 = 1;
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
    display_uchar(0x0a, result.send);
    display_uchar(0x44, result.key);
    display_uchar(0x4c, result.user);
}
_Bool equal(unsigned char a, unsigned char b)
{
    unsigned char b1 = b + comp_dif;
    unsigned char b2 = b - comp_dif;
    return (a >= b1 && a <= b2);
}
void reset()
{
    result.type = NUL;
    result.user = 0x00;
    result.key = 0x00;
    digit = 0;
    deco = 0;
    complete = 0;
}
void decode()
{
    static unsigned char last;
    unsigned char rev[2];
    if (!deco)
    {
        deco = 1;
        last = tim;
        return;
    }
    unsigned char dif = tim > last ? tim - last : 255 - last + tim;
    last = tim;
    if (result.type == NUL) //decode start bit
    {
        if (equal(dif, conf[SIRC].start))
        {
            result.type = SIRC;
        }
        else if (equal(dif, conf[NEC].start))
        {
            result.type = NEC;
        }
        else
        {
            deco = 0;
        }
        return;
    }
    unsigned char tmp, shif;
    const struct config* c = conf + result.type;
    if (equal(dif, c->num[0]))
    {
        tmp = 0;
    }
    else if (equal(dif, c->num[1]))
    {
        tmp = 1;
    }
    else
    {
        reset();
        return;
    }
    if (digit < (c->address[0]))
    {
        shif = (c->address[0]) - digit;
        tmp <<= shif;
        result.user |= tmp;
    }
    else if (digit < (c->address[1]))
    {
        shif = (c->address[1]) - digit;
        tmp <<= shif;
        rev[0] |= tmp;
    }
    else if (digit < (c->command[0]))
    {
        shif = (c->command[0]) - digit;
        tmp <<= shif;
        result.key |= tmp;
    }
    else if (digit < (c->command[1]))
    {
        shif = (c->command[1]) - digit;
        tmp <<= shif;
        rev[1] |= tmp;
    }
    digit++;
    if (digit == (c->command[1]))
    {
        //check sum
        {
            if ((c->address[0]) != (c->address[1]))
            {
                if ((~rev[0]) != result.user)
                {
                    reset();
                    return;
                }
            }
            if ((c->command[0]) != (c->command[1]))
            {
                if ((~rev[1]) != result.key)
                {
                    reset();
                    return;
                }
            }
        }
        deco = 0;
        complete = 1;
    }
}

void main()
{
    init();
    reset();
    while (1)
    {
        if (rx)
        {
            decode();
        }
        if (complete)
        {
            while (TI)
                ;
            SBUF = result.send;
            while (TI)
                ;
            update();
            reset();
        }
    }
}

void tf0() __interrupt(TF0_VECTOR)
{
    current++;
}
void ie1() __interrupt(IE1_VECTOR)
{
    tim = current;
    rx = 0xff;
}
