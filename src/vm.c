#include "vm.h"

#include <string.h>
#include <stdbool.h>
#include <stdio.h>

vm_opcode_entry vmOpcodes[] = {
	{ OPCODE_NOP,  "nop" },
	{ OPCODE_AND,  "and" },
	{ OPCODE_OR,   "or" },
	{ OPCODE_NOT,  "not" },
	{ OPCODE_HALT, "halt" },
	{ OPCODE_LOADI, "loadi" },
	{ OPCODE_LDI1, "ldi.1" },
	{ OPCODE_JUMP, "jump" },
	{ OPCODE_CALL, "call" },
	{ OPCODE_RET, "ret" },
	{ OPCODE_DUP, "dup" },
	{ OPCODE_POP, "pop" },
	{ OPCODE_CMP, "cmp" },
	{ OPCODE_INC, "inc" },
	{ OPCODE_HVCALL, "hvcall" },
	{ OPCODE_JUMPZ, "jumpz" },
	{ OPCODE_JUMPNZ, "jumpnz" },
	{ 0, NULL }
};

#define PRINT_DEBUG(...) fprintf(stderr, __VA_ARGS__)

static inline void Stack_Push(vm_instance *instance, const vm_word_t *word)
{
	instance->stackPointer -= sizeof(vm_word_t);
	memcpy(instance->randomAccessMemory + instance->stackPointer, word, sizeof(vm_word_t));
}

static inline void Stack_Pop(vm_instance *instance, vm_word_t *wordOut)
{
	memcpy(wordOut, instance->randomAccessMemory + instance->stackPointer, sizeof(vm_word_t));
	instance->stackPointer += sizeof(vm_word_t);
}

void Vm_Initialize(vm_instance *instance)
{
	instance->programCounter = 0;
	instance->stackPointer = sizeof(instance->randomAccessMemory);
	memset(instance->randomAccessMemory, 0, sizeof(instance->randomAccessMemory));
}

vm_status Vm_Load(vm_instance *instance, vm_word_t destination, const uint8_t *source, size_t sourceLength)
{
	memcpy(instance->randomAccessMemory + destination, source, sourceLength);

	return VM_STATUS_OK;
}

