#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

__attribute__((weak)) int LLVMFuzzerTestOneInput(const uint8_t*, size_t);
__attribute__((weak)) int LLVMFuzzerInitialize(int *argc, char ***argv);

__attribute__((weak)) int main(int argc, char **argv)
{
	if (argc != 2)
	{
		perror("argc != 2");
		return 1;
	}
	FILE* f = fopen(argv[1], "rb");
	if (f == NULL)
	{
		perror("fopen failed");
		return 1;
	}
	int r = fseek(f, 0, SEEK_END);
	if (r != 0)
	{
		perror("fseek failed");
		return 1;
	}
	long size = ftell(f);
	if (size < 0)
	{
		perror("ftell failed");
		return 1;
	}
	r = fseek(f, 0, SEEK_SET);
	if (r != 0)
	{
		perror("fseek failed");
		return 1;
	}
	uint8_t* buf = (uint8_t*)malloc(size);
	if (fread(buf, 1, size, f) != size)
	{
		perror("fread failed");
		return 1;
	}
	if (LLVMFuzzerInitialize)
		LLVMFuzzerInitialize(&argc, &argv);
	LLVMFuzzerTestOneInput(buf, size);
	free(buf);
	fclose(f);
	return 0;
}