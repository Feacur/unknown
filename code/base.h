#if !defined (UKWN_BASE_H)
#define UKWN_BASE_H

#include <stdbool.h> // IWYU pragma: export
#include <stdint.h> // IWYU pragma: export
#include <stddef.h> // IWYU pragma: export

#include "_project.h" // IWYU pragma: export

// ---- ---- ---- ----
// types, basic
// ---- ---- ---- ----

typedef uint8_t   u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t    s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;

typedef float    f32;
typedef double   f64;

// ---- ---- ---- ----
// types, bits
// ---- ---- ---- ----

typedef union Bits32 bits32;
union Bits32 {
	u32 as_u;
	s32 as_s;
	f32 as_f;
};

typedef union Bits64 bits64;
union Bits64 {
	u64 as_u;
	s64 as_s;
	f32 as_f;
};

// ---- ---- ---- ----
// types, array
// ---- ---- ---- ----

typedef struct Array_U8 arr8;
struct Array_U8 {
	size_t capacity;
	size_t count;
	u8 * buffer;
};

typedef struct Array_U16 arr16;
struct Array_U16 {
	size_t capacity;
	size_t count;
	u16 * buffer;
};

typedef struct Array_U32 arr32;
struct Array_U32 {
	size_t capacity;
	size_t count;
	u32 * buffer;
};

// ---- ---- ---- ----
// types, string
// ---- ---- ---- ----

typedef struct String_U8 str8;
struct String_U8 {
	size_t count;
	u8 * buffer;
};

typedef struct String_U16 str16;
struct String_U16 {
	size_t count;
	u16 * buffer;
};

typedef struct String_U32 str32;
struct String_U32 {
	size_t count;
	u32 * buffer;
};

// ---- ---- ---- ----
// types, f32 math
// ---- ---- ---- ----

typedef struct Vector2_F32 vec2;
struct Vector2_F32 {
	f32 x, y;
};

typedef struct Vector3_F32 vec3;
struct Vector3_F32 {
	f32 x, y, z;
};

typedef struct Vector4_F32 vec4;
struct Vector4_F32 {
	f32 x, y, z, w;
};

typedef struct Matrix2_F32 mat2;
struct Matrix2_F32 {
	vec2 x, y;
};

typedef struct Matrix3_F32 mat3;
struct Matrix3_F32 {
	vec3 x, y, z;
};

typedef struct Matrix4_F32 mat4;
struct Matrix4_F32 {
	vec4 x, y, z, w;
};

// ---- ---- ---- ----
// types, s32 math
// ---- ---- ---- ----

typedef struct Vector2_S32 vec2i;
struct Vector2_S32 {
	s32 x, y;
};

typedef struct Vector3_S32 vec3i;
struct Vector3_S32 {
	s32 x, y, z;
};

typedef struct Vector4_S32 vec4i;
struct Vector4_S32 {
	s32 x, y, z, w;
};

// ---- ---- ---- ----
// functions, basic
// ---- ---- ---- ----

char   base_get_chr(void);
char * base_get_str(char * buffer, int limit);

void mem_zero(void * target, size_t size);
void mem_copy(void const * source, void * target, size_t size);
bool str_equals(char const * v1, char const * v2);

size_t align_size(size_t value, size_t align);

u64 mul_div_u64(u64 value, u64 mul, u64 div);

// ---- ---- ---- ----
// functions, limit
// ---- ---- ---- ----

u8  min_u8(u8   v1,  u8 v2);
u16 min_u16(u16 v1, u16 v2);
u32 min_u32(u32 v1, u32 v2);
u64 min_u64(u64 v1, u64 v2);

s8  min_s8(s8   v1,  s8 v2);
s16 min_s16(s16 v1, s16 v2);
s32 min_s32(s32 v1, s32 v2);
s64 min_s64(s64 v1, s64 v2);

f32 min_f32(f32 v1, f32 v2);
f64 min_f64(f64 v1, f64 v2);

long min_long(long v1, long v2);
size_t min_size(size_t v1, size_t v2);

u8  max_u8(u8   v1,  u8 v2);
u16 max_u16(u16 v1, u16 v2);
u32 max_u32(u32 v1, u32 v2);
u64 max_u64(u64 v1, u64 v2);

s8  max_s8(s8   v1,  s8 v2);
s16 max_s16(s16 v1, s16 v2);
s32 max_s32(s32 v1, s32 v2);
s64 max_s64(s64 v1, s64 v2);

f32 max_f32(f32 v1, f32 v2);
f64 max_f64(f64 v1, f64 v2);

long max_long(long v1, long v2);
size_t max_size(size_t v1, size_t v2);

u8  clamp_u8(u8   v,  u8 min,  u8 max);
u16 clamp_u16(u16 v, u16 min, u16 max);
u32 clamp_u32(u32 v, u32 min, u32 max);
u64 clamp_u64(u64 v, u64 min, u64 max);

