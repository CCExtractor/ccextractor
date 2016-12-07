#ifndef _DEBUG_DEF_H_
#define _DEBUG_DEF_H_

#ifdef DEBUG
#define LOG_DEBUG(...) printf(__VA_ARGS__)
#else
#define LOG_DEBUG ;
#endif


#endif