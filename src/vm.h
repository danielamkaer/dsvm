#ifndef __VM_H_
#define __VM_H_

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#define VM_WORD_SIZE 32

#if VM_WORD_SIZE == 32
typedef uint32_t vm_word_t;
#elif VM_WORD_SIZE == 16
typedef uint16_t vm_word_t;
#endif

/**
 * Instructions
 *
 * 00xxxxxx - nothing follows
 * 01xxxxxx - 1 B follows
 * 10xxxxxx - VM_WORD_SIZE follows
 * 11xxxxxx - reserved
 *
 * 00000000 - NOP
 * 00000001 - AND
 * 00000010 - OR
 * 00000011 - NOT
 *
 * 10000000 - LOAD
 */

typedef enum {
	OPCODE_LENGTH_NO_EXTRA = 0x00,
	OPCODE_LENGTH_1B_EXTRA = 0x01,
	OPCODE_LENGTH_WORD_EXTRA = 0x02,
	OPCODE_LENGTH_RESERVED = 0x03
} opcode_length;

#define OPCODE_NOP   (0x00)
#define OPCODE_AND   (0x01)
#define OPCODE_OR    (0x02)
#define OPCODE_NOT   (0x03)
#define OPCODE_DUP   (0x04)
#define OPCODE_CMP   (0x05)
#define OPCODE_INC   (0x06)
#define OPCODE_POP   (0x07)
#define OPCODE_LDI1  (0x08)
#define OPCODE_RET   (0x3E)
#define OPCODE_HALT  (0x3F)

#define OPCODE_HVCALL (0x40)

#define OPCODE_LOAD  (0x80)
#define OPCODE_LOADI (0x81)
#define OPCODE_JUMP  (0x82)
#define OPCODE_CALL  (0x83)
#define OPCODE_JUMPZ (0x84)
#define OPCODE_JUMPNZ (0x85)
//#define OPCODE_STORE (0x81)

typedef struct
{
	uint8_t opcode;
	const char *name;
} vm_opcode_entry;

extern vm_opcode_entry vmOpcodes[];

typedef vm_word_t (*vm_hypervisor_call)(vm_word_t);

typedef struct
{
	uint8_t randomAccessMemory[1<<16];
	vm_word_t stackPointer;
	vm_word_t programCounter;
	bool zeroFlag;
	vm_hypervisor_call hypervisorCalls[256];
} vm_instance;

typedef enum
{
	VM_STATUS_OK = 0,
	VM_STATUS_ERROR
} vm_status;

void Vm_Initialize(vm_instance *instance);
vm_status Vm_Load(vm_instance *instance, vm_word_t destination, const uint8_t *source, size_t sourceLength);
vm_status Vm_Run(vm_instance *instance);
vm_status Vm_SetHypervisorCall(vm_instance *instance, uint8_t hvCallNumber, vm_hypervisor_call func);

#endif