s8  clamp_s8(s8   v,  s8 min,  s8 max);
s16 clamp_s16(s16 v, s16 min, s16 max);
s32 clamp_s32(s32 v, s32 min, s32 max);
s64 clamp_s64(s64 v, s64 min, s64 max);

f32 clamp_f32(f32 v, f32 min, f32 max);
f64 clamp_f64(f64 v, f64 min, f64 max);

long clamp_long(long v, long min, long max);
size_t clamp_size(size_t v, size_t min, size_t max);

// ---- ---- ---- ----
// functions, array
// ---- ---- ---- ----

void arr8_append_unique(arr8 * target, u8 value);
void arr16_append_unique(arr16 * target, u16 value);
void arr32_append_unique(arr32 * target, u32 value);

// ---- ---- ---- ----
// functions, string
// ---- ---- ---- ----

void str8_append(str8 * target, str8 value);

// ---- ---- ---- ----
// functions, f32 math, vector
// ---- ---- ---- ----

vec2 vec2_add(vec2 l, vec2 r);
vec2 vec2_sub(vec2 l, vec2 r);
vec2 vec2_mul(vec2 l, vec2 r);
vec2 vec2_div(vec2 l, vec2 r);
f32  vec2_dot(vec2 l, vec2 r);

vec3 vec3_add(vec3 l, vec3 r);
vec3 vec3_sub(vec3 l, vec3 r);
vec3 vec3_mul(vec3 l, vec3 r);
vec3 vec3_div(vec3 l, vec3 r);
f32  vec3_dot(vec3 l, vec3 r);

vec4 vec4_add(vec4 l, vec4 r);
vec4 vec4_sub(vec4 l, vec4 r);
vec4 vec4_mul(vec4 l, vec4 r);
vec4 vec4_div(vec4 l, vec4 r);
f32  vec4_dot(vec4 l, vec4 r);

f32  vec2_crs(vec2 l, vec2 r);
vec3 vec3_crs(vec3 l, vec3 r);

// ---- ---- ---- ----
// functions, f32 math, matrix
// ---- ---- ---- ----

vec2 mat2_mul_vec(mat2 l, vec2 r);
mat2 mat2_mul_mat(mat2 l, mat2 r);

vec3 mat3_mul_vec(mat3 l, vec3 r);
mat3 mat3_mul_mat(mat3 l, mat3 r);

vec4 mat4_mul_vec(mat4 l, vec4 r);
mat4 mat4_mul_mat(mat4 l, mat4 r);

// ---- ---- ---- ----
// functions, s32 math, vector
// ---- ---- ---- ----

vec2i vec2i_add(vec2i l, vec2i r);
vec2i vec2i_sub(vec2i l, vec2i r);
vec2i vec2i_mul(vec2i l, vec2i r);
vec2i vec2i_div(vec2i l, vec2i r);
s32   vec2i_dot(vec2i l, vec2i r);

vec3i vec3i_add(vec3i l, vec3i r);
vec3i vec3i_sub(vec3i l, vec3i r);
vec3i vec3i_mul(vec3i l, vec3i r);
vec3i vec3i_div(vec3i l, vec3i r);
s32   vec3i_dot(vec3i l, vec3i r);

vec4i vec4i_add(vec4i l, vec4i r);
vec4i vec4i_sub(vec4i l, vec4i r);
vec4i vec4i_mul(vec4i l, vec4i r);
vec4i vec4i_div(vec4i l, vec4i r);
s32   vec4i_dot(vec4i l, vec4i r);

s32   vec2i_crs(vec2i l, vec2i r);
vec3i vec3i_crs(vec3i l, vec3i r);

// ---- ---- ---- ----
// macros
// ---- ---- ---- ----

#define StrToken(token) #token
#define StrMacro(macro) StrToken(macro)

#define CatToken(a, b) a ## b
#define CatMacro(a, b) CatToken(a, b)

#define ArrayCount(array) (sizeof(array) / sizeof((array)[0]))
#define FieldSize(type, name) sizeof(((type *)0)->name)

#if defined(__clang__)
# define AlignOf(type) __alignof__(type)
#elif defined(_MSC_VER)
# define AlignOf(type) alignof(type)
#else
# error not implemented
#endif

#define AsMillis(seconds) (u64)((seconds) * (u64)1000)
#define AsMicros(seconds) (u64)((seconds) * (u64)1000000)
#define AsNanos(seconds)  (u64)((seconds) * (u64)1000000000)

#define KB(value) ((u64)(value) << 10)
#define MB(value) ((u64)(value) << 20)
#define GB(value) ((u64)(value) << 30)
#define TB(value) ((u64)(value) << 40)

#define str8_lit(value)                \
/**/(struct String_U8){                \
/**/    .count = sizeof("" value) - 1, \
/**/    .buffer = (u8*)(value),        \
/**/}                                  \

// ---- ---- ---- ----
// attributes
// ---- ---- ---- ----

#define AttrFuncLocal() static
#define AttrFileLocal() static
#define AttrExternal() extern

#if defined (__has_attribute) && __has_attribute(fallthrough)
// @note it's a clang / GCC feature
# define AttrFallthrough() __attribute__((fallthrough))
#else
# error not implemented
#endif

