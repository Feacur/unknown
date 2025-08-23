#include <stdio.h>
#include <string.h>
#include <math.h>

#include "os.h"

/*
@note floating point IEEE 754

| TYPE | BINARY REPRESENTATION                | FORMULA                          | FINITE EXPONENT |
| f16  | [sign:1][exponent: s5][mantissa:u10] | (1-S*2) * (1 + M/(2^10)) * (2^E) | [  -15 ..   15] |
| f32  | [sign:1][exponent: s8][mantissa:u23] | (1-S*2) * (1 + M/(2^23)) * (2^E) | [ -127 ..  127] |
| f64  | [sign:1][exponent:s11][mantissa:u52] | (1-S*2) * (1 + M/(2^52)) * (2^E) | [-1023 .. 1023] |

`exponent` is a "window" between powers of two
`mantissa` is an "offset" within a said "window"

for `f32` with `E == 0`    "window" is `[1 .. 2)`
for `f32` with `M == 2^22` "offset" is `0.5`
  result is `1.5`

for `f16` with `E ==   -16` result is `infinity` if `M == 0`, otherwise `nan` (quiet if highest bit on, signaling in other case)
for `f32` with `E ==  -128` result is `infinity` if `M == 0`, otherwise `nan` (quiet if highest bit on, signaling in other case)
for `f64` with `E == -1024` result is `infinity` if `M == 0`, otherwise `nan` (quiet if highest bit on, signaling in other case)

consecutive integer range is [-2^(mantissa width + 1) .. 2^(mantissa width + 1)]




@note coordinate system is left-handed

Y (0, 1, 0) up
|     
|  Z (0, 0, 1) forward
| /   
|/    
+----X (1, 0, 0) right

X = cross(Y, Z)
Y = cross(Z, X)
Z = cross(X, Y)




@note designated initializers looks have nothing to do with ordering

V_rm x M_rm = transpose(M_cm x V_cm)
M_cm x V_cm = transpose(V_cm x M_cm)

> ROW-MAJOR ORDER means interpreting contiguous memory like this
                    +---------------+
                    |x_x x_y x_z x_w| -> axis_x
                    +---------------+
                    |y_x y_y y_z y_w| -> axis_y
                    +---------------+
                    |z_x z_y z_z z_w| -> axis_z
                    +---------------+
                    |w_x w_y w_z w_w| -> offset
                    +---------------+

                    +---------------+
                    |x_x x_y x_z x_w|
                    +---------------+
+---------------+   |y_x y_y y_z y_w|   +---------------+
|v_x v_y v_z v_w| x +---------------+ = |r_x r_y r_z r_w|
+---------------+   |z_x z_y z_z z_w|   +---------------+
                    +---------------+
                    |w_x w_y w_z w_w|
                    +---------------+

> COLUMN-MAJOR ORDER in its turn means doing that way around
 axis_x  axis_z
 |       |
+---------------+
|x_x|y_x|z_x|w_x|
|   |   |   |   |
|x_y|y_y|z_y|w_y|
|   |   |   |   |
|x_z|y_z|z_z|w_z|
|   |   |   |   |
|x_w|y_w|z_w|w_w|
+---------------+
     |       |
     axis_y  offset

+---------------+   +---+   +---+
|x_x|y_x|z_x|w_x|   |v_x|   |r_x|
|   |   |   |   |   |   |   |   |
|x_y|y_y|z_y|w_y|   |v_y|   |r_y|
|   |   |   |   | x |   | = |   |
|x_z|y_z|z_z|w_z|   |v_z|   |r_z|
|   |   |   |   |   |   |   |   |
|x_w|y_w|z_w|w_w|   |v_w|   |r_w|
+---------------+   +---+   +---+
*/

// ---- ---- ---- ----
// types
// ---- ---- ---- ----

AssertSize(u8,  1);
AssertSize(u16, 2);
AssertSize(u32, 4);
AssertSize(u64, 8);

AssertSize(s8,  1);
AssertSize(s16, 2);
AssertSize(s32, 4);
AssertSize(s64, 8);

AssertSize(f32, 4);
AssertSize(f64, 8);

// ---- ---- ---- ----
// functions, basic
// ---- ---- ---- ----

