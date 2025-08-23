#if !defined (UKWN_BASE_H)
#define UKWN_BASE_H

#include "_project.h" // IWYU pragma: export

// ---- ---- ---- ----
// functions
// ---- ---- ---- ----

char   base_get_chr(void);
char * base_get_str(char * buffer, int limit);

// ---- ---- ---- ----
// attributes
// ---- ---- ---- ----

#define AttrFuncLocal() static
#define AttrFileLocal() static

#if defined(__clang__)
# define AttrPrint(fmt, args)                \
/**/__attribute__((format(printf,fmt,args))) \

#else
# define AttrPrint(fmt, args)
#endif

// ---- ---- ---- ----
// formatting
// ---- ---- ---- ----

AttrPrint(1, 2)
void fmt_print(char * fmt, ...);

#endif
