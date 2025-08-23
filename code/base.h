#if !defined (UKWN_BASE_H)
#define UKWN_BASE_H

#include <stdbool.h> // IWYU pragma: export
#include <stdint.h> // IWYU pragma: export
#include <stddef.h> // IWYU pragma: export

#include "_project.h" // IWYU pragma: export

// ---- ---- ---- ----
// meta
// ---- ---- ---- ----

enum Meta_Type {
	META_TYPE_NONE,
	META_TYPE_SIZE,
	// unsigned integers
	META_TYPE_U8,
	META_TYPE_U16,
	META_TYPE_U32,
	META_TYPE_U64,
	// signed integers
	META_TYPE_S8,
	META_TYPE_S16,
	META_TYPE_S32,
	META_TYPE_S64,
	// floating points
	META_TYPE_F32,
	META_TYPE_F64,
};

size_t get_meta_type_size(enum Meta_Type type);

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
	f64 as_f;
};

// ---- ---- ---- ----
// types, array
// ---- ---- ---- ----

typedef struct Array_U8 arr8;
struct Array_U8 {
	size_t capacity, count;
	u8 * buffer;
};

typedef struct Array_U16 arr16;
struct Array_U16 {
	size_t capacity, count;
	u16 * buffer;
};

typedef struct Array_U32 arr32;
struct Array_U32 {
	size_t capacity, count;
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

typedef struct Vector4_F32 quat;
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

typedef struct Vector2_S32 svec2;
struct Vector2_S32 {
	s32 x, y;
};

typedef struct Vector3_S32 svec3;
struct Vector3_S32 {
	s32 x, y, z;
};

typedef struct Vector4_S32 svec4;
struct Vector4_S32 {
	s32 x, y, z, w;
};

// ---- ---- ---- ----
// types, u32 math
// ---- ---- ---- ----

typedef struct Vector2_U32 uvec2;
struct Vector2_U32 {
	u32 x, y;
};

typedef struct Vector3_U32 uvec3;
struct Vector3_U32 {
	u32 x, y, z;
};

typedef struct Vector4_U32 uvec4;
struct Vector4_U32 {
	u32 x, y, z, w;
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
// functions, f32
// ---- ---- ---- ----

bool inf32(f32 value);
bool nan32(f32 value);

f32 sin32(f32 value);
f32 cos32(f32 value);
f32 tan32(f32 value);
f32 sqrt32(f32 value);

f32 round32(f32 value);
f32 trunc32(f32 value);
f32 floor32(f32 value);
f32 ceil32(f32 value);

f32 exp2_32(f32 value);
f32 expe_32(f32 value);
f32 log2_32(f32 value);
f32 loge_32(f32 value);
f32 log10_32(f32 value);

f32 pow32(f32 base, f32 exp);
f32 ldexp32(f32 mul, s32 exp); // @note `ret == mul * (2^exp)`

f32 lerp32(f32 v1, f32 v2, f32 t);
f32 lerp32_stable(f32 v1, f32 v2, f32 t);
f32 lerp32_inverse(f32 v1, f32 v2, f32 value);

f32 eerp32(f32 v1, f32 v2, f32 t);
f32 eerp32_stable(f32 v1, f32 v2, f32 t);
f32 eerp32_inverse(f32 v1, f32 v2, f32 value);

f32 prev_f32(f32 value);
f32 next_f32(f32 value);

// ---- ---- ---- ----
// functions, f64
// ---- ---- ---- ----

bool inf64(f64 value);
bool nan64(f64 value);

f64 sin64(f64 value);
f64 cos64(f64 value);
f64 tan64(f64 value);
f64 sqrt64(f64 value);

f64 round64(f64 value);
f64 trunc64(f64 value);
f64 floor64(f64 value);
f64 ceil64(f64 value);

f64 exp2_64(f64 value);
f64 expe_64(f64 value);
f64 log2_64(f64 value);
f64 loge_64(f64 value);
f64 log10_64(f64 value);

f64 pow64(f64 base, f64 exp);
f64 ldexp64(f64 mul, s32 exp); // @note `ret == mul * (2^exp)`

f64 lerp64(f64 v1, f64 v2, f64 t);
f64 lerp64_stable(f64 v1, f64 v2, f64 t);
f64 lerp64_inverse(f64 v1, f64 v2, f64 value);

f64 eerp64(f64 v1, f64 v2, f64 t);
f64 eerp64_stable(f64 v1, f64 v2, f64 t);
f64 eerp64_inverse(f64 v1, f64 v2, f64 value);

f64 prev_f64(f64 value);
f64 next_f64(f64 value);

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
void str16_append(str16 * target, str16 value);
void str32_append(str32 * target, str32 value);

// ---- ---- ---- ----
// functions, f32 math, vector
// ---- ---- ---- ----

vec2 vec2_add(vec2 l, vec2 r);
vec2 vec2_sub(vec2 l, vec2 r);
vec2 vec2_mul(vec2 l, vec2 r);
vec2 vec2_div(vec2 l, vec2 r);
f32  vec2_dot(vec2 l, vec2 r);
vec2 vec2_muls(vec2 l, f32 r);
vec2 vec2_divs(vec2 l, f32 r);

vec3 vec3_add(vec3 l, vec3 r);
vec3 vec3_sub(vec3 l, vec3 r);
vec3 vec3_mul(vec3 l, vec3 r);
vec3 vec3_div(vec3 l, vec3 r);
f32  vec3_dot(vec3 l, vec3 r);
vec3 vec3_muls(vec3 l, f32 r);
vec3 vec3_divs(vec3 l, f32 r);

vec4 vec4_add(vec4 l, vec4 r);
vec4 vec4_sub(vec4 l, vec4 r);
vec4 vec4_mul(vec4 l, vec4 r);
vec4 vec4_div(vec4 l, vec4 r);
f32  vec4_dot(vec4 l, vec4 r);
vec4 vec4_muls(vec4 l, f32 r);
vec4 vec4_divs(vec4 l, f32 r);

f32  vec2_crs(vec2 l, vec2 r);
vec3 vec3_crs(vec3 l, vec3 r);

// ---- ---- ---- ----
// functions, f32 math, quaternion
// ---- ---- ---- ----

quat quat_axis(vec3 axis, f32 radians);
quat quat_rotation(vec3 radians);
quat quat_mul(quat l, quat r);
vec3 quat_transform(quat q, vec3 v);
void quat_get_axes(vec4 q, vec3 * x, vec3 * y, vec3 * z);

// ---- ---- ---- ----
// functions, f32 math, matrix
// ---- ---- ---- ----

vec2 mat2_mul_vec(mat2 l, vec2 r);
mat2 mat2_mul_mat(mat2 l, mat2 r);

vec3 mat3_mul_vec(mat3 l, vec3 r);
mat3 mat3_mul_mat(mat3 l, mat3 r);

vec4 mat4_mul_vec(mat4 l, vec4 r);
mat4 mat4_mul_mat(mat4 l, mat4 r);

mat4 mat4_transformation(vec3 offset, quat rotation, vec3 scale);
mat4 mat4_transformation_inverse(vec3 offset, quat rotation, vec3 scale);
mat4 mat4_invert_transformation(mat4 transformation);

mat4 mat4_projection(
	vec2 scale_xy, vec2 offset_xy, f32 ortho,
	f32 view_near, f32 view_far,
	f32 ndc_near,  f32 ndc_far
);

// ---- ---- ---- ----
// functions, s32 math, vector
// ---- ---- ---- ----

svec2 svec2_add(svec2 l, svec2 r);
svec2 svec2_sub(svec2 l, svec2 r);
svec2 svec2_mul(svec2 l, svec2 r);
svec2 svec2_div(svec2 l, svec2 r);
s32   svec2_dot(svec2 l, svec2 r);
svec2 svec2_muls(svec2 l, s32 r);
svec2 svec2_divs(svec2 l, s32 r);

svec3 svec3_add(svec3 l, svec3 r);
svec3 svec3_sub(svec3 l, svec3 r);
svec3 svec3_mul(svec3 l, svec3 r);
svec3 svec3_div(svec3 l, svec3 r);
s32   svec3_dot(svec3 l, svec3 r);
svec3 svec3_muls(svec3 l, s32 r);
svec3 svec3_divs(svec3 l, s32 r);

svec4 svec4_add(svec4 l, svec4 r);
svec4 svec4_sub(svec4 l, svec4 r);
svec4 svec4_mul(svec4 l, svec4 r);
svec4 svec4_div(svec4 l, svec4 r);
s32   svec4_dot(svec4 l, svec4 r);
svec4 svec4_muls(svec4 l, s32 r);
svec4 svec4_divs(svec4 l, s32 r);

s32   svec2_crs(svec2 l, svec2 r);
svec3 svec3_crs(svec3 l, svec3 r);

// ---- ---- ---- ----
// functions, u32 math, vector
// ---- ---- ---- ----

uvec2 uvec2_add(uvec2 l, uvec2 r);
uvec2 uvec2_sub(uvec2 l, uvec2 r);
uvec2 uvec2_mul(uvec2 l, uvec2 r);
uvec2 uvec2_div(uvec2 l, uvec2 r);
u32   uvec2_dot(uvec2 l, uvec2 r);
uvec2 uvec2_muls(uvec2 l, u32 r);
uvec2 uvec2_divs(uvec2 l, u32 r);

uvec3 uvec3_add(uvec3 l, uvec3 r);
uvec3 uvec3_sub(uvec3 l, uvec3 r);
uvec3 uvec3_mul(uvec3 l, uvec3 r);
uvec3 uvec3_div(uvec3 l, uvec3 r);
u32   uvec3_dot(uvec3 l, uvec3 r);
uvec3 uvec3_muls(uvec3 l, u32 r);
uvec3 uvec3_divs(uvec3 l, u32 r);

uvec4 uvec4_add(uvec4 l, uvec4 r);
uvec4 uvec4_sub(uvec4 l, uvec4 r);
uvec4 uvec4_mul(uvec4 l, uvec4 r);
uvec4 uvec4_div(uvec4 l, uvec4 r);
u32   uvec4_dot(uvec4 l, uvec4 r);
uvec4 uvec4_muls(uvec4 l, u32 r);
uvec4 uvec4_divs(uvec4 l, u32 r);

// ---- ---- ---- ----
// macros
// ---- ---- ---- ----

#define StrToken(token) #token
#define StrMacro(macro) StrToken(macro)

#define CatToken(a, b) a ## b
#define CatMacro(a, b) CatToken(a, b)

#define ArrayCount(array) (sizeof(array) / sizeof((array)[0]))
#define FieldSize(type, name) sizeof(((type *)0)->name)
#define FieldCount(type, name) ArrayCount(((type *)0)->name)

#if defined (__clang__)
# define AlignOf(type) __alignof__(type)
#elif defined (_MSC_VER)
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

#if defined (__clang__) || defined (__GNUC__)
# define AttrThreadLocal() __thread
#elif defined (_MSC_VER)
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

AttrExternal() bits32 const bits_min32;
AttrExternal() bits32 const bits_seq32;
AttrExternal() bits32 const bits_lim32;
AttrExternal() bits32 const bits_inf32;
AttrExternal() bits32 const bits_qnan32;
AttrExternal() bits32 const bits_snan32;

AttrExternal() bits64 const bits_min64;
AttrExternal() bits64 const bits_seq64;
AttrExternal() bits64 const bits_lim64;
AttrExternal() bits64 const bits_inf64;
AttrExternal() bits64 const bits_qnan64;
AttrExternal() bits64 const bits_snan64;

AttrExternal() bits32 const bits_tau32;
AttrExternal() bits64 const bits_tau64;

AttrExternal() bits32 const bits_pi32;
AttrExternal() bits64 const bits_pi64;

AttrExternal() bits32 const bits_e32;
AttrExternal() bits64 const bits_e64;

#define MIN32  bits_min32.as_f
#define SEQ32  bits_seq32.as_f
#define LIM32  bits_lim32.as_f
#define INF32  bits_inf32.as_f
#define QNAN32 bits_qnan32.as_f
#define SNAN32 bits_snan32.as_f

#define MIN64  bits_min64.as_f
#define SEQ64  bits_seq64.as_f
#define LIM64  bits_lim64.as_f
#define INF64  bits_inf64.as_f
#define QNAN64 bits_qnan64.as_f
#define SNAN64 bits_snan64.as_f

#define TAU32 bits_tau32.as_f
#define TAU64 bits_tau64.as_f

#define PI32 bits_pi32.as_f
#define PI64 bits_pi64.as_f

#define E32  bits_e32.as_f
#define E64  bits_e64.as_f

AttrExternal() vec2 const vec2_0;
AttrExternal() vec2 const vec2_1;
AttrExternal() vec2 const vec2_x1;
AttrExternal() vec2 const vec2_y1;

AttrExternal() vec3 const vec3_0;
AttrExternal() vec3 const vec3_1;
AttrExternal() vec3 const vec3_x1;
AttrExternal() vec3 const vec3_y1;
AttrExternal() vec3 const vec3_z1;

AttrExternal() vec4 const vec4_0;
AttrExternal() vec4 const vec4_1;
AttrExternal() vec4 const vec4_x1;
AttrExternal() vec4 const vec4_y1;
AttrExternal() vec4 const vec4_z1;
AttrExternal() vec4 const vec4_w1;

AttrExternal() quat const quat_i;

AttrExternal() mat2 const mat2_i;

AttrExternal() mat3 const mat3_i;

AttrExternal() mat4 const mat4_i;

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

#if defined (__has_builtin) && __has_builtin(__builtin_debugtrap)
// @note it's a clang feature
# define Breakpoint() __builtin_debugtrap()
#elif defined (__has_builtin) && __has_builtin(__builtin_trap)
// @note it's a GCC feature
# define Breakpoint() __builtin_trap()
#elif defined (_MSC_VER)
# define Breakpoint() __debugbreak()
#elif defined (_WIN32)
void DebugBreak(void);
# define Breakpoint() DebugBreak()
#elif defined (__x86_64__)
# define Breakpoint() __asm__ volatile ("int3")
#else
#error not implemented
#endif

#define AssertStatic(condition) \
/**/typedef char assert_static  \
/**/[(condition) ? 1 : -1]      \

#define AssertSize(type, size)           \
/**/AssertStatic(sizeof(type) == (size)) \

#define AssertAlign(type, field, align)                \
/**/AssertStatic(offsetof(type, field) % (align) == 0) \


#if BUILD_DEBUG == BUILD_DEBUG_ENABLE
# define AssertF(condition, msg, ...)        \
/**/do {                                     \
/**/    if (!(condition)) {                  \
/**/        fmt_print("" msg, __VA_ARGS__);  \
/**/        fmt_print("  @ " FileLine "\n"); \
/**/        Breakpoint();                    \
/**/    }                                    \
/**/} while (0)                              \

#else
# define AssertF(condition, msg, ...)           \
/**/(void)(0, (condition), "" msg, __VA_ARGS__) \

#endif


#if BUILD_DEBUG == BUILD_DEBUG_ENABLE
# define Assert(condition, msg)              \
/**/do {                                     \
/**/    if (!(condition)) {                  \
/**/        fmt_print("" msg);               \
/**/        fmt_print("  @ " FileLine "\n"); \
/**/        Breakpoint();                    \
/**/    }                                    \
/**/} while (0)                              \

#else
# define Assert(condition, msg)    \
/**/(void)(0, (condition), "" msg) \

#endif


#if BUILD_DEBUG == BUILD_DEBUG_ENABLE
# define DbgPrintF(fmt, ...)       \
/**/fmt_print("" fmt, __VA_ARGS__) \

#else
# define DbgPrintF(fmt, ...)       \
/**/(void)(0, "" fmt, __VA_ARGS__) \

#endif


#if BUILD_DEBUG == BUILD_DEBUG_ENABLE
# define DbgPrint(fmt) \
/**/fmt_print("" fmt)  \

#else
# define DbgPrint(fmt) \
/**/(void)(0, "" fmt)  \

#endif


#endif
