#include <stdlib.h>
#include <stdio.h>
#include <string.h>
//#include <sys/types.h>
//#include <pthread.h>

#include "locking.h"




/* Exercise 1:
 *     Basic memory barrier
 */
void mem_barrier() {
    /* Implement this */
    asm volatile("mfence":::"memory");
}


/* Exercise 2: 
 *     Simple atomic operations 
 */

void
atomic_sub( int * value,
	    int   dec_val)
{
    /* Implement this */
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
    /* Implement this */
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
    /* Implement this */
    //unsigned char ret;
    unsigned int original;
    asm volatile (
	"lock\n"
	"cmpxchgl %1,%2\n"
	: "=a" (original)
	: "q" (new), "m" (*ptr), "0" (expected)
	: "memory"
    );
    return original;
}

void
spinlock_init(struct spinlock * lock)
{
    /* Implement this */
    lock->free = 0;
}

void
spinlock_lock(struct spinlock * lock)
{
    /* Implement this */
    while(compare_and_swap(&lock->free,0,1)==1);
}


void
spinlock_unlock(struct spinlock * lock)
{
    /* Implement this */
    lock->free = 0;
    //printf("lock = %d",lock->free);
}


/* return previous value */
int
atomic_add_ret_prev(int * value,
		    int   inc_val)
{
    /* Implement this */
    return 0;
}

/* Exercise 4:
 *     Barrier operations
 */

void
barrier_init(struct barrier * bar,
	     int              count)
{
    /* Implement this */
    bar->init_count = count;
    bar->iterations = 0;
}

void
barrier_wait(struct barrier * bar)
{
    /* Implement this */
    atomic_add(&bar->cur_count,1);
    if(bar->cur_count == bar->init_count) { mem_barrier(); bar->iterations = 1; }
    while(bar->iterations != 1) {mem_barrier();};
    atomic_sub(&bar->cur_count,1);    
    if(bar->cur_count == 0) {
      bar->iterations = 0;
    }
    while(bar->iterations != 0) { mem_barrier(); }
}


/* Exercise 5:
 *     Reader Writer Locks
 */

void
rw_lock_init(struct read_write_lock * lock)
{
    /* Implement this */
    lock->num_readers=0;
    lock->writer=0;
    lock->mutex.free=0;
}


void
rw_read_lock(struct read_write_lock * lock)
{
    /* Implement this */
    while(1)
    {
      atomic_add(&lock->num_readers,1);
      if(!lock->mutex.free) return;
      atomic_sub(&lock->num_readers,1);
      while(lock->mutex.free);
    }
}

void
rw_read_unlock(struct read_write_lock * lock)
{
    /* Implement this */
    atomic_sub(&lock->num_readers,1);
}

void
rw_write_lock(struct read_write_lock * lock)
{
    /* Implement this */
    spinlock_lock(&lock->mutex);
    while(lock->num_readers);
}


void
rw_write_unlock(struct read_write_lock * lock)
{
    /* Implement this */
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
    /* Implement this */
//    struct node * t = (struct node *)ptr;
//    struct node * t1 = (struct node *)expected;
//    struct node * t2 = (struct node *)new;
//    printf("cmp_swp enter ptr = %p\n",t->value);
//    printf("new = %d",t2->value);
    uintptr_t prev = 0;
    asm volatile( "lock\n"
		  "cmpxchgq %1,%2\n"
		: "=a"(prev)
		: "q"(new), "m"(*ptr), "0"(expected)
		: "memory");
//    t = ptr;
//    printf("cmp_swp exit ptr = %p\n",t->value);
    return prev ;
}

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
    struct node * Node = malloc(sizeof(struct node));
    Node->value = val;
    Node->next = NULL;
    struct node * tail = queue->tail;
    struct node * temp = tail;
    do {
	while(tail->next != NULL){ tail = tail->next; }
    } while(!(compare_and_swap_ptr((uintptr_t *)&tail->next,(uintptr_t)NULL,(uintptr_t)Node) == (uintptr_t)NULL));
    compare_and_swap_ptr((uintptr_t *)&queue->tail,(uintptr_t)temp,(uintptr_t)tail->next);        
}

int
lf_dequeue(struct lf_queue * queue,
	   int             * val)
{
    struct node * Node;
    do {
    	Node = queue->head;
	if(Node->next == NULL) return 1;
    }while(compare_and_swap_ptr((uintptr_t *)&queue->head,(uintptr_t)Node,(uintptr_t)Node->next) != (uintptr_t)Node);
    *val = Node->next->value;
    return 0;
}



