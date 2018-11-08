#include "VirtualMachine.h"

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
	{ OPCODE_STORE, "store" },
	{ 0, NULL }
};

#define PRINT_DEBUG(...) fprintf(stderr, __VA_ARGS__)

static inline void Stack_Push(vm_instance *instance, const vm_word_t *word)
{
	instance->stackPointer -= sizeof(vm_word_t);

	instance->memoryWrite(instance->memoryArgument, instance->stackPointer, *word);
}

static inline void Stack_Pop(vm_instance *instance, vm_word_t *wordOut)
{
	instance->memoryRead(instance->memoryArgument, instance->stackPointer, wordOut);

	instance->stackPointer += sizeof(vm_word_t);
}

void Vm_Initialize(vm_instance *instance)
{
	memset(instance, 0, sizeof(vm_instance));
}

vm_status Vm_Load(vm_instance *instance, vm_word_t destination, const vm_word_t *source, size_t sourceWords)
{
	for (size_t i = 0; i < sourceWords; i++)
	{
		instance->memoryWrite(instance->memoryArgument, destination + i * sizeof(vm_word_t), source[i]);
	}

	return VM_STATUS_OK;
}

void Vm_SetMemoryHandling(vm_instance *instance, vm_memory_read_function readFunction, vm_memory_write_function writeFunction, void *argument)
{
	instance->memoryRead = readFunction;
	instance->memoryWrite = writeFunction;
	instance->memoryArgument = argument;
}

static vm_status MemRead(vm_instance *instance, uint8_t *destination, vm_word_t address, size_t numberOfBytes)
{
	vm_status status = VM_STATUS_OK;

	size_t read = 0;
	while (read < numberOfBytes)
	{
		vm_word_t aligned = (address + read) & ~(sizeof(vm_word_t) - 1);
		vm_word_t offset = (address + read) & (sizeof(vm_word_t) - 1);

		vm_word_t word;
		uint8_t *p = (uint8_t *) &word;

		status = instance->memoryRead(instance->memoryArgument, aligned, &word);

		if (status != VM_STATUS_OK)
		{
			return status;
		}

		for (; read<numberOfBytes && offset < sizeof(vm_word_t); read++)
		{
			*destination++ = p[offset++];
		}
	}

	return status;
}

vm_status Vm_Run(vm_instance *instance)
{
	bool halt = false;
	while (halt == false)
	{
		PRINT_DEBUG("> programCounter = %08x\r\n", instance->programCounter);
		uint8_t opcode;

		MemRead(instance, &opcode, instance->programCounter, sizeof(opcode));

		PRINT_DEBUG("Opcode = %02x\r\n", opcode);

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
			vm_word_t bottomOfStack;

			instance->memoryRead(instance->memoryArgument, instance->stackPointer, &bottomOfStack);

			Stack_Push(instance, &bottomOfStack);
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

				vm_word_t word;
				MemRead(instance, (uint8_t *) &word, instance->programCounter + 1, sizeof(word));

				PRINT_DEBUG("LOADI address=%08x word=%08x\r\n", instance->programCounter + 1, word);

				Stack_Push(instance, &word);
			}
			break;
		case OPCODE_STORE:
			{
				PRINT_DEBUG("STORE\r\n");

				vm_word_t address;
				MemRead(instance, (uint8_t *) &address, instance->programCounter + 1, sizeof(address));

				vm_word_t word;
				Stack_Pop(instance, &word);

				PRINT_DEBUG("STORE address=%08x word=%08x\r\n", address, word);

				instance->memoryWrite(instance->memoryArgument, address, word);
			}
			break;
		case OPCODE_LDI1:
			{
				PRINT_DEBUG("LDI1\r\n");

				vm_word_t address;
				uint8_t value;

				Stack_Pop(instance, &address);

				MemRead(instance, &value, address, sizeof(value));

				vm_word_t promotedValue = value;

				Stack_Push(instance, &promotedValue);
			}
			break;
		case OPCODE_HVCALL:
			{
				uint8_t hvCall;

				MemRead(instance, &hvCall, instance->programCounter + 1, sizeof(hvCall));

				vm_word_t word;

				Stack_Pop(instance, &word);
				Stack_Push(instance, &word);

				printf("%c\r\n", word & 0xff);
			}
			break;
		case OPCODE_JUMP:
			{
				PRINT_DEBUG("JUMP\r\n");
				postIncrementProgramCounter = 0;

				vm_word_t address;
				MemRead(instance, (uint8_t *) &address, instance->programCounter + 1, sizeof(vm_word_t));
				instance->programCounter = address;
			}
			break;
		case OPCODE_JUMPZ:
			{
				PRINT_DEBUG("JUMPZ\r\n");
				if (instance->zeroFlag) {
					PRINT_DEBUG("JUMPZ - true\r\n");
					postIncrementProgramCounter = 0;

					vm_word_t address;
					MemRead(instance, (uint8_t *) &address, instance->programCounter + 1, sizeof(vm_word_t));
					instance->programCounter = address;
				}
			}
			break;
		case OPCODE_JUMPNZ:
			{
				PRINT_DEBUG("JUMPNZ\r\n");
				if (!instance->zeroFlag) {
					PRINT_DEBUG("JUMPNZ - true\r\n");
					postIncrementProgramCounter = 0;

					vm_word_t address;
					MemRead(instance, (uint8_t *) &address, instance->programCounter + 1, sizeof(vm_word_t));
					instance->programCounter = address;
				}
			}
			break;
		case OPCODE_CALL:
			{
				PRINT_DEBUG("CALL\r\n");

				vm_word_t returnAddress = instance->programCounter + postIncrementProgramCounter;

				Stack_Push(instance, &returnAddress);

				vm_word_t jumpAddress;

				MemRead(instance, (uint8_t *) &jumpAddress, instance->programCounter + 1, sizeof(vm_word_t));

				instance->programCounter = jumpAddress;

				postIncrementProgramCounter = 0;
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

vm_word_t Vm_GetStackPointer(vm_instance *instance)
{
	return instance->stackPointer;
}

void Vm_SetStackPointer(vm_instance *instance, vm_word_t newStackPointer)
{
	instance->stackPointer = newStackPointer;
}

vm_word_t Vm_GetProgramCounter(vm_instance *instance)
{
	return instance->programCounter;
}

void Vm_SetProgramCounter(vm_instance *instance, vm_word_t newProgramCounter)
{
	instance->programCounter = newProgramCounter;
}

