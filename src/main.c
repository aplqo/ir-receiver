// clang-format off
#include<lint.h>
// clang-format on
#include <8052.h>
#include <stdbool.h>
#include "lcd.h"

#define buf_size 34

struct config
{
    unsigned char start; //start bit
    unsigned char num[2];
    unsigned char dif;
};
#define NEC_COMMAND 24
#define NEC_COMMAND_REV 32
#define NEC_ADDRESS 8
#define NEC_ADDRESS_REV 16
#define SIRC_COMMAND 7
#define SIRC_ADDRESS 15
const __code struct config conf[2] = {
    { 135, { 11 + 1, 23 + 1 }, 5 },
    { 30, { 12 + 1, 18 + 1 }, 2 }
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
unsigned char count = 0;
unsigned char rev[2];
unsigned char digit;
unsigned char mask = 0x01;

volatile unsigned char tim[buf_size];
volatile unsigned char rx_pos = 0;
unsigned char decode_pos = 0;
volatile unsigned char current = 1;

volatile unsigned char rx = 0x00, timeout = 0x00;
__bit deco = 0, complete; // if is decoding

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
_Bool equal(unsigned char a, unsigned char b, unsigned char dif)
{
#ifdef DEBUG_EQUAL
    sec(0xee);
    send(a);
    send(b);
#endif
    unsigned char b1 = b + dif;
    unsigned char b2 = b - dif;
    return (a <= b1 && a >= b2);
}
inline void rr()
{
    mask <<= 1;
}
void reset_recv()
{
    EA = 0;

    TR2 = 0;
    TL2 = RCAP2L;
    TH2 = RCAP2H;
    TF2 = 0;
    current = 1;
    rx_pos = 0;
    decode_pos = 0;

    EA = 1;
    timeout = 0x00;
    deco = 0;
#ifdef DEBUG_RESET
    sec(0x01);
    send(0xc0);
#endif
}
void reset_result()
{
    result.type = NUL;
    result.user = 0x00;
    result.key = 0x00;
    count = 0;

    rev[0] = 0x00;
    rev[1] = 0x00;

    digit = 0;
    mask = 0x01;

    complete = 0;

#ifdef DEBUG_RESET
    sec(0x01);
    send(0xe0);
#endif
}
void decode_sirc(unsigned char b)
{
    if (digit < SIRC_COMMAND)
    {
        result.key |= b;
    }
    else if (digit < SIRC_ADDRESS)
    {
        result.user |= b;
        if ((digit == 12 - 2) || (digit == 15 - 2))
        {
            rr();
            while (current < 9 + 1)
                ;
            b = P3_3 ? mask : 0x00;
            result.user |= b;
        }
        if (digit == 15 - 2)
            digit++;
    }
    digit++;
    if (digit == SIRC_COMMAND)
    {
        mask = 0x01;
        return;
    }
    rr();
    if (digit == SIRC_ADDRESS)
    {
        count++;
        if (count == 2)
            complete = 1;
        reset_recv();
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
    if (digit < NEC_ADDRESS)
    {
        result.user |= b;
    }
    else if (digit < NEC_ADDRESS_REV)
    {
        rev[0] |= b;
    }
    else if (digit < NEC_COMMAND)
    {
        result.key |= b;
    }
    else if (digit < NEC_COMMAND_REV)
    {
        rev[1] |= b;
    }
    digit++;
    if ((digit % 8) == 0)
    {
        mask = 0x01;
    }
    else
    {
        rr();
    }
    if (digit == NEC_COMMAND_REV)
    {
        reset_recv();
#ifdef DEBUG_NEC
        sec(0xcc);
        send(0x0e);
        send(result.user);
        send(rev[0]);
        send(result.key);
        send(rev[1]);
#endif
        if ((rev[0] ^ result.user) != 0xff)
        {
            send(0xe1);
            reset_result();
            return;
        }
        if ((rev[1] ^ result.key) != 0xff)
        {
            send(0xe2);
            reset_result();
            return;
        }
        complete = 1;
    }
}
unsigned char decode_bit(unsigned char dif)
{
    const struct config* c = conf + result.type;

    if (equal(dif, c->num[0], c->dif))
    {
        return 0x00;
    }
    else if (equal(dif, c->num[1], c->dif))
    {
        return mask;
    }
    return 0xff;
}
void decode()
{
    unsigned char t = tim[decode_pos];
    decode_pos++;
    if (decode_pos == buf_size)
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
        if (equal(t, conf[SIRC].start, conf[SIRC].dif))
        {
            result.type = SIRC;
        }
        else if (equal(t, conf[NEC].start, conf[NEC].dif))
        {
            result.type = NEC;
        }
        else
        {
            reset_recv();
            reset_result();
        }
        return;
    }
    unsigned char tmp = decode_bit(t);
    if (tmp == 0xff)
    {
        reset_recv();
        if ((result.type == SIRC) && (digit == 11))
        {
#ifdef DEBUG_SIRC
            sec(0xcc);
            send(0x0c);
            send(result.user);
            send(result.key);
#endif
            count++;
            if (count == 2)
                complete = 1;
            return;
        }
        reset_result();
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
void finish()
{
    update();
    send(result.send);
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
            if (decode_pos == rx_pos)
            {
                rx = 0x00;
            }
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
            if ((result.type == SIRC) && (digit == 11))
            {
                count++;
                if (count == 2)
                    finish();
                continue;
            }
            reset_result();
            timeout = 0x00;
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
    TR2 = 0;
    TL2 = RCAP2L;
    TH2 = RCAP2H;
    current = 1;
    TF2 = 0;
    TR2 = 1;
    rx_pos++;
    if (rx_pos == buf_size)
    {
        rx_pos = 0;
    }
    rx = 0xff;
}
