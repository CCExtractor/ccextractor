#ifndef CC_UTILITY_H
#define CC_UTILITY_H

#ifndef _WIN32
#include <arpa/inet.h>
#endif

#define RL32(x) (*(unsigned int *)(x))
#define RB32(x) (ntohl(*(unsigned int *)(x)))
#define RL16(x) (*(unsigned short int*)(x))
#define RB16(x) (ntohs(*(unsigned short int*)(x)))

void shell_sort(void *base, int nb,size_t size,int (*compar)(const void*p1,const void *p2,void*arg),void *arg);
int string_cmp(const void *p1,const void *p2);
int string_cmp2(const void *p1,const void *p2,void *arg);

#endif
