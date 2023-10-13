#ifndef __REV_MACROS_H__
#define __REV_MACROS_H__

#define NOP asm volatile("nop");

#ifndef NO_REV_MACROS
#define TRACE_OFF      asm volatile("xori x0,x0,0"); // 00004013
#define TRACE_ON       asm volatile("xori x0,x0,1"); // 00104013
#define TRACE_PUSH_OFF asm volatile("xori x0,x0,2"); // 00204013
#define TRACE_PUSH_ON  asm volatile("xori x0,x0,3"); // 00304013
#define TRACE_POP      asm volatile("xori x0,x0,4"); // 00404013
#define assert(x) { TRACE_PUSH_ON;	    \
  if (!(x)) { asm volatile(".word 0x0"); }; \
  TRACE_PUSH_OFF }
#else
#define TRACE_OFF
#define TRACE_ON 
#define TRACE_PUSH_OFF
#define TRACE_PUSH_ON
#define TRACE_POP
#define assert(x)      
#endif

#endif // __REV_MACROS_H__
