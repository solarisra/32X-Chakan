#include <stddef.h>

/* Freestanding GCC may emit these for the audio context structure copies. */
__attribute__((used,externally_visible))
void *memset(void *destination,int value,size_t count) {
    volatile unsigned char *out=destination;
    while (count--) *out++=(unsigned char)value;
    return destination;
}

__attribute__((used,externally_visible))
void *memcpy(void *destination,const void *source,size_t count) {
    volatile unsigned char *out=destination;
    const unsigned char *in=source;
    while (count--) *out++=*in++;
    return destination;
}
