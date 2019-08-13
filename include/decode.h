#ifndef DEOCDE_H
#define DEOCDE_H

#include <stdbool.h>
// clang-format off
#include <lint.h>
// clang-format on

extern struct res
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
extern struct res result;
extern volatile unsigned char timeout, rx;
extern __bit complete;

void reset_recv();
void reset_result();

void decode();
_Bool sirc_type();

#ifdef ISR
extern void tf2() __interrupt(5);
extern void ie1() __interrupt(IE1_VECTOR);
#undef ISR
#endif

#endif