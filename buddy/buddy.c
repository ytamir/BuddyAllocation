/**
 * Buddy Allocator
 *
 * For the list library usage, see http://www.mcs.anl.gov/~kazutomo/list/
 */


/**************************************************************************
 * Conditional Compilation Options
 **************************************************************************/
#define USE_DEBUG 0

/**************************************************************************
 * Included Files
 **************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>


#include "buddy.h"
#include "list.h"

/**************************************************************************
 * Public Definitions
 **************************************************************************/
#define WRONG NULL
#define VERYWRONG break

#define MIN_ORDER 12
#define MAX_ORDER 20

#define PAGE_SIZE (1<<MIN_ORDER)
/* page index to address */
#define PAGE_TO_ADDR(page_idx) (void *)((page_idx*PAGE_SIZE) + g_memory)

/* address to page index */
#define ADDR_TO_PAGE(addr) ((unsigned long)((void *)addr - (void *)g_memory) / PAGE_SIZE)

/* find buddy address */
#define BUDDY_ADDR(addr, o) (void *)((((unsigned long)addr - (unsigned long)g_memory) ^ (1<<o)) \
									 + (unsigned long)g_memory)

#if USE_DEBUG == 1
#  define PDEBUG(fmt, ...) \
	fprintf(stderr, "%s(), %s:%d: " fmt,			\
		__func__, __FILE__, __LINE__, ##__VA_ARGS__)
#  define IFDEBUG(x) x
#else
#  define PDEBUG(fmt, ...)
#  define IFDEBUG(x)
#endif

/**************************************************************************
 * Public Types
 **************************************************************************/
typedef struct {
	struct list_head list;
	int block_size;
	/* TODO: DECLARE NECESSARY MEMBER VARIABLsES */
	int page_index;
	char* page_address;
} page_t;

/**************************************************************************
 * Global Variables
 **************************************************************************/
/* free lists*/
struct list_head free_area[MAX_ORDER+1];

/* memory area */
char g_memory[1<<MAX_ORDER];

/* page structures */
page_t g_pages[(1<<MAX_ORDER)/PAGE_SIZE];

/**************************************************************************
 * Public Function Prototypes
 **************************************************************************/

/**************************************************************************
 * Local Functions
 **************************************************************************/

/**
 * Initialize the buddy system
 */
void buddy_init()
{
	#if USE_DEBUG
		printf("%s\n", "2");
	#endif




	int number_pages = (1<<MAX_ORDER) / PAGE_SIZE;
	for (int i = 0; i < number_pages; i++) {
		/* TODO: INITIALIZE PAGE STRUCTURES */

		    (&g_pages[i].list)->next = (&g_pages[i].list);
			(&g_pages[i].list)->prev = (&g_pages[i].list);
			if ( i == 0)
			{
				g_pages[i].block_size = MAX_ORDER;
			}
			else
			{
				g_pages[i].block_size = INT_MIN;
			}
			g_pages[i].page_index = i;
			g_pages[i].page_address = PAGE_TO_ADDR(i);

	}

	/* initialize freelist */
	for (int i = MIN_ORDER; i <= MAX_ORDER; i++) {
		INIT_LIST_HEAD(&free_area[i]);
	}

	/* add the entire memory as a freeblock */
	list_add(&g_pages[0].list, &free_area[MAX_ORDER]);




}

/**
 * Allocate a memory block.
 *
 * On a memory request, the allocator returns the head of a free-list of the
 * matching size (i.e., smallest block that satisfies the request). If the
 * free-list of the matching block size is empty, then a larger block size will
 * be selected. The selected (large) block is then splitted into two smaller
 * blocks. Among the two blocks, left block will be used for allocation or be
 * further splitted while the right block will be added to the appropriate
 * free-list.
 *
 * @param size size in bytes
 * @return memory block address
 */
void *buddy_alloc(int size)
{
	#if USE_DEBUG
		printf("%s\n", "1");
	#endif

	/* TODO: IMPLEMENT THIS FUNCTION */

	//Check if the size is possible
	#if USE_DEBUG
		printf("%s%i\n","Size:",size );
	#endif

	//printf("%s\n%i\n","MAX_ORDER:",MAX_ORDER );

	if(size > order_to_bytes(MAX_ORDER)){
		return NULL;
	}
	else if (size < 1)
	{
		return NULL;
	}

	/* Used to store the minimal allocation and order of said allocation */
	int smallest_alloc = order_to_bytes(MIN_ORDER);
	int smallest_order = MIN_ORDER;

	while(size > smallest_alloc && smallest_order <= MAX_ORDER){
		#if USE_DEBUG
		printf("%s%i\n","smallest_order:",smallest_order );
		printf("%s%i\n","smallest_alloc:",smallest_alloc );
		#endif

		smallest_order++;
		smallest_alloc = order_to_bytes(smallest_order);
	}

	/* For each memory size possible, try to allocate or split up memory */
	for(int i=smallest_order; i <= MAX_ORDER;i++){

		/**
		 *	If there is an available block of the right size, start allocating
		 *
		 *	Note: the for loop starts with the smallest size possible and goes
		 *	up to the greatest size possible
		 */
		 #if USE_DEBUG
	 		printf("%s%i\n","i:",i );
	 	 #endif


		if(!list_empty(&free_area[i])){

			/* Variables used to partition */
			page_t *left_page;
			page_t *right_page;


			/* If the block is the same size as what we need it's easy */

			if(i == smallest_order)
			{

				left_page = list_entry(free_area[i].next, page_t, list);
				list_del(&(left_page->list));

			}
			/* Otherwise we have to split up the block recursively */
			else{

				left_page = &g_pages[ADDR_TO_PAGE(buddy_alloc(order_to_bytes(smallest_order+1)))];
				right_page = &g_pages[left_page->page_index + (order_to_bytes(smallest_order)/PAGE_SIZE)];
				list_add(&(right_page->list), &free_area[smallest_order]);
				#if USE_DEBUG
				      printf("%s\n", "7");
				#endif

			}

			left_page->block_size = smallest_order;
			return PAGE_TO_ADDR (left_page->page_index);

		}
	}

	return NULL;

}


int order_to_bytes(int order){
	return (1 << order);
}



page_t* whereisavialable(int size, void* addr)
{
	struct list_head* l;
	page_t * page = NULL;
	for (l = (&free_area[size])->next; l != (&free_area[size]); l = l->next)
	{
		page = list_entry(l, page_t, list);

		if (page->page_address == BUDDY_ADDR(addr , size))
		{
			return page;
		}
	}
	return NULL;
}
/**
 * Free an allocated memory block.
 *
 * Whenever a block is freed, the allocator checks its buddy. If the buddy is
 * free as well, then the two buddies are combined to form a bigger block. This
 * process continues until one of the buddies is not free.
 *
 * @param addr memory block address to be freed
 */
 void buddy_free(void *addr)
 {
	 int buddy_address = ADDR_TO_PAGE(addr);
	 int buddy_block_size = g_pages[buddy_address].block_size;
	 page_t * current_page = NULL;


	 while(1)
	 {
		 current_page = whereisavialable(buddy_block_size,addr);

 		if ( current_page == NULL || current_page->page_address != BUDDY_ADDR(addr, buddy_block_size))
 		{
 			g_pages[buddy_address].block_size = -1;
 			list_add(&g_pages[buddy_address].list, &free_area[buddy_block_size]);
 			return;
 		}
		else
		{
			#if USE_DEBUG
	 		      printf("%s%i\n","addr:",(int)addr );
	 		#endif
	 		#if USE_DEBUG
	 		      printf("%s%i\n","current_page->page_address:",(int)current_page->page_address );
	 		#endif
	 		if( (char*) addr > current_page->page_address )
	 		{
	 			addr = current_page->page_address;
				buddy_address = ADDR_TO_PAGE(current_page->page_address);
	 		}
	 		list_del(&(current_page->list));
	 		#if USE_DEBUG
	 		      printf("%s\n", "24");
	 		#endif
			 buddy_block_size = buddy_block_size + 1;
		 }
		}


 }


/**
 * Print the buddy system status---order oriented
 *
 * print free pages in each order.
 */
void buddy_dump()
{
	int o;
	for (o = MIN_ORDER; o <= MAX_ORDER; o++) {
		struct list_head *pos;
		int cnt = 0;
		list_for_each(pos, &free_area[o]) {
			cnt++;
		}
		printf("%d:%dK ", cnt, (1<<o)/1024);
	}
	printf("\n");
}
