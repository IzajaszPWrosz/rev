#ifndef __REV_MACROS_H__
#define __REV_MACROS_H__

#ifndef NO_REV_MACROS
#define TRACE_OFF      asm volatile("slli x0,x0,0"); // 00081013
#define TRACE_ON       asm volatile("slli x0,x0,1"); // 00181013
#define TRACE_PUSH_OFF asm volatile("slli x0,x0,2"); // 00281013
#define TRACE_PUSH_ON  asm volatile("slli x0,x0,3"); // 00381013
#define TRACE_POP      asm volatile("slli x0,x0,4"); // 00481013
#define TRACE_ASSERT(x) { TRACE_PUSH_ON;	    \
  if (!(x)) { asm volatile(".word 0x0"); }; \
  TRACE_PUSH_OFF }
#else
#define TRACE_OFF
#define TRACE_ON 
#define TRACE_PUSH_OFF
#define TRACE_PUSH_ON
#define TRACE_POP
#define TRACE_ASSERT(x)      
#endif

#endif // __REV_MACROS_H__