char base_get_chr(void) {
	return fgetc(stdin);
}

char * base_get_str(char * buffer, int limit) {
	return fgets(buffer, limit, stdin);
}

void mem_zero(void * target, size_t size) {
	if (target != NULL && size > 0)
		memset(target, 0, size);
}

void mem_copy(void const * source, void * target, size_t size) {
	memcpy(target, source, size);
}

bool str_equals(char const * v1, char const * v2) {
	return strcmp(v1, v2) == 0;
}

size_t align_size(size_t value, size_t align) {
	size_t const mask = max_size(align, 1) - 1;
	return (value + mask) & ~mask;
}

u64 mul_div_u64(u64 value, u64 mul, u64 div) {
	// @note `value * mul / div` equivalent
	// but with a tiny overflow protection
	return (value / div) * mul
	     + (value % div) * mul / div;
}

// ---- ---- ---- ----
// functions, limit
// ---- ---- ---- ----

u8  min_u8(u8   v1,  u8 v2) { return v1 < v2 ? v1 : v2; }
u16 min_u16(u16 v1, u16 v2) { return v1 < v2 ? v1 : v2; }
u32 min_u32(u32 v1, u32 v2) { return v1 < v2 ? v1 : v2; }
u64 min_u64(u64 v1, u64 v2) { return v1 < v2 ? v1 : v2; }

s8  min_s8(s8   v1,  s8 v2) { return v1 < v2 ? v1 : v2; }
s16 min_s16(s16 v1, s16 v2) { return v1 < v2 ? v1 : v2; }
s32 min_s32(s32 v1, s32 v2) { return v1 < v2 ? v1 : v2; }
s64 min_s64(s64 v1, s64 v2) { return v1 < v2 ? v1 : v2; }

f32 min_f32(f32 v1, f32 v2) { return v1 < v2 ? v1 : v2; }
f64 min_f64(f64 v1, f64 v2) { return v1 < v2 ? v1 : v2; }

long min_long(long v1, long v2) { return v1 < v2 ? v1 : v2; }
size_t min_size(size_t v1, size_t v2) { return v1 < v2 ? v1 : v2; }

u8  max_u8(u8   v1,  u8 v2) { return v1 > v2 ? v1 : v2; }
u16 max_u16(u16 v1, u16 v2) { return v1 > v2 ? v1 : v2; }
u32 max_u32(u32 v1, u32 v2) { return v1 > v2 ? v1 : v2; }
u64 max_u64(u64 v1, u64 v2) { return v1 > v2 ? v1 : v2; }

s8  max_s8(s8   v1,  s8 v2) { return v1 > v2 ? v1 : v2; }
s16 max_s16(s16 v1, s16 v2) { return v1 > v2 ? v1 : v2; }
s32 max_s32(s32 v1, s32 v2) { return v1 > v2 ? v1 : v2; }
s64 max_s64(s64 v1, s64 v2) { return v1 > v2 ? v1 : v2; }

f32 max_f32(f32 v1, f32 v2) { return v1 > v2 ? v1 : v2; }
f64 max_f64(f64 v1, f64 v2) { return v1 > v2 ? v1 : v2; }

long max_long(long v1, long v2) { return v1 > v2 ? v1 : v2; }
size_t max_size(size_t v1, size_t v2) { return v1 > v2 ? v1 : v2; }

u8  clamp_u8(u8   v,  u8 min,  u8 max) { return v < min ? min : v > max ? max : v; }
u16 clamp_u16(u16 v, u16 min, u16 max) { return v < min ? min : v > max ? max : v; }
u32 clamp_u32(u32 v, u32 min, u32 max) { return v < min ? min : v > max ? max : v; }
u64 clamp_u64(u64 v, u64 min, u64 max) { return v < min ? min : v > max ? max : v; }

s8  clamp_s8(s8   v,  s8 min,  s8 max) { return v < min ? min : v > max ? max : v; }
s16 clamp_s16(s16 v, s16 min, s16 max) { return v < min ? min : v > max ? max : v; }
s32 clamp_s32(s32 v, s32 min, s32 max) { return v < min ? min : v > max ? max : v; }
s64 clamp_s64(s64 v, s64 min, s64 max) { return v < min ? min : v > max ? max : v; }

