/**
 * @brief	Memory management unit for external SDRAM wired to RA8876
 * @file	memory.cpp
 * @author	John Leung @ TechToys www.TechToys.com.hk
 * @section	HISTORY
 *
 */

#include "memory.h"

void Memory::mem_init(void)
{
	mem_block_size = (uint16_t)ra8876lite.getCanvasWidth()*MEM_BLOCK_LN_NUM;
	mem_alloc_tbl_size = (MEM_SIZE_MAX/mem_block_size);
	
	memory_tbl = new uint16_t[mem_alloc_tbl_size];

	for(uint16_t i=0; i<mem_alloc_tbl_size; i++)
		memory_tbl[i] = 0;
	
#ifdef DEBUG_LLD_MEMORY
	printf("###############################################\n\r");
	printf("Mem block size = %d, memory_tbl_size = %d\n\r", mem_block_size, mem_alloc_tbl_size);
#endif	
	isMemoryManagementReady = 1;
}

Memory::~Memory()
{
	delete [] memory_tbl;
	memory_tbl = NULL;
	mem_alloc_tbl_size = 0;
	mem_block_size = 0;
	isMemoryManagementReady = 0;
#ifdef DEBUG_LLD_MEMORY
	printf("###############################################\n\r");
	printf("mmu object deleted.\n\r");
#endif
}

int16_t Memory::mem_percentage_used(void)
{
	int used=0;
	int i;
	for(i=0;i<mem_alloc_tbl_size;i++)
	{
		if(memory_tbl[i])
		{
			used++;
		}
	}
	return ((used*100)/mem_alloc_tbl_size);
}

//return -1:FAIL
//>=0:	return "allocated" physical address in offset*mem_block_size
int32_t Memory::mem_malloc(uint32_t size)
{
    int offset = 0;
    int startEntry = MEM_START_ENTRY; // (CANVAS_WIDTH*CANVAS_HEIGHT*ra8876lite.getColorDepth())/mem_block_size;
	int nmemb;	//number of memory block required
	int i;

    if(!isMemoryManagementReady)
    {
		mem_init();
    }
	
#ifdef DEBUG_LLD_MEMORY
	printf("***********************************************\n\r");
	printf("Size = 0x%x.\n\r", size);
#endif	

	if(size==0)
	{
#ifdef DEBUG_LLD_MEMORY
		printf(" Error mem_malloc(%d): size==0\n",size);
#endif
		return -1;
	}

	nmemb=size/mem_block_size;	//calculate the number of memory blocks required
	
	if(size%mem_block_size)
	{
		nmemb++;
	}
	
#ifdef DEBUG_LLD_MEMORY	
	printf("Number of memory blocks required is %d\n", nmemb);
#endif	
	//allocate small memory piece in higher memory region
	if(size > MEM_LARGE_BLOCK_THRESHOLD)
	{
	    for(offset=startEntry;offset<mem_alloc_tbl_size-nmemb;offset++)
        {
        	if(!memory_tbl[offset])	//search start of vacant block
	        {
               	//check if the size is enough
				//printf("1!!!!!!!!!!!!!!!!!!!!!!!\n\r");
	            int vacantSize=0;
        	    for(vacantSize=0;vacantSize<nmemb && !memory_tbl[offset+vacantSize];vacantSize++);
                if(vacantSize==nmemb)
                {

					for(i=0;i<nmemb;i++)
					{
		               	memory_tbl[offset+i]=nmemb;
					}
#ifdef DEBUG_LLD_MEMORY
					printf("Memory allocation is successful!\n\r");
					printf("Physical address = 0x%x.\n\r", offset*mem_block_size);
					printf("Memory used = %d%c\n", mem_percentage_used(), '%');
#endif
                    return (offset*mem_block_size);
 	            }
			}
        }
    }
	else
	{
		//printf("mem_alloc_tbl_size=%d\n",mem_alloc_tbl_size);
	    //for(offset=mem_alloc_tbl_size-1;offset>=0;offset--)
		for(offset=mem_alloc_tbl_size-1;offset>=startEntry;offset--)
        {	//scan the whole array memory_tbl[] for continguous memory region  
			//that is able to allocate nmemb's memory blocks for this bitmap
        	if(!memory_tbl[offset] && ((offset+nmemb)<=mem_alloc_tbl_size))
	       	{
				//printf("offset = %d\n",offset);
				//printf("(offset+nmemb) = %d\n",(offset+nmemb));
               	//check if the size is enough
	            int vacantSize=0;
        	    for(vacantSize=0;vacantSize<nmemb && !memory_tbl[offset+vacantSize];vacantSize++);
                if(vacantSize==nmemb)
                {
					
					for(i=0;i<nmemb;i++)
					{
		               	memory_tbl[offset+i]=nmemb;
					}
#ifdef DEBUG_LLD_MEMORY
					printf("Memory allocation is successful!\n\r");
					printf("Physical address = 0x%x.\n\r", offset*mem_block_size);
					printf("Memory used = %d%c\n", mem_percentage_used(), '%');
#endif
                    return (offset*mem_block_size);
 	            }
			}
        }
	}
#ifdef DEBUG_LLD_MEMORY
	printf("Memory allocation failed!\n\r");
#endif
    return -1;
}

int32_t	Memory::mem_free(int32_t offset)
{
	//printf(" mem_free(0x%x)\n\r",offset);
	int i;
        if(!isMemoryManagementReady)
        {
            mem_init();
			return 1;
        }
        if(offset<MEM_SIZE_MAX)
        {
		int index=offset/mem_block_size;
		int nmemb=memory_tbl[index];
		for(i=0;i<nmemb;i++)
		{
			memory_tbl[index+i]=0;
		}
#ifdef DEBUG_LLD_MEMORY
		printf(" mem_free(%d) bytes @ 0x%X\n",nmemb*mem_block_size,offset);
		printf("Memory used = %d%c\n", mem_percentage_used(), '%');
#endif
            return 0;
        }
        else
        {
#ifdef DEBUG_LLD_MEMORY
		printf(" mem_free: Out of bound\n");
#endif
            return 1;//out of bound
        }
}
//Memory *mmu = new Memory();