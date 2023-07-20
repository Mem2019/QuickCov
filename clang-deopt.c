#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

#ifndef DEOPT_CXX
#define DEOPT_CXX "clang++"
#endif
#ifndef DEOPT_CC
#define DEOPT_CC "clang"
#endif

bool is_link(int argc, char const *argv[])
{
	for (int i = 0; i < argc; ++i)
	{
		if (strcmp(argv[i], "-c") == 0)
			return false;
	}
	return true;
}

int main(int argc, char const *argv[])
{
	char const** new_argv = (char const**)malloc((argc + 100) * sizeof(char*));
	size_t idx = 0;
	if (strstr(argv[0], "++") != NULL)
		new_argv[idx++] = getenv("DEOPT_CXX") ? getenv("DEOPT_CXX") : DEOPT_CXX;
	else
		new_argv[idx++] = getenv("DEOPT_CC") ? getenv("DEOPT_CC") : DEOPT_CC;
	new_argv[idx++] = "-g";
	new_argv[idx++] = "-O0";

	// Important, otherwise mysterious errors might occur
	new_argv[idx++] = "-Qunused-arguments";

	bool x_set = false;
	for (int i = 1; i < argc; ++i)
	{
		if (strcmp(argv[i], "-s") == 0)
			continue;
		if (strstr(argv[i], "-O") == argv[i])
			continue;
		if (strstr(argv[i], "-g") == argv[i])
			continue;
		if (strstr(argv[i], "-flto") == argv[i])
			continue;
		if (!strcmp(argv[i], "-x")) x_set = true;
		new_argv[idx++] = argv[i];
	}
	if (x_set)
	{
		new_argv[idx++] = "-x";
		new_argv[idx++] = "none";
	}
	if (getenv("DEOPT_COV"))
	{
		new_argv[idx++] = "-flto";
		if (is_link(argc, argv))
		{
			new_argv[idx++] = "--ld-path="LD_LLD_PATH;
			new_argv[idx++] = "-Wl,-mllvm=-load="DEOPT_DIR"/coverage.so";
			new_argv[idx++] = DEOPT_DIR"/runtime.o";
		}
	}
	new_argv[idx] = NULL;
	if (getenv("SHOW_COMPILER_ARGS"))
	{
		for (int i = 0; i < argc; ++i)
			fprintf(stderr, "%s ", argv[i]);
		fprintf(stderr, "\n");
		for (char const** i = new_argv; *i; ++i)
			fprintf(stderr, "%s ", *i);
		fprintf(stderr, "\n");
	}
	execvp(new_argv[0], (char**)new_argv);
	abort();
	return 0;
}