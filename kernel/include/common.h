#ifndef __COMMON_H__
#define __COMMON_H__

#include <kernel.h>
#include <nanos.h>
#include <x86.h>

#define LOG

#define TODO() do {\
   logr("You need to finish!"); \
   assert(0);\
} while(0);


#endif