f32 clamp_f32(f32 v, f32 min, f32 max) { return v < min ? min : v > max ? max : v; }
f64 clamp_f64(f64 v, f64 min, f64 max) { return v < min ? min : v > max ? max : v; }

long clamp_long(long v, long min, long max) { return v < min ? min : v > max ? max : v; }
size_t clamp_size(size_t v, size_t min, size_t max) { return v < min ? min : v > max ? max : v; }

// ---- ---- ---- ----
// functions, array
// ---- ---- ---- ----

void arr8_append_unique(arr8 * target, u8 value) {
	for (size_t i = 0; i < target->count; i++)
		if (target->buffer[i] == value)
			return;
	target->buffer[target->count++] = value;
}

void arr16_append_unique(arr16 * target, u16 value) {
	for (size_t i = 0; i < target->count; i++)
		if (target->buffer[i] == value)
			return;
	target->buffer[target->count++] = value;
}

void arr32_append_unique(arr32 * target, u32 value) {
	for (size_t i = 0; i < target->count; i++)
		if (target->buffer[i] == value)
			return;
	target->buffer[target->count++] = value;
}

// ---- ---- ---- ----
// functions, string
// ---- ---- ---- ----

void str8_append(str8 * target, str8 value) {
	mem_copy(value.buffer, target->buffer + target->count, value.count);
	target->count += value.count;
}

// ---- ---- ---- ----
// functions, f32 math, vector
// ---- ---- ---- ----

vec2 vec2_add(vec2 l, vec2 r) { return (vec2){l.x + r.x, l.y + r.y}; }
vec2 vec2_sub(vec2 l, vec2 r) { return (vec2){l.x - r.x, l.y - r.y}; }
vec2 vec2_mul(vec2 l, vec2 r) { return (vec2){l.x * r.x, l.y * r.y}; }
vec2 vec2_div(vec2 l, vec2 r) { return (vec2){l.x / r.x, l.y / r.y}; }
f32  vec2_dot(vec2 l, vec2 r) { return l.x * r.x + l.y * r.y; }

vec3 vec3_add(vec3 l, vec3 r) { return (vec3){l.x + r.x, l.y + r.y, l.z + r.z}; }
vec3 vec3_sub(vec3 l, vec3 r) { return (vec3){l.x - r.x, l.y - r.y, l.z - r.z}; }
vec3 vec3_mul(vec3 l, vec3 r) { return (vec3){l.x * r.x, l.y * r.y, l.z * r.z}; }
vec3 vec3_div(vec3 l, vec3 r) { return (vec3){l.x / r.x, l.y / r.y, l.z / r.z}; }
f32  vec3_dot(vec3 l, vec3 r) { return l.x * r.x + l.y * r.y + l.z * r.z; }

vec4 vec4_add(vec4 l, vec4 r) { return (vec4){l.x + r.x, l.y + r.y, l.z + r.z, l.w + r.w}; }
vec4 vec4_sub(vec4 l, vec4 r) { return (vec4){l.x - r.x, l.y - r.y, l.z - r.z, l.w - r.w}; }
vec4 vec4_mul(vec4 l, vec4 r) { return (vec4){l.x * r.x, l.y * r.y, l.z * r.z, l.w * r.w}; }
vec4 vec4_div(vec4 l, vec4 r) { return (vec4){l.x / r.x, l.y / r.y, l.z / r.z, l.w / r.w}; }
f32  vec4_dot(vec4 l, vec4 r) { return l.x * r.x + l.y * r.y + l.z * r.z + l.w * r.w; }

f32  vec2_crs(vec2 l, vec2 r) { return l.x * r.y - l.y * r.x; }
vec3 vec3_crs(vec3 l, vec3 r) { return (vec3){
	l.y * r.z - l.z * r.y,
	l.z * r.x - l.x * r.z,
	l.x * r.y - l.y * r.x,
}; }

// ---- ---- ---- ----
// functions, s32 math, matrix
// ---- ---- ---- ----

