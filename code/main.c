#include "base.h"

int main(int argc, char * argv[]) {
	fmt_print("[main] args:\n");
	for (int i = 0; i < argc; i++) {
		fmt_print("- %s\n", argv[i]);
	}
	fmt_print("\n");
	base_get_chr();
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
