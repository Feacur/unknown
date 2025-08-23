#include <stdio.h>

#include "base.h"

// ---- ---- ---- ----
// functions
// ---- ---- ---- ----

char base_get_chr(void) {
	return fgetc(stdin);
}

char * base_get_str(char * buffer, int limit) {
	return fgets(buffer, limit, stdin);
}

// ---- ---- ---- ----
// formatting
// ---- ---- ---- ----

#define STB_SPRINTF_STATIC
#define STB_SPRINTF_IMPLEMENTATION
#include "_internal/warnings_push.h"
# include <stb/stb_sprintf.h>
#include "_internal/warnings_pop.h"


struct Fmt_Ctx {
	char buffer[STB_SPRINTF_MIN];
};

AttrFileLocal()
char * fmt_print_write_chunk(char const * buf, void * user, int len) {
	struct Fmt_Ctx * ctx = user;
	if (len > 0) {
		fwrite(buf, sizeof(*buf), len, stdout);
	}
	return ctx->buffer;
}

void fmt_print(char * fmt, ...) {
	va_list args;
	va_start(args, fmt);

	struct Fmt_Ctx ctx;
	stbsp_vsprintfcb(fmt_print_write_chunk, &ctx, ctx.buffer, fmt, args);

	va_end(args);
}