vec2 mat2_mul_vec(mat2 l, vec2 r) { return (vec2){
	vec2_dot((vec2){l.x.x, l.y.x}, r),
	vec2_dot((vec2){l.x.y, l.y.y}, r),
}; }

mat2 mat2_mul_mat(mat2 l, mat2 r) { return (mat2){
	mat2_mul_vec(l, r.x),
	mat2_mul_vec(l, r.y),
}; }

vec3 mat3_mul_vec(mat3 l, vec3 r) { return (vec3){
	vec3_dot((vec3){l.x.x, l.y.x, l.z.x}, r),
	vec3_dot((vec3){l.x.y, l.y.y, l.z.y}, r),
	vec3_dot((vec3){l.x.z, l.y.z, l.z.z}, r),
}; }

mat3 mat3_mul_mat(mat3 l, mat3 r) { return (mat3){
	mat3_mul_vec(l, r.x),
	mat3_mul_vec(l, r.y),
	mat3_mul_vec(l, r.z),
}; }

vec4 mat4_mul_vec(mat4 l, vec4 r) { return (vec4){
	vec4_dot((vec4){l.x.x, l.y.x, l.z.x, l.w.x}, r),
	vec4_dot((vec4){l.x.y, l.y.y, l.z.y, l.w.y}, r),
	vec4_dot((vec4){l.x.z, l.y.z, l.z.z, l.w.z}, r),
	vec4_dot((vec4){l.x.w, l.y.w, l.z.w, l.w.w}, r),
}; }

mat4 mat4_mul_mat(mat4 l, mat4 r) { return (mat4){
	mat4_mul_vec(l, r.x),
	mat4_mul_vec(l, r.y),
	mat4_mul_vec(l, r.z),
	mat4_mul_vec(l, r.w),
}; }

// ---- ---- ---- ----
// functions, s32 math, vector
// ---- ---- ---- ----

vec2i vec2i_add(vec2i l, vec2i r) { return (vec2i){l.x + r.x, l.y + r.y}; }
vec2i vec2i_sub(vec2i l, vec2i r) { return (vec2i){l.x - r.x, l.y - r.y}; }
vec2i vec2i_mul(vec2i l, vec2i r) { return (vec2i){l.x * r.x, l.y * r.y}; }
vec2i vec2i_div(vec2i l, vec2i r) { return (vec2i){l.x / r.x, l.y / r.y}; }
s32   vec2i_dot(vec2i l, vec2i r) { return l.x * r.x + l.y * r.y; }

vec3i vec3i_add(vec3i l, vec3i r) { return (vec3i){l.x + r.x, l.y + r.y, l.z + r.z}; }
vec3i vec3i_sub(vec3i l, vec3i r) { return (vec3i){l.x - r.x, l.y - r.y, l.z - r.z}; }
vec3i vec3i_mul(vec3i l, vec3i r) { return (vec3i){l.x * r.x, l.y * r.y, l.z * r.z}; }
vec3i vec3i_div(vec3i l, vec3i r) { return (vec3i){l.x / r.x, l.y / r.y, l.z / r.z}; }
s32   vec3i_dot(vec3i l, vec3i r) { return l.x * r.x + l.y * r.y + l.z * r.z; }

vec4i vec4i_add(vec4i l, vec4i r) { return (vec4i){l.x + r.x, l.y + r.y, l.z + r.z, l.w + r.w}; }
vec4i vec4i_sub(vec4i l, vec4i r) { return (vec4i){l.x - r.x, l.y - r.y, l.z - r.z, l.w - r.w}; }
vec4i vec4i_mul(vec4i l, vec4i r) { return (vec4i){l.x * r.x, l.y * r.y, l.z * r.z, l.w * r.w}; }
vec4i vec4i_div(vec4i l, vec4i r) { return (vec4i){l.x / r.x, l.y / r.y, l.z / r.z, l.w / r.w}; }
s32   vec4i_dot(vec4i l, vec4i r) { return l.x * r.x + l.y * r.y + l.z * r.z + l.w * r.w; }

s32   vec2i_crs(vec2i l, vec2i r) { return l.x * r.y - l.y * r.x; }
vec3i vec3i_crs(vec3i l, vec3i r) { return (vec3i){
	l.y * r.z - l.z * r.y,
	l.z * r.x - l.x * r.z,
	l.x * r.y - l.y * r.x,
}; }

