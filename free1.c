/* custom free space */
#include <assert.h>
#include <stdlib.h>
#include "mem.h"

int main() {
   int x = 50;   //temporary variable to point to
   void* ptr[4];
   void* badPtr;
   badPtr = &x;

   assert(Mem_Init(4096) == 0);

   //Insert a proper pointer
   ptr[0] = Mem_Alloc(800);

   //Free it; test for 0 because it should've worked fine
   assert(Mem_Free(ptr[0]) == 0);
   puts("After assert(Mem_Free(ptr[0]) == 0);");
   Mem_Dump();

   //Free it again; should return -1 because the pointer has already been freed
   assert(Mem_Free(ptr[0]) == -1);
   puts("Mem_Free(ptr[0]) == 0) Again");
   Mem_Dump();
   assert(Mem_Free(badPtr) == -1);
   puts("After 'freeing' pointer that resides outside of address space");
   Mem_Dump();

   exit(0);
}
