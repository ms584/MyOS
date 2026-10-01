#ifndef MYOS_SDK_CTYPE_H
#define MYOS_SDK_CTYPE_H
/* ASCII digit classification needed by format parsing, not full locale ctype. */
static inline int isdigit(int value){return (unsigned)value-'0'<10;}
#endif
