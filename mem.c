/******************************************************************************
 * FILENAME: mem.c
 * AUTHOR:   cherin@cs.wisc.edu <Cherin Joseph>
 * MODIFIED BY:  Anthony To, section1, Alex Faber, section1
 * DATE:     7 May 2014
 * PROVIDES: Contains a set of library functions for memory allocation
 * *****************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include "mem.h"
#include <stdbool.h>

/* this structure serves as the header for each block */
typedef struct block_hd{
  /* The blocks are maintained as a linked list */
  /* The blocks are ordered in the increasing order of addresses */
  struct block_hd* next;

  /* size of the block is always a multiple of 4 */
  /* ie, last two bits are always zero - can be used to store other information*/
  /* LSB = 0 => free block */
  /* LSB = 1 => allocated/busy block */

  /* For free block, block size = size_status */
  /* For an allocated block, block size = size_status - 1 */

  /* The size of the block stored here is not the real size of the block */
  /* the size stored here = (size of block) - (size of header) */
  int size_status;

}block_header;

/* Global variable - This will always point to the first block */
/* ie, the block with the lowest address */
block_header* list_head = NULL;


/* Function used to Initialize the memory allocator */
/* Not intended to be called more than once by a program */
/* Argument - sizeOfRegion: Specifies the size of the chunk which needs to be allocated */
/* Returns 0 on success and -1 on failure */
int Mem_Init(int sizeOfRegion)
{
  int pagesize;
  int padsize;
  int fd;
  int alloc_size;
  void* space_ptr;
  static int allocated_once = 0;
  
  if(0 != allocated_once)
  {
    fprintf(stderr,"Error:mem.c: Mem_Init has allocated space during a previous call\n");
    return -1;
  }
  if(sizeOfRegion <= 0)
  {
    fprintf(stderr,"Error:mem.c: Requested block size is not positive\n");
    return -1;
  }

  /* Get the pagesize */
  pagesize = getpagesize();

  /* Calculate padsize as the padding required to round up sizeOfRegio to a multiple of pagesize */
  padsize = sizeOfRegion % pagesize;
  padsize = (pagesize - padsize) % pagesize;

  alloc_size = sizeOfRegion + padsize;

  /* Using mmap to allocate memory */
  fd = open("/dev/zero", O_RDWR);
  if(-1 == fd)
  {
    fprintf(stderr,"Error:mem.c: Cannot open /dev/zero\n");
    return -1;
  }
  space_ptr = mmap(NULL, alloc_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
  if (MAP_FAILED == space_ptr)
  {
    fprintf(stderr,"Error:mem.c: mmap cannot allocate space\n");
    allocated_once = 0;
    return -1;
  }
  
  allocated_once = 1;
  
  /* To begin with, there is only one big, free block */
  list_head = (block_header*)space_ptr;
  list_head->next = NULL;
  /* Remember that the 'size' stored in block size excludes the space for the header */
  list_head->size_status = alloc_size - (int)sizeof(block_header);
  
  return 0;
}


/* Function for allocating 'size' bytes. */
/* Returns address of allocated block on success */
/* Returns NULL on failure */
/* Here is what this function should accomplish */
/* - Check for sanity of size - Return NULL when appropriate */
/* - Round up size to a multiple of 4 */
/* - Traverse the list of blocks and allocate the first free block which can accommodate the requested size */
/* -- Also, when allocating a block - split it into two blocks when possible */
/* Tips: Be careful with pointer arithmetic */
void* Mem_Alloc(int size)
{
  //local variables
  block_header* current = NULL;         //means of traversing list
  block_header* nodeToAllocate = NULL; 
  block_header* nodeToShrink = NULL;
  block_header* next;                   //Temp hold next value for nodeToShrink
  int currentSize;                      //size of the node we're currently looking at.
  bool found = false;
  char* addressChange;


  //temp
  int shrinkSize;

  if(size <= 0){
    //puts("Size is invalid");
    return NULL;
  }

  //Adjust size to make it speedy
  if((size % 4) != 0){
    size += 4 - (size % 4);
  }

  //Begin traversing list
  current = list_head;
  while(current != NULL && !found){
    currentSize = current->size_status;

    //Iterate until we find a free block
    if(currentSize & 1){
      current = current->next;
    }else{
      //Now that we've found a free block, let's see if it's big enough
      if(currentSize >= size + (int)sizeof(block_header)){
        //If it's big enough, this is the block that we want
        nodeToAllocate = current;
        nodeToShrink = current;
        found = true;
      }else{
        //advance to the next block otherwise
        current = current->next;
      }
    }
  }

  //Check Contents before allocation
  //Mem_Dump();
  
  if(nodeToAllocate != NULL){
    //Get the size and next before changing the address of the pointer
    shrinkSize =  nodeToShrink->size_status - size - (int)sizeof(block_header);
    next = nodeToShrink->next;


    //Change the address of the pointer.
    //nodeToShrink += (size + (int)sizeof(block_header)) / size;
    addressChange = (char *)nodeToShrink + size + (int)sizeof(block_header);
    //printf("The addition: %x", nodeToShrink + (size / 8 ) + (int)sizeof(block_header));
    //printf("\n\nNew nodeToShrink address: %x", nodeToShrink);

    //Next, modify the size and update next of nodetoShrink
    nodeToShrink = (block_header *)addressChange;
    nodeToShrink->size_status = shrinkSize;
    nodeToShrink->next = next;
    //printf("\n\nnodeToShrink after modifying address: %i\n\n", nodeToShrink->size_status);

    //Now initialize the busy block before nodetoShrink
    nodeToAllocate->size_status = size + 1;
    nodeToAllocate->next = nodeToShrink;

    //Mem_Dump();
    
    return current;
  }

  //Return NULL since we didn't find a free block big enough
  return NULL;
}

/* Function for freeing up a previously allocated block */
/* Argument - ptr: Address of the block to be freed up */
/* Returns 0 on success */
/* Returns -1 on failure */
/* Here is what this function should accomplish */
/* - Return -1 if ptr is NULL */
/* - Return -1 if ptr is not pointing to the first byte of a busy block */
/* - Mark the block as free */
/* - Coalesce if one or both of the immediate neighbours are free */
int Mem_Free(void *ptr)
{
  //local variables
  block_header* current = NULL;         //means of traversing list
  block_header* nextBlock = NULL;
  block_header* blockPointer = NULL;
  int sizeToExpand;
  bool isFound;
	

  //puts("Before 212");
  //Check to see if pointer is pointing to NULL
  if(ptr == NULL){
    return -1;
  }
  //puts("Before 216");
  blockPointer = (block_header *)ptr;

  //check to see if the ptr exists in the list
  isFound = false;
  current = list_head;
  while(current != NULL && !isFound){
    if(current == blockPointer){
      isFound = true;
    }
    current = current->next;
  }
  //if the pointer doesn't exist, return -1
  if(!isFound){
    return -1;
  }

  //reset current
  current = list_head;

  //Now check to see if pointer is busy block
  if(!(blockPointer->size_status & 1)){
    return -1;
  }

  //Mark block as free
  blockPointer->size_status += -1;

  //Now coalesce
  //First check to see if the node to the right is free
  nextBlock = blockPointer->next;
  //Make sure nextBlock isn't NULL
  if(nextBlock != NULL){
    //check to see if the block is free
    if(!(nextBlock->size_status & 1)){
      sizeToExpand = nextBlock->size_status + (int)sizeof(block_header);
      blockPointer->size_status += sizeToExpand;
      blockPointer->next = nextBlock->next;
    }
  }

  //Iterate through the list to find the block before ptr
  if(current != blockPointer){
    while(current->next != blockPointer){
      current = current->next;
    }

    //Now know current is right before pointer, and check to see if busy
    if(!(current->size_status & 1)){
        sizeToExpand = blockPointer->size_status + (int)sizeof(block_header);
        current->size_status += sizeToExpand;
        current->next = blockPointer->next;
    }

  }

  //Mem_Dump();
  return 0;
}

/* Function to be used for debug */
/* Prints out a list of all the blocks along with the following information for each block */
/* No.      : Serial number of the block */
/* Status   : free/busy */
/* Begin    : Address of the first useful byte in the block */
/* End      : Address of the last byte in the block */
/* Size     : Size of the block (excluding the header) */
/* t_Size   : Size of the block (including the header) */
/* t_Begin  : Address of the first byte in the block (this is where the header starts) */
void Mem_Dump()
{
  int counter;
  block_header* current = NULL;
  char* t_Begin = NULL;
  char* Begin = NULL;
  int Size;
  int t_Size;
  char* End = NULL;
  int free_size;
  int busy_size;
  int total_size;
  char status[5];

  free_size = 0;
  busy_size = 0;
  total_size = 0;
  current = list_head;
  counter = 1;
  fprintf(stdout,"************************************Block list***********************************\n");
  fprintf(stdout,"No.\tStatus\tBegin\t\tEnd\t\tSize\tt_Size\tt_Begin\n");
  fprintf(stdout,"---------------------------------------------------------------------------------\n");
  while(NULL != current)
  {
    t_Begin = (char*)current;
    Begin = t_Begin + (int)sizeof(block_header);
    Size = current->size_status;
    strcpy(status,"Free");
    if(Size & 1) /*LSB = 1 => busy block*/
    {
      strcpy(status,"Busy");
      Size = Size - 1; /*Minus one for ignoring status in busy block*/
      t_Size = Size + (int)sizeof(block_header);
      busy_size = busy_size + t_Size;
    }
    else
    {
      t_Size = Size + (int)sizeof(block_header);
      free_size = free_size + t_Size;
    }
    End = Begin + Size;
    fprintf(stdout,"%d\t%s\t0x%08lx\t0x%08lx\t%d\t%d\t0x%08lx\n",counter,status,(unsigned long int)Begin,(unsigned long int)End,Size,t_Size,(unsigned long int)t_Begin);
    total_size = total_size + t_Size;
    current = current->next;
    counter = counter + 1;
  }
  fprintf(stdout,"---------------------------------------------------------------------------------\n");
  fprintf(stdout,"*********************************************************************************\n");

  fprintf(stdout,"Total busy size = %d\n",busy_size);
  fprintf(stdout,"Total free size = %d\n",free_size);
  fprintf(stdout,"Total size = %d\n",busy_size+free_size);
  fprintf(stdout,"*********************************************************************************\n");
  fflush(stdout);
  return;
}
