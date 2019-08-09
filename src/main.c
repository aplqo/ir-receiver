// clang-format off
#include<lint.h>
// clang-format on
#include <8052.h>
#include <stdbool.h>
#include "lcd.h"

#define comp_dif 3

struct config
{
    unsigned char start; //start bit
    unsigned char num[2];
};
#define NEC_COMMAND 24
#define NEC_COMMAND_REV 32
#define NEC_ADDRESS 8
#define NEC_ADDRESS_REV 16
#define SIRC_COMMAND 7
#define SIRC_ADDRESS 15
const __code struct config conf[2] = {
    { 135, { 11, 23 } },
    { 30, { 12, 18 } }
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
volatile unsigned char rx, timeout;

__bit deco = 0, complete; // if is decoding
unsigned char digit;

void send(unsigned char);

#ifdef DEBUG
void sec(unsigned char num)
{
    for (unsigned char i = 0; i < 3; i++)
    {
        send(0x00);
    }
    send(num);
    send(0x00);
}
#endif

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
        TI = 1;
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
#ifdef DEBUG_EQUAL
    sec(0xee);
    send(a);
    send(b);
#endif
    unsigned char b1 = b + comp_dif;
    unsigned char b2 = b - comp_dif;
    return (a <= b1 && a >= b2);
}
void reset()
{
    EA = 0;

    TR0 = 0;
    TL0 = TH0;
    current = 0;

    result.type = NUL;
    result.user = 0x00;
    result.key = 0x00;
    digit = 0;

    deco = 0;
    complete = 0;

    rx = 0x00;
    timeout = 0x00;
    EA = 1;
#ifdef DEBUG_RESET
    sec(0x01);
#endif
}
void decode_sirc(unsigned char b)
{
    if (digit < SIRC_COMMAND)
    {
        b <<= digit;
        result.key |= b;
    }
    else if (digit < SIRC_ADDRESS)
    {
        b <<= digit - SIRC_ADDRESS;
        result.user |= b;
    }
    digit++;
    if (digit == SIRC_ADDRESS)
    {
        EA = 0;
        deco = 0;
        complete = 1;
#ifdef DEBUG_SIRC
        sec(0xcc);
        send(0x0c);
        send(result.user);
        send(result.key);
#endif
    }
}
void decode_nec(unsigned char b)
{
    static unsigned char rev[2];
    if (digit < NEC_ADDRESS)
    {
        b <<= digit;
        result.user |= b;
    }
    else if (digit < NEC_ADDRESS_REV)
    {
        b <<= digit - NEC_ADDRESS_REV;
        rev[0] |= b;
    }
    else if (digit < NEC_COMMAND)
    {
        b <<= digit - NEC_COMMAND;
        result.key |= b;
    }
    else if (digit < NEC_COMMAND_REV)
    {
        b <<= digit - NEC_COMMAND_REV;
        rev[1] |= b;
    }
    digit++;
    if (digit == NEC_COMMAND_REV)
    {
        EA = 0;
#ifdef DEBUG_NEC
        sec(0xcc);
        send(0x0e);
        send(result.user);
        send(rev[0]);
        send(result.key);
        send(rev[1]);
#endif
        if ((~rev[0]) != result.user)
        {
            reset();
            return;
        }
        if ((~rev[1]) != result.key)
        {
            reset();
            return;
        }
        deco = 0;
        complete = 1;
    }
}
unsigned char decode_bit(unsigned char dif)
{
    const struct config* c = conf + result.type;
    if (equal(dif, c->num[0]))
    {
        return 0;
    }
    else if (equal(dif, c->num[1]))
    {
        return 1;
    }
    return 2;
}
void decode()
{
    if (!deco)
    {
        deco = 1;
        return;
    }
    unsigned char t = tim;
#ifdef DEBUG_TIM
    sec(0xdd);
    send(t);
#endif
    if (result.type == NUL) //decode start bit
    {
        if (equal(t, conf[SIRC].start))
        {
            result.type = SIRC;
        }
        else if (equal(t, conf[NEC].start))
        {
            result.type = NEC;
        }
        else
        {
            reset();
        }
        return;
    }
    unsigned char tmp = decode_bit(t);
    if (tmp == 2)
    {
        reset();
        return;
    }
    switch (result.type)
    {
    case NEC:
        decode_nec(tmp);
        break;
    case SIRC:
        decode_sirc(tmp);
        break;
    default:
        break;
    }
}
void send(unsigned char dat)
{
    while (!TI)
        ;
    TI = 0;
    SBUF = dat;
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
            rx = 0x00;
        }
        if (timeout)
        {
#ifdef DEBUG_TIM
            sec(0xaa);
#endif
            reset();
        }
        if (complete)
        {
            update();
            send(result.send);
            reset();
        }
    }
}

void tf0() __interrupt(TF0_VECTOR)
{
    current++;
    if (current == 0)
    {
        timeout = 0xff;
    }
}
void ie1() __interrupt(IE1_VECTOR)
{
    tim = current;
    TL0 = TH0;
    current = 0;
    TR0 = 1;
    rx = 0xff;
}
