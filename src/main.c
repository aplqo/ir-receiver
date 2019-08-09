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
volatile unsigned char tim[32];
volatile unsigned char rx_pos = 0;
unsigned char decode_pos = 0;
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

    TR2 = 0;
    TL2 = RCAP2L;
    TH2 = RCAP2H;
    current = 0;
    rx_pos = 0;
    decode_pos = 0;

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
        if (digit == SIRC_ADDRESS - 2)
        {
            while (current < 9)
                ;
            if (P3_3)
            {
                b = 1;
            }
            else
            {
                b = 0;
            }
            b <<= SIRC_ADDRESS - 1 - SIRC_COMMAND;
            result.user |= b;
            digit++;
        }
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
        if (digit == (NEC_COMMAND_REV - 2))
        {
            while (current < 13)
                ;
            if (P3_3)
            {
                b = 1;
            }
            else
            {
                b = 0;
            }
            b <<= NEC_COMMAND_REV - 1 - NEC_COMMAND;
            rev[1] |= b;
            digit++;
        }
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
    unsigned char t = tim[decode_pos];
    decode_pos++;
    if (decode_pos == 32)
        decode_pos = 0;
#ifdef DEBUG_TIM
    sec(0xdd);
    send(t);
    send(digit);
#endif
    if (!deco)
    {
        deco = 1;
        return;
    }
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
            if (decode_pos == rx_pos)
            {
                rx = 0x00;
            }
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

void tf2() __interrupt(5) //tf2 vector
{
    TF2 = 0;
    current++;
    if (current == 0)
    {
        timeout = 0xff;
    }
}
void ie1() __interrupt(IE1_VECTOR)
{
    tim[rx_pos] = current;
    TL2 = RCAP2L;
    TH2 = RCAP2H;
    current = 0;
    TR2 = 1;
    rx_pos++;
    if (rx_pos == 32)
    {
        rx_pos = 0;
    }
    rx = 0xff;
}
