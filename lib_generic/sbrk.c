#include <base.h>


register char * stack_ptr asm ("sp");

char *sbrk (int incr)
{
	extern char   end asm ("end");	/* Defined by the linker.  */
	static char * heap_end;
	char *        prev_heap_end;
	char sp;


	RDIAG(LEGACY_DEBUG,"head_end:0x%8x,stack_ptr:0x%8x,&end:0x%8x,&sp:0x%8x",heap_end,stack_ptr, &end,&sp);
	//RDIAG(LEGACY_DEBUG,"head_end:0x%8x,stack_ptr:0x%8x,&end:0x%8x,&sp:0x%8x");

	if (heap_end == NULL)
		heap_end = & end;

	prev_heap_end = heap_end;

	if ((heap_end + incr) > (&end + MALLOC_MEM_SIZE))
	{
		/* Some of the libstdc++-v3 tests rely upon detecting
		   out of memory errors, so do not abort here.  */
#if 0
		extern void abort (void);

		_write (1, "_sbrk: Heap and stack collision\n", 32);

		abort ();
#else
		errno = ENOMEM;

		RDIAG(LEGACY_DEBUG,"sbrk:ENOMEM!!!\r\n");
		return (char *) -1;
#endif
	}

	heap_end += incr;

	RDIAG(LEGACY_DEBUG,"prev_heap_end:0x%8x,heap_end:0x%8x",prev_heap_end,heap_end);
	return (char *) prev_heap_end;
}
