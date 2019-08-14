#if (!defined DEBUG_H) && (defined DEBUG)
#define DEBUG_H

void sec(unsigned char num);

/*---macros for debug---*/
#define DEBUG_SIRC
#define DEBUG_NEC
#define DEBUG_TIM
#define DEBUG_ERR
#define DEBUG_RESET
#define DEBUG_EQUAL
#define DEBUG_CONV
#define DEBUG_CONV_DAT
#define DEBUG_REPEAT

#endif