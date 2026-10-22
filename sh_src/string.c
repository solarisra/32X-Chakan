#include "mars.h"
#include "string.h"

__attribute__((used,externally_visible))
void *memset(void *destination,int value,size_t count) {
    volatile unsigned char *out=destination;
    while (count--) *out++=(unsigned char)value;
    return destination;
}

/* GCC can emit this for structure copies even in a freestanding build. */
__attribute__((used,externally_visible))
void *memcpy(void *destination,const void *source,size_t count) {
    typedef uint32_t __attribute__((may_alias)) CopyWord;
    volatile unsigned char *out=destination;
    const unsigned char *in=source;
    if ((((uintptr_t)out|(uintptr_t)in)&3)==0) {
        volatile CopyWord *words_out=(volatile CopyWord *)out;
        const CopyWord *words_in=(const CopyWord *)in;
        while(count>=4) { *words_out++=*words_in++; count-=4; }
        out=(volatile unsigned char *)words_out;
        in=(const unsigned char *)words_in;
    }
    while(count--) *out++=*in++;
    return destination;
}

size_t strlen(const char *str) {
	const char *src = str;
	while(*src++);
	return src - str - 1;
}

size_t strnlen(const char *str, size_t maxlen) {
	const char *src;
	for(src = str; maxlen-- && *src != '\0'; ++src);
	return src - str;
}
