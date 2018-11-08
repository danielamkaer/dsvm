#ifndef __MEMORYBUS_H_
#define __MEMORYBUS_H_

#include "VirtualMachine.h"

typedef vm_status (*memory_bus_read_function)(void *argument, vm_word_t address, vm_word_t *word);
typedef vm_status (*memory_bus_write_function)(void *argument, vm_word_t address, vm_word_t word);

typedef struct memory_page_registration
{
	struct memory_page_registration *nextPageRegistration;
	unsigned int start;
	unsigned short pages;
	memory_bus_read_function pageRead;
	memory_bus_write_function pageWrite;
	void *pageArgument;
} memory_page_registration;

typedef struct
{
	unsigned short pageSize;
	memory_page_registration *firstPageRegistration;
} memory_bus;

void MemoryBus_Initialize(memory_bus *memoryBus, unsigned short pageSize);
void MemoryBus_AddPageRegistration(memory_bus *memoryBus, memory_page_registration *registration, unsigned int start, unsigned short pages, memory_bus_read_function pageRead, memory_bus_write_function pageWrite, void *pageArgument);
vm_status MemoryBus_ReadFunction(void *argument, vm_word_t address, vm_word_t *word);
vm_status MemoryBus_WriteFunction(void *argument, vm_word_t address, vm_word_t word);

vm_status Buffer_ReadFunction(void *argument, vm_word_t address, vm_word_t *word);
vm_status Buffer_WriteFunction(void *argument, vm_word_t address, vm_word_t word);

#endif
