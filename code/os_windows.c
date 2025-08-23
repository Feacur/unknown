#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "base.h"

// ---- ---- ---- ----
// internal
// ---- ---- ---- ----

#if BUILD_TARGET == BUILD_TARGET_GRAPHICAL
# include <stdlib.h>

int main(int argc, char * argv[]);

WINAPI
int WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow) {
	return main(__argc, __argv);
}
#endif
