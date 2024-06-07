#include <stdio.h>

#include "_project.h" // IWYU pragma: keep

int main(int argc, char * argv[]) {
	printf("[main] args:\n");
	for (int i = 0; i < argc; i++) {
		printf("- %s\n", argv[i]);
	}
	printf("\n");
	fgetc(stdin);
	return 0;
}

// ---- ---- ---- ----
// internal
// ---- ---- ---- ----

#if BUILD_TARGET == BUILD_TARGET_GRAPHICAL
# define WIN32_LEAN_AND_MEAN
# include <Windows.h>

# include <stdlib.h>
int main(int argc, char * argv[]);

WINAPI
int WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow) {
	return main(__argc, __argv);
}
#endif
