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
