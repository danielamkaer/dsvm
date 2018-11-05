#include <stdio.h>
#include <string.h>
#include "vm.h"

static vm_instance vmInstance;

static int Assemble(uint8_t *destination, size_t destinationSize, size_t *assembledLength)
{
	(void) destination;
	(void) destinationSize;

	int status = 0;
	size_t position = 0;

	FILE *fp = fopen("program", "r");
	if (fp == NULL)
	{
		status = 1;
		goto out;
	}

	char opcode[40];

	while (1) {
		if (fscanf(fp, "%s", opcode) == EOF)
		{
			break;
		}

		if (strcasecmp(".db", opcode) == 0)
		{
			fseek(fp, 1, SEEK_CUR);
			char value;

			while (1) {
				int scanResult = fscanf(fp, "%c", &value);
				if (scanResult == EOF || value == '\n' || value == '\r')
				{
					break;
				}

				destination[position++] = value;
			}
			destination[position++] = 0;

			continue;
		}

		vm_opcode_entry *entry = vmOpcodes;
		while (entry->name != NULL)
		{
			if (strcasecmp(entry->name, opcode) == 0)
			{
				break;
			}

			entry++;
		}

		if (entry->name == NULL)
		{
			printf("Invalid instruction <%s>\r\n", opcode);
			status = 2;
			goto out;
		}

		destination[position++] = entry->opcode;
		opcode_length opcodeLength = (opcode_length) entry->opcode >> 6;

		switch (opcodeLength) {
			case OPCODE_LENGTH_1B_EXTRA:
			{
				int32_t value;
				fscanf(fp, "%i", &value);
				destination[position++] = (uint8_t) value;
			}
			break;
			case OPCODE_LENGTH_WORD_EXTRA:
			{
				int32_t value;
				fscanf(fp, "%i", &value);

				vm_word_t vmValue = (vm_word_t) value;

				memcpy(destination + position, &vmValue, sizeof(vm_word_t));

				position += sizeof(vm_word_t);
			}
			break;
			default:
				break;
		}
	}
	*assembledLength = position;

	out:
	if (fp) fclose(fp);

	return status;
}

static vm_word_t VmPutChar(vm_word_t word)
{
	putchar((char) word);
	return 0;
}

int main(void)
{
	uint8_t program[1<<16];
	size_t assembledLength;

	int status = Assemble(program, sizeof(program), &assembledLength);
	if (status) {
		printf("status = %i\r\n", status);
		return status;
	}

	for (size_t i = 0; i < assembledLength; i++)
	{
		char c = (program[i] >= 32 && program[i] <= 127) ? program[i] : '.';

		printf("%04x: '%c' %02x", i, c, program[i]);

		opcode_length opcodeLength = program[i] >> 6;
		int opLength = 0;
		if (opcodeLength == OPCODE_LENGTH_1B_EXTRA)
		{
			opLength = 1;
		}
		if (opcodeLength == OPCODE_LENGTH_WORD_EXTRA)
		{
			opLength = sizeof(vm_word_t);
		}

		for (size_t z = i + 1; z < i + opLength + 1; z++)
		{
			printf(" %02x", program[z]);
		}
		i += opLength;

		printf("\r\n");
	}

	Vm_Initialize(&vmInstance);
	Vm_SetHypervisorCall(&vmInstance, 0x10, VmPutChar);

	Vm_Load(&vmInstance, 0, program, assembledLength);

	Vm_Run(&vmInstance);
}