// ---- ---- ---- ----
// constants
// ---- ---- ---- ----

bits32 const BITS_F32_SEQ  = ((bits32){.as_u = 0x4b800000u}); // 2^24
bits32 const BITS_F32_LIM  = ((bits32){.as_u = 0x7f7fffffu}); // 2^127 * [1 .. 2)
bits32 const BITS_F32_INF  = ((bits32){.as_u = 0x7f800000u}); // 2^128
bits32 const BITS_F32_QNAN = ((bits32){.as_u = 0x7fc00000u}); // 2^128 * (1 .. 2)
bits32 const BITS_F32_SNAN = ((bits32){.as_u = 0x7fa00000u}); // 2^128 * (1 .. 2)

bits64 const BITS_F64_SEQ  = ((bits64){.as_u = 0x4340000000000000ull}); // 2^53
bits64 const BITS_F64_LIM  = ((bits64){.as_u = 0x7fefffffffffffffull}); // 2^1023 * [1 .. 2)
bits64 const BITS_F64_INF  = ((bits64){.as_u = 0x7ff0000000000000ull}); // 2^1024
bits64 const BITS_F64_QNAN = ((bits64){.as_u = 0x7ff8000000000000ull}); // 2^1024 * (1 .. 2)
bits64 const BITS_F64_SNAN = ((bits64){.as_u = 0x7ff4000000000000ull}); // 2^1024 * (1 .. 2)


// ---- ---- ---- ----
// file utilities
// ---- ---- ---- ----

struct Array_U8 base_file_read(char const * name) {
	struct Array_U8 ret = {0};
	struct OS_File * file = os_file_init((struct OS_File_IInfo){
		.name = name,
	});
	if (file != NULL) {
		u64 const required = os_file_get_size(file);
		ret.capacity = min_u64(required + 1, ~ret.capacity);
		Assert(required <= ret.capacity, "file \"%s\" is too large %llu / %zu\n", name, required, ret.capacity);
		ret.buffer = os_memory_heap(NULL, ret.capacity);
		ret.count = os_file_read(file, 0, ret.capacity, ret.buffer);
		if (ret.count < ret.capacity) ret.buffer[ret.count] = 0;
		os_file_free(file);
	}
	return ret;
}

// ---- ---- ---- ----
// formatting
// ---- ---- ---- ----

#define STB_SPRINTF_STATIC
#define STB_SPRINTF_IMPLEMENTATION
#include "_internal/warnings_push.h"
# include <stb/stb_sprintf.h>
#include "_internal/warnings_pop.h"

struct Fmt_Print_Ctx {
	char scratch[STB_SPRINTF_MIN];
};

AttrFileLocal()
char * fmt_print_write(char const * input, void * user, int length) {
	struct Fmt_Print_Ctx * ctx = user;
	if (length > 0) {
		fwrite(input, sizeof(*input), length, stdout);
	}
	return ctx->scratch;
}

uint32_t fmt_print(char * fmt, ...) {
	va_list args;
	va_start(args, fmt);

	struct Fmt_Print_Ctx ctx;
	int const written = stbsp_vsprintfcb(fmt_print_write, &ctx, ctx.scratch, fmt, args);

	va_end(args);
	return written;
}

struct Fmt_Buffer_Ctx {
	char scratch[STB_SPRINTF_MIN];
	char * output;
};

AttrFileLocal()
char * fmt_buffer_write(char const * input, void * user, int length) {
	struct Fmt_Buffer_Ctx * ctx = user;
	if (length > 0) {
		mem_copy(ctx->scratch, ctx->output, length);
		ctx->output += length;
	}
	return ctx->scratch;
}

AttrPrint(2, 3)
uint32_t fmt_buffer(char * out_buffer, char * fmt, ...) {
	va_list args;
	va_start(args, fmt);

	struct Fmt_Buffer_Ctx ctx = {.output = out_buffer};
	int const written = stbsp_vsprintfcb(fmt_buffer_write, &ctx, ctx.scratch, fmt, args);

	va_end(args);
	return written;
}
