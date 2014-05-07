/* custom free space */
#include <assert.h>
#include <stdlib.h>
#include "mem.h"

int main() {
   int x = 50;                   //temporary variable to point to
   void* ptr[4];                 
   void* badPtr;                 //Will be as its name suggets, our bad pointer
   badPtr = &x;                  //gives badPtr an address (of x) that resides outside of what Mem_Init set aside for us

   assert(Mem_Init(4096) == 0);

   //Insert a proper pointer
   ptr[0] = Mem_Alloc(800);

   //Free it; test for 0 because it should've worked fine
   assert(Mem_Free(ptr[0]) == 0);

   //Free it again; should return -1 because the pointer has already been freed
   assert(Mem_Free(ptr[0]) == -1);

   //Give it a pointer that resides outside of the address space that Mem_Init allocated for us
   // it will return a negative -1 because that pointer won't exist in our linked list, so 
   // check for it
   assert(Mem_Free(badPtr) == -1);

   //Check to see if Mem_Free handles a NULL pointer properly
   badPtr = NULL;
   assert(Mem_Free(badPtr) == -1);

   exit(0);
}
