#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "locking.h"




/* Exercise 1:
 *     Basic memory barrier
 */
 
void mem_barrier() {
   
	asm volatile("mfence":::"memory");
}


/* Exercise 2: 
 *     Simple atomic operations 
 */

void
atomic_sub( int * value,
	    int   dec_val)
{
   int result;
	asm volatile("lock; xaddl %0, %1"
	: "=r" (result), "=m" (*value)
	:"0"(-dec_val),"m"(*value)
	:"memory"
	);
	
	

}

void
atomic_add( int * value,
	    int   inc_val)
{
   asm volatile("lock; xaddl %0, %1"
	: "+r" (inc_val), "+m" (*value)
	:
	:"memory"
	);
	

}



/* Exercise 3:
 *     Spin lock implementation
 */


/* Compare and Swap
 * Returns the value that was stored at *ptr when this function was called
 * Sets '*ptr' to 'new' if '*ptr' == 'expected'
 */
unsigned int
compare_and_swap(unsigned int * ptr,
		 unsigned int   expected,
		 unsigned int   new)
{
   unsigned int prev;
	asm volatile(" lock\n"
			"cmpxchgl %1,%2\n"			
			: "=a" (prev)
			: "q" (new), "m" (*ptr), "0" (expected)
			: "memory");

	return prev; 
	

}
void
spinlock_init(struct spinlock * lock)
{
   lock->free=0;
}

void
spinlock_lock(struct spinlock * lock)
{
  
	while(compare_and_swap(&lock->free,0,1)==1);

}


void
spinlock_unlock(struct spinlock * lock)
{
   lock->free=0;	
	
}


/* return previous value */
int
atomic_add_ret_prev(int * value,
		    int   inc_val)
{
	int prev;
    	asm volatile("lock; xaddl %3, %4"
	: "=a"(prev),"=r" (inc_val), "=m" (*value)
	:"r"(inc_val),"m"(*value),"0"(*value)
	:"memory"
	);

    return prev;
}

/* Exercise 4:
 *     Barrier operations
 */

void
barrier_init(struct barrier * bar,
	     int              count)
{
    bar->init_count = count;
    bar->iterations = 0;
}

void
barrier_wait(struct barrier * bar)
{
    atomic_add(&bar->cur_count,1);
    
    //when all thread have called barrier_wait(), make iterations = 1
    if(bar->cur_count == bar->init_count) 
      bar->iterations = 1;
    
    //wait until all threads have reached barrier
    while(bar->iterations != 1);
    
    atomic_sub(&bar->cur_count,1);    
    
    //last thread makes the iteration = 0
    if(bar->cur_count == 0) {
      bar->iterations = 0;
    }
    
    //wait until last thread has reached this point, then all threads leave barrier
    while(bar->iterations != 0);
}




/* Exercise 5:
 *     Reader Writer Locks
 */

void
rw_lock_init(struct read_write_lock * lock)
{
   lock->num_readers=0;
	lock->writer=0;
	lock->mutex.free=0;
}


void
rw_read_lock(struct read_write_lock * lock)
{
   while(1)
	{
		//increment reader count
      atomic_add(&lock->num_readers,1);
		
      //check if lock is available
      if(!lock->mutex.free)
         return;
         
      //if lock not available, decrement the count       
		atomic_sub(&lock->num_readers,1);
     
      //wait until lock is available
		while(lock->mutex.free);
	}

}

void
rw_read_unlock(struct read_write_lock * lock)
{
  //decrement reader count
  atomic_sub(&lock->num_readers,1);
}

void
rw_write_lock(struct read_write_lock * lock)
{
	//acquire lock
   spinlock_lock(&lock->mutex);
	
   //watch for any reader
   while(lock->num_readers);
	
}


void
rw_write_unlock(struct read_write_lock * lock)
{
   //release lock
	spinlock_unlock(&lock->mutex);
}



/* Exercise 6:
 *      Lock-free queue
 *
 * see: Implementing Lock-Free Queues. John D. Valois.
 *
 * The test function uses multiple enqueue threads and a single dequeue thread.
 *  Would this algorithm work with multiple enqueue and multiple dequeue threads? Why or why not?
 */


/* Compare and Swap 
 * Same as earlier compare and swap, but with pointers 
 * Explain the difference between this and the earlier Compare and Swap function
 */
uintptr_t
compare_and_swap_ptr(uintptr_t * ptr,
		     uintptr_t   expected,
		     uintptr_t   new)
{
	uintptr_t prev = 0;
    	asm volatile( "lock\n"
                  "cmpxchgq %1,%2\n"
                : "=a"(prev)
                : "q"(new), "m"(*ptr), "0"(expected)
                : "memory");
	return prev;
}


/*
   inserts dummy node
   makes head and tail point to the dummy node
*/
void
lf_queue_init(struct lf_queue * queue)
{
	struct node * Node = malloc(sizeof(struct node));
    	Node->value = -1;
    	Node->next = NULL;
    	queue->head = Node;
    	queue->tail = Node;
}


void
lf_queue_deinit(struct lf_queue * lf)
{
	   lf->head = NULL;
    	lf->tail = NULL; 
}


void
lf_enqueue(struct lf_queue * queue,
	   int               val)
{
      //create a node
    	struct node * Node = malloc(sizeof(struct node));
    	Node->value = val;
    	Node->next = NULL;
    	struct node * tail = queue->tail;
    	struct node * temp = tail;
    	
      //appending node to the tail atomically using compare_and_swap_ptr
      do {
         
        	while(tail->next != NULL){
					tail = tail->next;
			}

		} while((compare_and_swap_ptr((uintptr_t *)&tail->next,(uintptr_t)NULL,(uintptr_t)Node) != (uintptr_t)NULL));
	
   compare_and_swap_ptr((uintptr_t *)&queue->tail,(uintptr_t)temp,(uintptr_t)Node);
}

int
lf_dequeue(struct lf_queue * queue,
		   int             * val)
{
	struct node * Node;
	
   //removing node from head atomicallly using compare_and_swap_ptr
   do {
		Node = queue->head;
		if(Node->next == NULL) 
         return 1;
	}while(compare_and_swap_ptr((uintptr_t *)&queue->head,(uintptr_t)Node,(uintptr_t)Node->next) != (uintptr_t)Node);
	
   *val = Node->next->value;
	free(Node);
   return 0;
}


