#include "MemoryBus.h"

#include <stdio.h>

#define PRINT_DEBUG(message, ...) fprintf(stderr, "MemoryBus: "  message, ##__VA_ARGS__)

void MemoryBus_Initialize(memory_bus *memoryBus, unsigned short pageSize)
{
	memoryBus->pageSize = pageSize;
	memoryBus->firstPageRegistration = NULL;
}

void MemoryBus_AddPageRegistration(memory_bus *memoryBus, memory_page_registration *registration, unsigned int start, unsigned short pages, memory_bus_read_function pageRead, memory_bus_write_function pageWrite, void *pageArgument)
{
	if (memoryBus->firstPageRegistration == NULL)
	{
		memoryBus->firstPageRegistration = registration;
	} else {
		memory_page_registration *p = memoryBus->firstPageRegistration;

		while (p->nextPageRegistration != NULL)
		{
			p = p->nextPageRegistration;
		}

		p->nextPageRegistration = registration;
	}

	registration->start = start;
	registration->pages = pages;
	registration->pageRead = pageRead;
	registration->pageWrite = pageWrite;
	registration->pageArgument = pageArgument;
}

static memory_page_registration *FindRegistration(memory_bus *memoryBus, vm_word_t address)
{
	memory_page_registration *p = memoryBus->firstPageRegistration;

	while (p != NULL)
	{
		vm_word_t startAddress = memoryBus->pageSize * p->start;
		vm_word_t endAddress = startAddress + memoryBus->pageSize * p->pages;

		if (startAddress <= address && address < endAddress)
		{
			return p;
		}

		p = p->nextPageRegistration;
	}

	return NULL;
}

vm_status MemoryBus_ReadFunction(void *argument, vm_word_t address, vm_word_t *word)
{
	memory_bus *memoryBus = (memory_bus *)argument;
	memory_page_registration *registration = FindRegistration(memoryBus, address);

	if (registration == NULL)
	{
		return VM_STATUS_MEMORY_BUS_FAULT;
	}

	return registration->pageRead(registration->pageArgument, address - registration->start * memoryBus->pageSize, word);
}

vm_status MemoryBus_WriteFunction(void *argument, vm_word_t address, vm_word_t word)
{
	memory_bus *memoryBus = (memory_bus *)argument;
	memory_page_registration *registration = FindRegistration(memoryBus, address);

	if (registration == NULL)
	{
		return VM_STATUS_MEMORY_BUS_FAULT;
	}

	return registration->pageWrite(registration->pageArgument, address - registration->start * memoryBus->pageSize, word);
}

vm_status Buffer_ReadFunction(void *argument, vm_word_t address, vm_word_t *word)
{
	vm_word_t *data = argument;

	if (address & (sizeof(vm_word_t) - 1))
	{
		return VM_STATUS_UNALIGNED_ACCESS;
	}

	*word = *(data + (address& ~(sizeof(vm_word_t) - 1)));

	PRINT_DEBUG("Read: %08x = %08x\r\n", address, *word);

	return VM_STATUS_OK;
}

vm_status Buffer_WriteFunction(void *argument, vm_word_t address, vm_word_t word)
{
	vm_word_t *data = argument;

	if (address & (sizeof(vm_word_t) - 1))
	{
		return VM_STATUS_UNALIGNED_ACCESS;
	}

	*(data + (address & ~(sizeof(vm_word_t) - 1))) = word;

	return VM_STATUS_OK;
}
