// clang-format off
#include<lint.h>
// clang-format on
#include <8052.h>
#include <stdbool.h>
void send(unsigned char dat)
{
    while (!TI)
        ;
    TI = 0;
    SBUF = dat;
}