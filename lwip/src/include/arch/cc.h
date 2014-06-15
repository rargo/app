#ifndef LWIP_CC_H
#define LWIP_CC_H

/* see sys_arch.txt,architecture environment */
#include <base.h> /* include standard error number definition */
#include <string.h>

#define BYTE_ORDER  LITTLE_ENDIAN

typedef signed char s8_t;
typedef unsigned char u8_t;
typedef signed short s16_t;
typedef unsigned short u16_t;
typedef signed int s32_t;
typedef unsigned int u32_t;
typedef signed long long  s64_t;
typedef unsigned long long u64_t;

#define PACK_STRUCT_STRUCT __attribute__((packed))

#define PACK_STRUCT_BEGIN

#define PACK_STRUCT_END

#define PACK_STRUCT_FIELD(x) x __attribute__((packed))

#define LWIP_UNUSED_ARG(x) (void)x

// x ("sdfdfd", sdfdd)
#define LWIP_PLATFORM_DIAG(x) do{dlog("[LDIAG %s %5d]:",__func__,__LINE__);\
										dlog x;dlog("\r\n");}while(0)

/* XXX assert infinite loop ? */
#define LWIP_PLATFORM_ASSERT(x) do{dlog("[LASSERT %s, %5d]:",__func__,__LINE__);\
											dlog(x);dlog("\r\n");}while(0)

#define U16_F "d"
#define S16_F "d"
#define U32_F "d"
#define S32_F "d"
#define X16_F "x"
#define X32_F "x"
#define SZT_F "d"

/* disable IRQ */
#define SYS_ARCH_DECL_PROTECT(lev) unsigned int lev
#define SYS_ARCH_PROTECT(lev)	lev = sys_arch_protect()
#define SYS_ARCH_UNPROTECT(lev)  sys_arch_unprotect(lev)

/* XXX it is appropriate to define below here ?*/
typedef unsigned long mem_ptr_t;

/* definition for err_t */
#define LWIP_ERR_T int

#endif
