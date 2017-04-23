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
	int number_pages = (1<<MAX_ORDER) / PAGE_SIZE;
	/* Loop through the number of pages */
	for (int i = 0; i < number_pages; i++)
	{
		/* set the previous and the next values for the list */
		(&g_pages[i].list)->next = (&g_pages[i].list);
		(&g_pages[i].list)->prev = (&g_pages[i].list);
		/* Start with the MAX_ORDER ortherwise set to MIN_INT */
		if ( i == 0)
		{
			g_pages[i].block_size = MAX_ORDER;
		}
		else
		{
			g_pages[i].block_size = INT_MIN;
		}
		/* set the page index */
		g_pages[i].page_index = i;
		/* set the page address */
		g_pages[i].page_address = PAGE_TO_ADDR(i);

	}

	/* initialize freelist */
	for (int i = MIN_ORDER; i <= MAX_ORDER; i++)
	{
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
	//Check if the size is possible
	if(size > order_to_bytes(MAX_ORDER))
	{
		#if USE_DEBUG
			printf("%s\n","Error this won't Work" );
		#endif
		return NULL;
	}
	else if (size < 1)
	{
		#if USE_DEBUG
			printf("%s\n","Error this won't Work" );
		#endif
		return NULL;
	}

	/* Used to store the minimal order for allocation*/
	int alloc_size = MIN_ORDER;

	/* For each memory size possible, try to allocate or split up memory */
	for(int i=MIN_ORDER; i <= MAX_ORDER;i++)
	{

		#if USE_DEBUG
			printf("%s%i\n","i:",i );
		#endif

		/* This is used to find the smallest block size possible */
		if(size > order_to_bytes(alloc_size))
		{
			alloc_size++;
		}

		/* If the given size is free, we can allocate */
		else if(!list_empty(&free_area[i]))
		{
			/* Variable used to partition */
			page_t *block;

			/* If the allocation size is not found, use recursive strategy */
			if(i != alloc_size)
			{
				block = &g_pages[ADDR_TO_PAGE(buddy_alloc(order_to_bytes(alloc_size+1)))];

				int buddy_location = block->page_index + order_to_bytes(alloc_size)/PAGE_SIZE;
				list_add(&(g_pages[buddy_location].list), &free_area[alloc_size]);
			}
			/* Otherwise, update free_area */
			else
			{
				block = list_entry(free_area[i].next, page_t, list);
				list_del(&(block->list));
			}

			block->block_size = alloc_size;
			/* Return the address */
			return PAGE_TO_ADDR (block->page_index);

		}
	}
	/* Return NULL, where there is not enough space available */
	return NULL;

}

int order_to_bytes(int order)
{
	return (1 << order);
}

/**
 * Finds the available space for buddy
 *
 * Name: whereisavialable(int size, void* addr)
 */
page_t* whereisavialable(int size, void* addr)
{
	/* Create a list that we will iterate through in search of similiar sized free area*/
	struct list_head* l;
	/* initialize a page which will be the return variable */
	page_t * page = NULL;
	/* iterate over the free_area list */
	for (l = (&free_area[size])->next; l != (&free_area[size]); l = l->next)
	{
		/* temporarily set a page variable equal to the current list entry */
		page = list_entry(l, page_t, list);

		/* check if the list entry size is the same as buddy's size */
		if (page->page_address == BUDDY_ADDR(addr , size))
		{
			/*return the list entry which is the same size as buddy's*/
			return page;
		}
	}
	/* return NULL if there is no similiarly sized list entries */
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
	/* Initialize variable to iterate and keep track of location */
	/* Create a variable to house buddy's address */
	int buddy_address = ADDR_TO_PAGE(addr);
	/* Create a variable to house the block size , which will be incremented */
	int buddy_block_size = g_pages[buddy_address].block_size;
	/* Create a page_t variable to house the location of whether buddy has a similiar size address */
	page_t * current_page;


	/* loop through the possible values that the block size could be */
	while(buddy_block_size <= MAX_ORDER)
	{
		/* get the location of a same sized list item */
		current_page = whereisavialable(buddy_block_size,addr);

		/* if there is no same sized location */
		if ( current_page == NULL )
		{
			list_add(&g_pages[buddy_address].list, &free_area[buddy_block_size]);
			return;
		}
		// an entry was found that is the same size as buddy
		else
		{
			#if USE_DEBUG
				printf("%s%i\n","addr:",(int)addr );
			#endif
			#if USE_DEBUG
				printf("%s%i\n","current_page->page_address:",(int)current_page->page_address );
			#endif
			/* increment the address */
			if( (char*) addr > current_page->page_address )
			{
				addr = current_page->page_address;
				buddy_address = ADDR_TO_PAGE(current_page->page_address);
			}
			/* Delete the current list entry */
			list_del(&(current_page->list));
			/* check the next size up of the block sizes */
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
