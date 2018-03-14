/**
 * @brief	Memory management uint for external SDRAM wired to RA8876
 * @file	memory.h
 * @author	John Leung @ TechToys www.TechToys.com.hk
 * @section	HISTORY
 *
 */

#ifndef _MEMORY_H
#define _MEMORY_H

#include "Ra8876_Lite.h"

#define MEM_LARGE_BLOCK_THRESHOLD       0					//always use lower memory region for storage
#define MEM_START_ENTRY					CANVAS_OFFSET		//always use the same start address for memory_map[] & RA8876's SDRAM
#define MEM_BLOCK_LN_NUM				4					//Number of canvas line for a memory block
															//Set this number larger (i.e. 10) for low SRAM mcu

class Memory {
	private:
		uint16_t *memory_tbl = NULL;
		uint16_t mem_alloc_tbl_size = 0;
		uint16_t mem_block_size = 0;
		uint8_t  isMemoryManagementReady = 0;
	public:
		Memory(){};
		~Memory();
		void 	mem_init(void);
		int16_t	mem_percentage_used(void);
		int32_t mem_malloc(uint32_t size);
		int32_t	mem_free(int32_t offset);
};

extern Memory *mmu;

#endif
