#if !defined (UKWN_PROJECT_H)
#define UKWN_PROJECT_H

#if defined (__clang__) || defined (__GNUC__)
// enable a lot of warnings
# pragma GCC diagnostic warning "-Wall"
# pragma GCC diagnostic warning "-Wextra"
# pragma GCC diagnostic warning "-Wconversion"
# pragma GCC diagnostic warning "-Wdouble-promotion"

// disable some of them
# pragma GCC diagnostic ignored "-Wunused-function"
# pragma GCC diagnostic ignored "-Wunused-variable"
# pragma GCC diagnostic ignored "-Wunused-parameter"
# pragma GCC diagnostic ignored "-Wunused-local-typedef"

// turn others into errors
# pragma GCC diagnostic error "-Wimplicit-int"
# pragma GCC diagnostic error "-Wimplicit-fallthrough"
# pragma GCC diagnostic error "-Wimplicit-function-declaration"
# pragma GCC diagnostic error "-Wimplicit-int-conversion"
# pragma GCC diagnostic error "-Wuninitialized"
# pragma GCC diagnostic error "-Wreturn-type"

# pragma GCC diagnostic error "-Wpedantic"
# pragma GCC diagnostic error "-Wparentheses"
# pragma GCC diagnostic error "-Wstrict-prototypes"
# pragma GCC diagnostic error "-Wc23-extensions"

# pragma GCC diagnostic error "-Wmissing-braces"
# pragma GCC diagnostic error "-Wmissing-field-initializers"

# pragma GCC diagnostic error "-Wformat"
# pragma GCC diagnostic error "-Wformat-pedantic"
# pragma GCC diagnostic error "-Wformat-insufficient-args"

# pragma GCC diagnostic error "-Wvla"
# pragma GCC diagnostic error "-Wundef"
# pragma GCC diagnostic error "-Wmacro-redefined"
# pragma GCC diagnostic error "-Wnewline-eof"

# pragma GCC diagnostic error "-Wenum-compare"
# pragma GCC diagnostic error "-Wincompatible-pointer-types"

#elif defined (_MSC_VER)
# error not implemented
#else
# error not implemented
#endif

#define BUILD_TARGET_NONE      0
#define BUILD_TARGET_TERMINAL  1
#define BUILD_TARGET_GRAPHICAL 2
#define BUILD_TARGET_SHARED    3
#if !defined(BUILD_TARGET)
# define BUILD_TARGET BUILD_TARGET_NONE
#endif

#define BUILD_OPTIMIZE_NONE    0
#define BUILD_OPTIMIZE_INSPECT 1
#define BUILD_OPTIMIZE_DEVELOP 2
#define BUILD_OPTIMIZE_RELEASE 3
#if !defined(BUILD_OPTIMIZE)
# define BUILD_OPTIMIZE BUILD_OPTIMIZE_NONE
#endif

#define BUILD_DEBUG_NONE   0
#define BUILD_DEBUG_ENABLE 1
#if !defined(BUILD_DEBUG)
# define BUILD_DEBUG (BUILD_OPTIMIZE < BUILD_OPTIMIZE_RELEASE)
#endif

#endif