#if defined (__clang__) || __has_attribute(__GNUC__)
# define AttrThreadLocal() __thread
#elif defined(_MSC_VER)
# define AttrThreadLocal() __declspec(thread)
#else
# error not implemented
#endif

#if defined (__has_attribute) && __has_attribute(format)
// @note it's a clang / GCC feature
# define AttrPrint(fmt, args)                \
/**/__attribute__((format(printf,fmt,args))) \

#else
# define AttrPrint(fmt, args)
#endif

// ---- ---- ---- ----
// constants
// ---- ---- ---- ----

AttrExternal() const bits32 BITS_F32_SEQ;
AttrExternal() const bits32 BITS_F32_LIM;
AttrExternal() const bits32 BITS_F32_INF;
AttrExternal() const bits32 BITS_F32_QNAN;
AttrExternal() const bits32 BITS_F32_SNAN;

AttrExternal() const bits64 BITS_F64_SEQ;
AttrExternal() const bits64 BITS_F64_LIM;
AttrExternal() const bits64 BITS_F64_INF;
AttrExternal() const bits64 BITS_F64_QNAN;
AttrExternal() const bits64 BITS_F64_SNAN;

#define F32_SEQ  BITS_F32_SEQ.as_f
#define F32_LIM  BITS_F32_LIM.as_f
#define F32_INF  BITS_F32_INF.as_f
#define F32_QNAN BITS_F32_QNAN.as_f
#define F32_SNAN BITS_F32_SNAN.as_f

#define F64_SEQ  BITS_F64_SEQ.as_f
#define F64_LIM  BITS_F64_LIM.as_f
#define F64_INF  BITS_F64_INF.as_f
#define F64_QNAN BITS_F64_QNAN.as_f
#define F64_SNAN BITS_F64_SNAN.as_f

// ---- ---- ---- ----
// memory
// ---- ---- ---- ----

struct Arena;
struct Arena_IInfo {
	size_t reserve;
	size_t commit;
};

struct Arena * arena_init(struct Arena_IInfo info);
void arena_free(struct Arena * arena);

u64 arena_get_position(struct Arena * arena);
void arena_set_position(struct Arena * arena, u64 position);

void * arena_push(struct Arena * arena, size_t size, size_t align);
void arena_pop(struct Arena * arena, size_t size);

#define ArenaPushArray(arena, type, count) (type *)arena_push((arena), sizeof(type) * (count), AlignOf(type))

// ---- ---- ---- ----
// thread context
// ---- ---- ---- ----

void thread_ctx_init(void);
void thread_ctx_free(void);

struct Arena * thread_ctx_get_scratch(void);

// ---- ---- ---- ----
// file utilities
// ---- ---- ---- ----

struct Array_U8 base_file_read(struct Arena * arena, char const * name);

// ---- ---- ---- ----
// formatting
// ---- ---- ---- ----

AttrPrint(1, 2)
uint32_t fmt_print(char * fmt, ...);

AttrPrint(2, 3)
uint32_t fmt_buffer(char * out_buffer, char * fmt, ...);

// ---- ---- ---- ----
// debugging
// ---- ---- ---- ----

#define FileLine __FILE__ ":" StrMacro(__LINE__)

#if defined(__has_builtin) && __has_builtin(__builtin_debugtrap)
// @note it's a clang feature
# define Breakpoint() __builtin_debugtrap()
#elif defined(__has_builtin) && __has_builtin(__builtin_trap)
// @note it's a GCC feature
# define Breakpoint() __builtin_trap()
#elif defined(_MSC_VER)
# define Breakpoint() __debugbreak()
#elif defined(_WIN32)
void DebugBreak();
# define Breakpoint() DebugBreak()
#elif defined(__x86_64__)
# define Breakpoint() __asm__ volatile ("int3")
#else
#error not implemented
#endif

#define AssertStatic(condition) \
/**/typedef char assert_static  \
/**/[(condition) ? 1 : -1]      \

#define AssertSize(type, size)            \
/**/AssertStatic(sizeof(type) == (size)); \

#if BUILD_DEBUG == BUILD_DEBUG_ENABLE
# define Assert(condition, msg, ...)           \
/**/do {                                       \
/**/    if (!(condition)) {                    \
/**/        fmt_print("" msg, ## __VA_ARGS__); \
/**/        fmt_print("  @ " FileLine "\n");   \
/**/        Breakpoint();                      \
/**/    }                                      \
/**/} while (0)                                \

#else
# define Assert(condition, msg, ...)               \
/**/(void)(0, (condition), "" msg, ## __VA_ARGS__) \

#endif

#if BUILD_DEBUG == BUILD_DEBUG_ENABLE
# define DbgPrint(fmt, ...)           \
/**/fmt_print("" fmt, ## __VA_ARGS__) \

#else
# define DbgPrint(fmt, ...)           \
/**/(void)(0, "" fmt, ## __VA_ARGS__) \

#endif

#endif
