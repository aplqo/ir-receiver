// clang-format off
#include <lint.h>
// clang-format on
#include <8052.h>
#include <stdbool.h>
#include "serial.h"
#include "debug.h"

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

struct res
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
};
struct res result;
unsigned char rev[2];
unsigned char digit;
unsigned char mask = 0x01;

volatile unsigned char tim[buf_size];
volatile unsigned char rx_pos = 0;
unsigned char decode_pos = 0;
volatile unsigned char current = 1;

volatile unsigned char rx = 0x00, timeout = 0x00;
__bit deco = 0, complete = 0; // if is decoding

_Bool equal(unsigned char a, unsigned char b, unsigned char dif)
{
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

    TR1 = 0;
    TL1 = TH1;
    TF1 = 0;
    current = 1;
    rx_pos = 0;
    decode_pos = 0;

    EA = 1;
#ifdef led
    led = 1;
#endif
    timeout = 0x00;
    deco = 0;
    rx = 0x00;
}
void reset_result()
{
    result.type = NUL;
    result.user = 0x00;
    result.key = 0x00;

    rev[0] = 0x00;
    rev[1] = 0x00;

    digit = 0;
    mask = 0x01;

    complete = 0;
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
        complete = 1;
        reset_recv();
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
        if ((rev[0] ^ result.user) != 0xff)
        {
            reset_result();
            return;
        }
        if ((rev[1] ^ result.key) != 0xff)
        {
            reset_result();
            return;
        }
        complete = 1;
    }
}
_Bool sirc_type()
{
    if ((result.type == SIRC) && (digit == 11))
    {
        complete = 1;
        return true;
    }
    return false;
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
    if (decode_pos == rx_pos)
        rx = 0x00;
    if (!deco)
    {
        deco = 1;
#ifdef led
        led = 0;
#endif
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
        if (sirc_type())
            return;
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
void tf1() __interrupt(TF1_VECTOR) //tf2 vector
{
    current++;
    if (current == 0)
    {
        timeout = 0xff;
    }
}
void ie1() __interrupt(IE1_VECTOR)
{
    tim[rx_pos] = current;
    TR1 = 0;
    TL1 = TH1;
    current = 1;
    TR1 = 1;
    rx_pos++;
    if (rx_pos == buf_size)
    {
        rx_pos = 0;
    }
    rx = 0xff;
}