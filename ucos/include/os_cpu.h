/*cpu dependant os port*/


#ifndef OS_CPU_H
#define OS_CPU_H

#include <base.h>

/*cpu stack is decrease*/
#define  OS_STK_GROWTH    1

typedef  unsigned  char  INT8U;
typedef  signed  char  INT8S;
typedef  unsigned  short   INT16U;
typedef  signed  short  INT16S;
typedef  unsigned  int   INT32U;
typedef  signed int  INT32S;
typedef  int  BOOLEAN;
typedef  float  FP32;
typedef  double  FP64;//IAR:Depends on whether the --64bit_doubles option is used


typedef  unsigned  int  OS_CPU_SR;
typedef  unsigned  int  OS_STK;
typedef  unsigned  int  OS_FLAGS;

/*use method 3 to enter critical*/
#define  OS_CRITICAL_METHOD  3
#define  OS_ENTER_CRITICAL()  local_irq_save(cpu_sr)   
#define  OS_EXIT_CRITICAL()   local_irq_restore(cpu_sr)    

extern void OS_TASK_SW(void);
extern void OsIntCtxSw(void);

#endif

