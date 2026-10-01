#ifndef MYOS_SDK_CTYPE_H
#define MYOS_SDK_CTYPE_H
/* Guest C-locale classification. Arguments: EOF or an unsigned-char value. */
static inline int isdigit(int value){return (unsigned)value-'0'<10;}
static inline int islower(int value){return (unsigned)value-'a'<26;}
static inline int isupper(int value){return (unsigned)value-'A'<26;}
static inline int isalpha(int value){return islower(value)||isupper(value);}
static inline int isalnum(int value){return isalpha(value)||isdigit(value);}
static inline int isspace(int value){return value==' '||(unsigned)value-'\t'<5;}
static inline int isblank(int value){return value==' '||value=='\t';}
static inline int iscntrl(int value){return (unsigned)value<32||value==127;}
static inline int isprint(int value){return (unsigned)value-32<95;}
static inline int isgraph(int value){return (unsigned)value-33<94;}
static inline int ispunct(int value){return isgraph(value)&&!isalnum(value);}
static inline int isxdigit(int value){return isdigit(value)||(unsigned)value-'a'<6||(unsigned)value-'A'<6;}
static inline int tolower(int value){return isupper(value)?value+('a'-'A'):value;}
static inline int toupper(int value){return islower(value)?value-('a'-'A'):value;}
#endif
