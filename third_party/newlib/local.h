/* The one definition the string functions take from newlib's libc/string/local.h:
   stop gcc from recognizing the copy loop inside memcpy() as a memcpy() and
   calling the function from itself.  */
#define __inhibit_loop_to_libcall \
  __attribute__ ((__optimize__ ("-fno-tree-loop-distribute-patterns")))