vm_status Vm_Run(vm_instance *instance)
{
	bool halt = false;
	while (halt == false)
	{
		PRINT_DEBUG("> programCounter = %08x\r\n", instance->programCounter);
		uint8_t opcode = instance->randomAccessMemory[instance->programCounter];
		opcode_length length = (opcode_length) opcode >> 6;

		vm_word_t postIncrementProgramCounter;
		switch (length)
		{
		case OPCODE_LENGTH_NO_EXTRA: postIncrementProgramCounter = 1; break;
		case OPCODE_LENGTH_1B_EXTRA: postIncrementProgramCounter = 1 + 1; break;
		case OPCODE_LENGTH_WORD_EXTRA: postIncrementProgramCounter = 1 + sizeof(vm_word_t); break;
		case OPCODE_LENGTH_RESERVED: return VM_STATUS_ERROR;
		}

		switch (opcode)
		{
		case OPCODE_NOP:
			PRINT_DEBUG("NOP\r\n");
			break;
		case OPCODE_DUP:
			PRINT_DEBUG("DUP\r\n");
			Stack_Push(instance, (vm_word_t *) (instance->randomAccessMemory + instance->stackPointer));
			break;
		case OPCODE_POP:
			{
				PRINT_DEBUG("POP\r\n");
				vm_word_t discard;
				Stack_Pop(instance, &discard);
			}
			break;
		case OPCODE_NOT:
			{
				PRINT_DEBUG("NOT\r\n");
				vm_word_t word;
				Stack_Pop(instance, &word);
				word = ~word;
				Stack_Push(instance, &word);
			}
			break;
		case OPCODE_AND:
			{
				PRINT_DEBUG("AND\r\n");
				vm_word_t a,b;

				Stack_Pop(instance, &a);
				Stack_Pop(instance, &b);

				a &= b;
				Stack_Push(instance, &a);
			}
			break;
		case OPCODE_OR:
			{
				PRINT_DEBUG("OR\r\n");
				vm_word_t a,b;

				Stack_Pop(instance, &a);
				Stack_Pop(instance, &b);

				a |= b;
				Stack_Push(instance, &a);
			}
			break;
		case OPCODE_CMP:
			{
				PRINT_DEBUG("CMP\r\n");

				vm_word_t word;

				Stack_Pop(instance, &word);

				instance->zeroFlag = (word == 0);
			}
			break;
		case OPCODE_INC:
			{
				PRINT_DEBUG("INC\r\n");
				vm_word_t word;

				Stack_Pop(instance, &word);
				word++;
				Stack_Push(instance, &word);
			}
			break;
		case OPCODE_LOADI:
			{
				PRINT_DEBUG("LOADI\r\n");
				Stack_Push(instance, (vm_word_t *) (instance->randomAccessMemory + instance->programCounter + 1));
			}
			break;
		case OPCODE_LDI1:
			{
				PRINT_DEBUG("LDI1\r\n");

				vm_word_t address;
				uint8_t value;

				Stack_Pop(instance, &address);
				memcpy(&value, instance->randomAccessMemory + address, sizeof(uint8_t));

				vm_word_t promotedValue = value;

				Stack_Push(instance, &promotedValue);
			}
			break;
		case OPCODE_JUMP:
			{
				PRINT_DEBUG("JUMP\r\n");
				postIncrementProgramCounter = 0;
				memcpy(&instance->programCounter, instance->randomAccessMemory + instance->programCounter + 1, sizeof(vm_word_t));
			}
			break;
		case OPCODE_JUMPZ:
			{
				PRINT_DEBUG("JUMPZ\r\n");
				if (instance->zeroFlag) {
					postIncrementProgramCounter = 0;
					memcpy(&instance->programCounter, instance->randomAccessMemory + instance->programCounter + 1, sizeof(vm_word_t));
				}
			}
			break;
		case OPCODE_JUMPNZ:
			{
				PRINT_DEBUG("JUMPNZ\r\n");
				if (!instance->zeroFlag) {
					postIncrementProgramCounter = 0;
					memcpy(&instance->programCounter, instance->randomAccessMemory + instance->programCounter + 1, sizeof(vm_word_t));
				}
			}
			break;
		case OPCODE_CALL:
			{
				PRINT_DEBUG("CALL\r\n");

				vm_word_t returnAddress = instance->programCounter + postIncrementProgramCounter;

				Stack_Push(instance, &returnAddress);

				memcpy(&instance->programCounter, instance->randomAccessMemory + instance->programCounter + 1, sizeof(vm_word_t));

				postIncrementProgramCounter = 0;
			}
			break;
		case OPCODE_HVCALL:
			{
				PRINT_DEBUG("HVCALL\r\n");

				vm_word_t argument;
				uint8_t hvCallNumber = instance->randomAccessMemory[instance->programCounter + 1];

				Stack_Pop(instance, &argument);

				argument = instance->hypervisorCalls[hvCallNumber](argument);

				Stack_Push(instance, &argument);
			}
			break;
		case OPCODE_RET:
			{
				PRINT_DEBUG("RET\r\n");
				Stack_Pop(instance, &instance->programCounter);
				postIncrementProgramCounter = 0;
			}
			break;
		case OPCODE_HALT:
			{
				PRINT_DEBUG("HALT\r\n");
				halt = true;
				postIncrementProgramCounter = 0;
			}
			break;
		}

		if (postIncrementProgramCounter > 0)
		{
			instance->programCounter += postIncrementProgramCounter;
		}

		PRINT_DEBUG("< programCounter = %08x\r\n", instance->programCounter);
	}

	return VM_STATUS_OK;
}

vm_status Vm_SetHypervisorCall(vm_instance *instance, uint8_t hvCallNumber, vm_hypervisor_call func)
{
	instance->hypervisorCalls[hvCallNumber] = func;

	return VM_STATUS_OK;
}
