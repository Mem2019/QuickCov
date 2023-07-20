#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include <unistd.h>
#include <sys/mman.h>

extern atomic_uchar __cov_trace_bitmap[];
extern char __cov_all_block_names[];
extern const uint64_t __cov_num_names;

void __cov_update_bit(uint64_t idx) __attribute__((visibility("default")));
void __cov_update_bit(uint64_t idx)
{
	// In 32-bit, we truncate high 32 bits since they must be zeros.
	const size_t i = (size_t)idx;
	atomic_fetch_or(__cov_trace_bitmap + i / 8, 1 << (i % 8));
}

static void try_output_bitmap(const char* bitmap_path)
{
	if (bitmap_path == NULL)
		return;
	FILE* fd = fopen(bitmap_path, "wb");
	if (fd == NULL)
	{
		fprintf(stderr, "Unable to open %s", bitmap_path);
		return;
	}
	size_t bitmap_size = (__cov_num_names + 7) / 8;
	size_t res = fwrite(__cov_trace_bitmap, 1, bitmap_size, fd);
	if (res != bitmap_size)
	{
		fprintf(stderr, "Unable to write %s", bitmap_path);
	}
	fclose(fd);
}

static void try_output_names(const char* names_path)
{
	if (names_path == NULL)
		return;
	FILE* fd = fopen(names_path, "w");
	if (fd == NULL)
	{
		fprintf(stderr, "Unable to open %s", names_path);
		return;
	}
	char* it = __cov_all_block_names;
	size_t i = 0;
	while (true)
	{
		char* lb = strchr(it, '\n');
		if (lb == NULL)
			break;
		*lb = 0;
		if (__cov_trace_bitmap[i / 8] & (1 << (i % 8)))
		{
			int r = fprintf(fd, "%s\n", it);
			if (r < 0)
			{
				fprintf(stderr, "Unable to write %s", names_path);
			}
		}
		it = lb + 1; ++i;
	}
	fclose(fd);
}

void __cov_summarize() __attribute__((destructor(0)));
void __cov_summarize()
{
	try_output_bitmap(getenv("COV_BITMAP"));
	try_output_names(getenv("COV_NAMES"));
}