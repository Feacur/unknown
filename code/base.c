#include <stdio.h>
#include <string.h>
#include <math.h>

#include "os.h"

/*
@note floating point IEEE 754

| TYPE | BINARY REPRESENTATION                | FORMULA                                   | EFFECTIVE EXPONENT |
| f16  | [sign:1][exponent: u5][mantissa:u10] | (1-S*2) * (1 + M/(2^10)) * (2^(E -   15)) | [  -15 ..   15]    |
| f32  | [sign:1][exponent: u8][mantissa:u23] | (1-S*2) * (1 + M/(2^23)) * (2^(E -  127)) | [ -127 ..  127]    |
| f64  | [sign:1][exponent:u11][mantissa:u52] | (1-S*2) * (1 + M/(2^52)) * (2^(E - 1023)) | [-1023 .. 1023]    |

`exponent` is a "window" between powers of two
`mantissa` is an "offset" within a said "window"

for `f32` with `E == 127`  "window" is `[1 .. 2)`
for `f32` with `M == 2^22` "offset" is `0.5`
  result is `1.5`

for `f16` with `E ==   31` result is `infinity` if `M == 0`, otherwise `nan` (quiet if highest bit on, signaling in other case)
for `f32` with `E ==  255` result is `infinity` if `M == 0`, otherwise `nan` (quiet if highest bit on, signaling in other case)
for `f64` with `E == 2047` result is `infinity` if `M == 0`, otherwise `nan` (quiet if highest bit on, signaling in other case)

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

> ROW-MAJOR ORDER means interpreting contiguous memory like this
                    +----------------+
                    |  1   2   3   4 | -> axis_x
                    +----------------+
                    |  5   6   7   8 | -> axis_y
                    +----------------+
                    |  9  10  11  12 | -> axis_z
                    +----------------+
                    | 13  14  15  16 | -> offset
                    +----------------+

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
+-------------------+
|  1 |  5 |  9 | 13 |
|    |    |    |    |
|  2 |  6 | 10 | 14 |
|    |    |    |    |
|  3 |  7 | 11 | 15 |
|    |    |    |    |
|  4 |  8 | 12 | 16 |
+-------------------+
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




@note vectors

> imaginary basis
  +------------+
  |    i  j  k |
  | i -1  k -j |
  | j -k -1  i |
  | k  j -i -1 |
  +------------+
  v = i*a1 + j*a2 + k*a3
  v1 * v2 = (i*a1 + j*a2 + k*a3) * (i*b1 + j*b2 + k*b3)
          = (v1 x v2) - (v1 . v2)

> bivector basis
  +---------------+
  |     x   y   z |
  | x   1  xy  xz |
  | y -xy   1  yz |
  | z -xz -yz   1 |
  +---------------+
  v = x*S1 + y*S2 + z*S3
  v1 * v2 = (x*a1 + y*a2 + z*a3) * (x*b1 + y*b2 + z*b3)
          = (v1 ^ v2) + (v1 . v2)

> inner product (dot)
  v1 . v2 = |v1| * |v2| * cos(a)
  * commutative        `(v1 . v2)      == (v2 . v1)`
  * distributive       `v1 . (v2 + v3) == (v1 . v2) + (v1 . v3)`
  * scalar associative `(v1 . v2) * v3 == v1 * (v2 . v3)`

> outer product (wedge)
  v1 ^ v2 = |v1| * |v2| * sin(a) * I
  `I` is a unit bivector, parallel to `v1` and `v2`
  * anticomutative `(v1 ^ v2)      == -(v2 ^ v1)`
  * distributive   `v1 ^ (v2 + v3) == (v1 ^ v2) + (v1 ^ v3)`
  * IS associative `(v1 ^ v2) ^ v3 == v1 ^ (v2 ^ v3)`

> cross product
  v1 x v2 = |v1| * |v2| * sin(a) * n
  `n` is a unit vector, perpendicular to `v1` and `v2`
  * anticomutative  `(v1 x v2)      == -(v2 x v1)`
  * distributive    `v1 x (v2 + v3) == (v1 x v2) + (v1 x v3)`
  * NOT associative `(v1 x v2) x v3 != v1 x (v2 x v3)`

  scalar triple product `A . (B x C) == (A x B) . C == B . (C x A)`
  vector triple product `A x B x C   == (A . C) * B - (A . B) * C`




@note quaternion

> multiplication
  * NOT commutative `(q1 . q2)      != (q2 . q1)`
  * distributive    `q1 x (q2 + q3) == (q1 x q2) + (q1 x q3)`
  * associative     `(q1 * q2) * q3 != q1 * (q2 * q3)`

> rotation of a vector `V` around an `axis` of rotation by an `angle`)
  - split the vector into two parts, perpendicular to the axis and parallel to it
    V = V_perp + V_para
    V_para = axis * (V . axis)
  - rotate the parts separately
    V_perp` = cos(angle) * V_perp + sin(angle) * (axis x V_perp)
    V_para` = V_para
  - recombine the parts
    V` = V_perp` + V_para`

> Rodrigues' rotation formula (as the said recombined step)
    V` = cos(angle) * V_perp + sin(angle) * (axis x V_perp) + V_para
       = cos(angle) * V      + sin(angle) * (axis x V)      + axis * (V . axis) * (1 - cos(angle))

> Euler's formula
  e^(angle * i)    = cos(angle) + sin(angle) * i
  e^(angle * axis) = cos(angle) + sin(angle) * (axis_x * i + axis_y * j + axis_z * k)

> commutativity of the vector part with the exponential form
  e(angle * axis) * V_perp = V_perp * e(-angle * axis)
  e(angle * axis) * V_para = V_para * e(angle * axis)

> transform into the quaternion form
  V` = cos(angle) * V_perp + sin(angle) * (axis x V_perp) + V_para
     = e^(angle     * axis) * V_perp                         +                                                V_para
     = e^(angle / 2 * axis) * e^(angle / 2 * axis) * V_perp  + e^(angle / 2 * axis) * e^(-angle / 2 * axis) * V_para
     = e^(angle / 2 * axis) * V_perp * e^(-angle / 2 * axis) + e^(angle / 2 * axis) * V_para * e^(-angle / 2 * axis)
     = e^(angle / 2 * axis) * (V_para + V_perp) * e^(-angle / 2 * axis)
     = e^(angle / 2 * axis) * V * e^(-angle / 2 * axis)

> transformation
  `quat_transform` relies on this exact half-angle format,
  allowing us working with arbitrary vectors and axes of rotation




@note matrices

> multiplication
  * NOT comutative `A * B       != B * A`
  * distributive   `A * (B + C) == A * B + A * C`
  * associative    `A * (B * C) == (A * B) * C`
  * transposition  `A * B       == t(t(B) * t(A))`

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
// meta
// ---- ---- ---- ----

size_t get_primitive_type_size(enum Primitive_Type type) {
	switch (type) {
		// unsigned integers
        case PRIMITIVE_TYPE_U8:  return sizeof(u8);
        case PRIMITIVE_TYPE_U16: return sizeof(u16);
        case PRIMITIVE_TYPE_U32: return sizeof(u32);
        case PRIMITIVE_TYPE_U64: return sizeof(u64);
		// signed integers
        case PRIMITIVE_TYPE_S8:  return sizeof(s8);
        case PRIMITIVE_TYPE_S16: return sizeof(s16);
        case PRIMITIVE_TYPE_S32: return sizeof(s32);
        case PRIMITIVE_TYPE_S64: return sizeof(s64);
		// floating points
        case PRIMITIVE_TYPE_F32: return sizeof(f32);
        case PRIMITIVE_TYPE_F64: return sizeof(f64);
    }
    return  0;
}

// ---- ---- ---- ----
// functions, basic
// ---- ---- ---- ----

char base_get_chr(void) {
	return (char)fgetc(stdin);
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
// functions, f32
// ---- ---- ---- ----

bool inf32(f32 value) { return isinf(value); }
bool nan32(f32 value) { return isnan(value); }

f32 sin32(f32 value)  { return sinf(value); }
f32 cos32(f32 value)  { return cosf(value); }
f32 tan32(f32 value)  { return tanf(value); }
f32 sqrt32(f32 value) { return sqrtf(value); }

f32 round32(f32 value) { return roundf(value); }
f32 trunc32(f32 value) { return truncf(value); }
f32 floor32(f32 value) { return floorf(value); }
f32 ceil32(f32 value)  { return ceilf(value); }

f32 exp2_32(f32 value)  { return exp2f(value); }
f32 expe_32(f32 value)  { return expf(value); }
f32 log2_32(f32 value)  { return log2f(value); }
f32 loge_32(f32 value)  { return logf(value); }
f32 log10_32(f32 value) { return log10f(value); }

f32 pow32(f32 base, f32 exp)  { return powf(base, exp); }
f32 ldexp32(f32 mul, s32 exp) { return ldexpf(mul, exp); }

f32 lerp32(f32 v1, f32 v2, f32 t) { return v1 + (v2 - v1)*t; }
f32 lerp32_stable(f32 v1, f32 v2, f32 t) { return v1 * (1 - t) + v2 * t; }
f32 lerp32_inverse(f32 v1, f32 v2, f32 value) { return (value - v1) / (v2 - v1); }

f32 eerp32(f32 v1, f32 v2, f32 t) { return v1 * pow32(v2 / v1, t); }
f32 eerp32_stable(f32 v1, f32 v2, f32 t) { return pow32(v1, (1 - t)) * pow32(v2, t); }
f32 eerp32_inverse(f32 v1, f32 v2, f32 value) { return loge_32(value / v1) / loge_32(v2 / v1); }

f32 prev_f32(f32 value) {
	bits32 bits = {.as_f = value};
	bits.as_u--;
	return bits.as_f;
}

f32 next_f32(f32 value) {
	bits32 bits = {.as_f = value};
	bits.as_u++;
	return bits.as_f;
}

// ---- ---- ---- ----
// functions, f64
// ---- ---- ---- ----

bool inf64(f64 value) { return isinf(value); }
bool nan64(f64 value) { return isnan(value); }

f64 sin64(f64 value)  { return sin(value); }
f64 cos64(f64 value)  { return cos(value); }
f64 tan64(f64 value)  { return tan(value); }
f64 sqrt64(f64 value) { return sqrt(value); }

f64 round64(f64 value) { return round(value); }
f64 trunc64(f64 value) { return trunc(value); }
f64 floor64(f64 value) { return floor(value); }
f64 ceil64(f64 value)  { return ceil(value); }

f64 exp2_64(f64 value)  { return exp2(value); }
f64 expe_64(f64 value)  { return exp(value); }
f64 log2_64(f64 value)  { return log2(value); }
f64 loge_64(f64 value)  { return log(value); }
f64 log10_64(f64 value) { return log10(value); }

f64 pow64(f64 base, f64 exp)  { return pow(base, exp); }
f64 ldexp64(f64 mul, s32 exp) { return ldexp(mul, exp); }

f64 lerp64(f64 v1, f64 v2, f64 t) { return v1 + (v2 - v1)*t; }
f64 lerp64_stable(f64 v1, f64 v2, f64 t) { return v1 * (1 - t) + v2 * t; }
f64 lerp64_inverse(f64 v1, f64 v2, f64 value) { return (value - v1) / (v2 - v1); }

f64 eerp64(f64 v1, f64 v2, f64 t) { return v1 * pow64(v2 / v1, t); }
f64 eerp64_stable(f64 v1, f64 v2, f64 t) { return pow64(v1, (1 - t)) * pow64(v2, t); }
f64 eerp64_inverse(f64 v1, f64 v2, f64 value) { return loge_64(value / v1) / loge_64(v2 / v1); }

f64 prev_f64(f64 value) {
	bits64 bits = {.as_f = value};
	bits.as_u--;
	return bits.as_f;
}

f64 next_f64(f64 value) {
	bits64 bits = {.as_f = value};
	bits.as_u++;
	return bits.as_f;
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
vec2 vec2_muls(vec2 l, f32 r) { return (vec2){l.x * r, l.y * r}; }
vec2 vec2_divs(vec2 l, f32 r) { return (vec2){l.x / r, l.y / r}; }

vec3 vec3_add(vec3 l, vec3 r) { return (vec3){l.x + r.x, l.y + r.y, l.z + r.z}; }
vec3 vec3_sub(vec3 l, vec3 r) { return (vec3){l.x - r.x, l.y - r.y, l.z - r.z}; }
vec3 vec3_mul(vec3 l, vec3 r) { return (vec3){l.x * r.x, l.y * r.y, l.z * r.z}; }
vec3 vec3_div(vec3 l, vec3 r) { return (vec3){l.x / r.x, l.y / r.y, l.z / r.z}; }
f32  vec3_dot(vec3 l, vec3 r) { return l.x * r.x + l.y * r.y + l.z * r.z; }
vec3 vec3_muls(vec3 l, f32 r) { return (vec3){l.x * r, l.y * r, l.z * r}; }
vec3 vec3_divs(vec3 l, f32 r) { return (vec3){l.x / r, l.y / r, l.z / r}; }

vec4 vec4_add(vec4 l, vec4 r) { return (vec4){l.x + r.x, l.y + r.y, l.z + r.z, l.w + r.w}; }
vec4 vec4_sub(vec4 l, vec4 r) { return (vec4){l.x - r.x, l.y - r.y, l.z - r.z, l.w - r.w}; }
vec4 vec4_mul(vec4 l, vec4 r) { return (vec4){l.x * r.x, l.y * r.y, l.z * r.z, l.w * r.w}; }
vec4 vec4_div(vec4 l, vec4 r) { return (vec4){l.x / r.x, l.y / r.y, l.z / r.z, l.w / r.w}; }
f32  vec4_dot(vec4 l, vec4 r) { return l.x * r.x + l.y * r.y + l.z * r.z + l.w * r.w; }
vec4 vec4_muls(vec4 l, f32 r) { return (vec4){l.x * r, l.y * r, l.z * r, l.w * r}; }
vec4 vec4_divs(vec4 l, f32 r) { return (vec4){l.x / r, l.y / r, l.z / r, l.w / r}; }

f32  vec2_crs(vec2 l, vec2 r) { return l.x * r.y - l.y * r.x; }
vec3 vec3_crs(vec3 l, vec3 r) { return (vec3){
	l.y * r.z - l.z * r.y,
	l.z * r.x - l.x * r.z,
	l.x * r.y - l.y * r.x,
}; }

// ---- ---- ---- ----
// functions, f32 math, quaternion
// ---- ---- ---- ----

quat quat_axis(vec3 axis, f32 radians) {
	f32 const h = radians * 0.5f;
	f32 const s = sin32(h);
	f32 const c = cos32(h);
	return (quat){axis.x * s, axis.y * s, axis.z * s, c};
}

quat quat_rotation(vec3 radians) {
	vec3 const h = vec3_mul(radians, (vec3){0.5f, 0.5f, 0.5f});
	vec3 const s = (vec3){sin32(h.x), sin32(h.y), sin32(h.z)};
	vec3 const c = (vec3){cos32(h.x), cos32(h.y), cos32(h.z)};
	float const sy_cx = s.y*c.x; float const cy_sx = c.y*s.x;
	float const cy_cx = c.y*c.x; float const sy_sx = s.y*s.x;
	return (quat){
		sy_cx*s.z + cy_sx*c.z,
		sy_cx*c.z - cy_sx*s.z,
		cy_cx*s.z - sy_sx*c.z,
		cy_cx*c.z + sy_sx*s.z,
	};

/*
ret = quat_axis({0,1,0}, radians_y)
    * quat_axis({1,0,0}, radians_x)
    * quat_axis({0,0,1}, radians_z)
*/
}

quat quat_mul(quat l, quat r) {
	return (quat){
		 l.x * r.w + l.y * r.z - l.z * r.y + l.w * r.x,
		-l.x * r.z + l.y * r.w + l.z * r.x + l.w * r.y,
		 l.x * r.y - l.y * r.x + l.z * r.w + l.w * r.z,
		-l.x * r.x - l.y * r.y - l.z * r.z + l.w * r.w,
	};

/*
ret = (l_v + l_w)
    * (r_v + r_w)

ret = (l_v x r_v) + r_v * l_w + l_v * r_w
    - (l_v . r_v)             + l_w * r_w
*/
}

vec3 quat_transform(quat q, vec3 v) {
	vec3 const cr = vec3_crs((vec3){q.x, q.y, q.z}, v);
	return (vec3){
	//  v   + (q_v x cr                + q_w * cr)   * 2
	/**/v.x + (q.y * cr.z - q.z * cr.y + q.w * cr.x) * 2,
	/**/v.y + (q.z * cr.x - q.x * cr.z + q.w * cr.y) * 2,
	/**/v.z + (q.x * cr.y - q.y * cr.x + q.w * cr.z) * 2,
	};

/*
ret = ( q_v + q_w)
    * (   v +   0)
    * (-q_v + q_w)
    / dot(q, q)

ret = v
    + cross(q_v,  cr * 2)
    +       q_w * cr * 2
*/
}

void quat_get_axes(vec4 q, vec3 * x, vec3 * y, vec3 * z) {
	f32 const xx = q.x*q.x; f32 const xy = q.x*q.y; f32 const xz = q.x*q.z;
	f32 const zw = q.z*q.w; f32 const yy = q.y*q.y; f32 const yz = q.y*q.z;
	f32 const yw = q.y*q.w; f32 const wx = q.w*q.x; f32 const zz = q.z*q.z;
	*x = (vec3){1 - (yy + zz) * 2,     (xy + zw) * 2,     (xz - yw) * 2};
	*y = (vec3){    (xy - zw) * 2, 1 - (zz + xx) * 2,     (yz + wx) * 2};
	*z = (vec3){    (xz + yw) * 2,     (yz - wx) * 2, 1 - (xx + yy) * 2};

/*
x = quat_transform(q, {1,0,0})
y = quat_transform(q, {0,1,0})
z = quat_transform(q, {0,0,1})
*/
}

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

mat4 mat4_transformation(vec3 offset, quat rotation, vec3 scale) {
	vec3 axis_x, axis_y, axis_z;
	quat_get_axes(rotation, &axis_x, &axis_y, &axis_z);
	axis_x = vec3_muls(axis_x, scale.x);
	axis_y = vec3_muls(axis_y, scale.y);
	axis_z = vec3_muls(axis_z, scale.z);
	return (mat4){
		{axis_x.x, axis_x.y, axis_x.z, 0},
		{axis_y.x, axis_y.y, axis_y.z, 0},
		{axis_z.x, axis_z.y, axis_z.z, 0},
		{offset.x, offset.y, offset.z, 1},
	};
}

mat4 mat4_transformation_inverse(vec3 offset, quat rotation, vec3 scale) {
	vec3 axis_x, axis_y, axis_z;
	quat_get_axes(rotation, &axis_x, &axis_y, &axis_z);
	axis_x = vec3_muls(axis_x, scale.x);
	axis_y = vec3_muls(axis_y, scale.y);
	axis_z = vec3_muls(axis_z, scale.z);
	return (mat4){
		// @note inverse rotation matrix is its transposition
		{axis_x.x, axis_y.x, axis_z.x, 0},
		{axis_x.y, axis_y.y, axis_z.y, 0},
		{axis_x.z, axis_y.z, axis_z.z, 0},
		{
			// @note inverse offset in inverted coordinates space
			-vec3_dot(offset, axis_x),
			-vec3_dot(offset, axis_y),
			-vec3_dot(offset, axis_z),
			1
		},
	};
}

mat4 mat4_invert_transformation(mat4 transformation) {
	vec3 const position = (vec3){transformation.w.x, transformation.w.y, transformation.w.z};
	return (mat4){
		{transformation.x.x, transformation.y.x, transformation.z.x, 0},
		{transformation.x.y, transformation.y.y, transformation.z.y, 0},
		{transformation.x.z, transformation.y.z, transformation.z.z, 0},
		{
			-vec3_dot(position, (vec3){transformation.x.x, transformation.x.y, transformation.x.z}),
			-vec3_dot(position, (vec3){transformation.y.x, transformation.y.y, transformation.y.z}),
			-vec3_dot(position, (vec3){transformation.z.x, transformation.z.y, transformation.z.z}),
			1
		},
	};
}

mat4 mat4_projection(
	vec2 scale_xy, vec2 offset_xy, f32 ortho,
	f32 view_near, f32 view_far,
	f32 ndc_near,  f32 ndc_far
) {
	float const reverse_depth = 1 / (view_far - view_near);

	float const persp_scale_z  = inf32(view_far) ? ndc_far                            : (reverse_depth * (ndc_far * view_far - ndc_near * view_near));
	float const persp_offset_z = inf32(view_far) ? ((ndc_near - ndc_far) * view_near) : (reverse_depth * (ndc_near - ndc_far) * view_near * view_far);

	float const ortho_scale_z  = inf32(view_far) ? 0        : (reverse_depth * (ndc_far - ndc_near));
	float const ortho_offset_z = inf32(view_far) ? ndc_near : (reverse_depth * (ndc_near * view_far - ndc_far * view_near));

	float const scale_z  = lerp32(persp_scale_z,  ortho_scale_z,  ortho);
	float const offset_z = lerp32(persp_offset_z, ortho_offset_z, ortho);
	float const zw = 1 - ortho;
	float const ww = ortho;

	return (mat4){
		{scale_xy.x,  0,           0,         0},
		{0,           scale_xy.y,  0,         0},
		{0,           0,           scale_z,  zw},
		{offset_xy.x, offset_xy.y, offset_z, ww},
	};

/*
> aim:
map [-pos_xy   .. pos_xy]   -> [-1       .. 1]
map [view_near .. view_far] -> [ndc_near .. ndc_far]

> known params (aspect correction)
scale_XY = {height / width, 1} / tan(FoV / 2) or {1, width / height} / tan(FoV / 2)
offset_XY = {0, 0}, i.e. the screen center

> known params (direct mapping from bottom-left to top-right)
scale_XY = {2 / width, 2 / height}
offset = {-1, -1}

> orthograhic: NDC = XYZ * scale + offset
Sz = (ndc_far - ndc_near) / (view_far - view_near)
   ~ 0 ; !IF! view_far == infinity
Oz = (ndc_near * view_far - ndc_far * view_near) / (view_far - view_near)
   ~ ndc_near ; !IF! view_far == infinity
| Sx  0   0  Ox |    | x |    | x * Sx + Ox |
| 0   Sy  0  Oy | \/ | y | == | y * Sy + Oy |
| 0   0   Sz Oz | /\ | z | == | z * Sz + Oz |
| 0   0   0  1  |    | 1 |    | 1           |

> perspective: NDC = (XYZ * scale + offset) / z
Sz = (ndc_far * view_far - ndc_near * view_near) / (view_far - view_near)
   ~ ndc_far ; !IF! view_far == infinity
Oz = (ndc_near - ndc_far) * view_near * view_far / (view_far - view_near)
   ~ (ndc_near - ndc_far) * view_near ; !IF! view_far == infinity
| Sx  0   0  Ox |    | x |    | x * Sx + Ox |
| 0   Sy  0  Oy | \/ | y | == | y * Sy + Oy |
| 0   0   Sz Oz | /\ | z | == | z * Sz + Oz |
| 0   0   1  0  |    | 1 |    | z           |
*/
}

// ---- ---- ---- ----
// functions, s32 math, vector
// ---- ---- ---- ----

svec2 svec2_add(svec2 l, svec2 r) { return (svec2){l.x + r.x, l.y + r.y}; }
svec2 svec2_sub(svec2 l, svec2 r) { return (svec2){l.x - r.x, l.y - r.y}; }
svec2 svec2_mul(svec2 l, svec2 r) { return (svec2){l.x * r.x, l.y * r.y}; }
svec2 svec2_div(svec2 l, svec2 r) { return (svec2){l.x / r.x, l.y / r.y}; }
s32   svec2_dot(svec2 l, svec2 r) { return l.x * r.x + l.y * r.y; }
svec2 svec2_muls(svec2 l, s32 r) { return (svec2){l.x * r, l.y * r}; }
svec2 svec2_divs(svec2 l, s32 r) { return (svec2){l.x / r, l.y / r}; }

svec3 svec3_add(svec3 l, svec3 r) { return (svec3){l.x + r.x, l.y + r.y, l.z + r.z}; }
svec3 svec3_sub(svec3 l, svec3 r) { return (svec3){l.x - r.x, l.y - r.y, l.z - r.z}; }
svec3 svec3_mul(svec3 l, svec3 r) { return (svec3){l.x * r.x, l.y * r.y, l.z * r.z}; }
svec3 svec3_div(svec3 l, svec3 r) { return (svec3){l.x / r.x, l.y / r.y, l.z / r.z}; }
s32   svec3_dot(svec3 l, svec3 r) { return l.x * r.x + l.y * r.y + l.z * r.z; }
svec3 svec3_muls(svec3 l, s32 r) { return (svec3){l.x * r, l.y * r, l.z * r}; }
svec3 svec3_divs(svec3 l, s32 r) { return (svec3){l.x / r, l.y / r, l.z / r}; }

svec4 svec4_add(svec4 l, svec4 r) { return (svec4){l.x + r.x, l.y + r.y, l.z + r.z, l.w + r.w}; }
svec4 svec4_sub(svec4 l, svec4 r) { return (svec4){l.x - r.x, l.y - r.y, l.z - r.z, l.w - r.w}; }
svec4 svec4_mul(svec4 l, svec4 r) { return (svec4){l.x * r.x, l.y * r.y, l.z * r.z, l.w * r.w}; }
svec4 svec4_div(svec4 l, svec4 r) { return (svec4){l.x / r.x, l.y / r.y, l.z / r.z, l.w / r.w}; }
s32   svec4_dot(svec4 l, svec4 r) { return l.x * r.x + l.y * r.y + l.z * r.z + l.w * r.w; }
svec4 svec4_muls(svec4 l, s32 r) { return (svec4){l.x * r, l.y * r, l.z * r, l.w * r}; }
svec4 svec4_divs(svec4 l, s32 r) { return (svec4){l.x / r, l.y / r, l.z / r, l.w / r}; }

s32   svec2_crs(svec2 l, svec2 r) { return l.x * r.y - l.y * r.x; }
svec3 svec3_crs(svec3 l, svec3 r) { return (svec3){
	l.y * r.z - l.z * r.y,
	l.z * r.x - l.x * r.z,
	l.x * r.y - l.y * r.x,
}; }

// ---- ---- ---- ----
// functions, u32 math, vector
// ---- ---- ---- ----

uvec2 uvec2_add(uvec2 l, uvec2 r) { return (uvec2){l.x + r.x, l.y + r.y}; }
uvec2 uvec2_sub(uvec2 l, uvec2 r) { return (uvec2){l.x - r.x, l.y - r.y}; }
uvec2 uvec2_mul(uvec2 l, uvec2 r) { return (uvec2){l.x * r.x, l.y * r.y}; }
uvec2 uvec2_div(uvec2 l, uvec2 r) { return (uvec2){l.x / r.x, l.y / r.y}; }
u32   uvec2_dot(uvec2 l, uvec2 r) { return l.x * r.x + l.y * r.y; }
uvec2 uvec2_muls(uvec2 l, u32 r) { return (uvec2){l.x * r, l.y * r}; }
uvec2 uvec2_divs(uvec2 l, u32 r) { return (uvec2){l.x / r, l.y / r}; }

uvec3 uvec3_add(uvec3 l, uvec3 r) { return (uvec3){l.x + r.x, l.y + r.y, l.z + r.z}; }
uvec3 uvec3_sub(uvec3 l, uvec3 r) { return (uvec3){l.x - r.x, l.y - r.y, l.z - r.z}; }
uvec3 uvec3_mul(uvec3 l, uvec3 r) { return (uvec3){l.x * r.x, l.y * r.y, l.z * r.z}; }
uvec3 uvec3_div(uvec3 l, uvec3 r) { return (uvec3){l.x / r.x, l.y / r.y, l.z / r.z}; }
u32   uvec3_dot(uvec3 l, uvec3 r) { return l.x * r.x + l.y * r.y + l.z * r.z; }
uvec3 uvec3_muls(uvec3 l, u32 r) { return (uvec3){l.x * r, l.y * r, l.z * r}; }
uvec3 uvec3_divs(uvec3 l, u32 r) { return (uvec3){l.x / r, l.y / r, l.z / r}; }

uvec4 uvec4_add(uvec4 l, uvec4 r) { return (uvec4){l.x + r.x, l.y + r.y, l.z + r.z, l.w + r.w}; }
uvec4 uvec4_sub(uvec4 l, uvec4 r) { return (uvec4){l.x - r.x, l.y - r.y, l.z - r.z, l.w - r.w}; }
uvec4 uvec4_mul(uvec4 l, uvec4 r) { return (uvec4){l.x * r.x, l.y * r.y, l.z * r.z, l.w * r.w}; }
uvec4 uvec4_div(uvec4 l, uvec4 r) { return (uvec4){l.x / r.x, l.y / r.y, l.z / r.z, l.w / r.w}; }
u32   uvec4_dot(uvec4 l, uvec4 r) { return l.x * r.x + l.y * r.y + l.z * r.z + l.w * r.w; }
uvec4 uvec4_muls(uvec4 l, u32 r) { return (uvec4){l.x * r, l.y * r, l.z * r, l.w * r}; }
uvec4 uvec4_divs(uvec4 l, u32 r) { return (uvec4){l.x / r, l.y / r, l.z / r, l.w / r}; }

// ---- ---- ---- ----
// constants
// ---- ---- ---- ----

bits32 const BITS_MIN32  = ((bits32){.as_u = 0x00000001u});
bits32 const bits_seq32  = ((bits32){.as_u = 0x4b800000u}); // 2^24
bits32 const bits_lim32  = ((bits32){.as_u = 0x7f7fffffu}); // 2^127 * [1 .. 2)
bits32 const bits_inf32  = ((bits32){.as_u = 0x7f800000u}); // 2^128
bits32 const bits_qnan32 = ((bits32){.as_u = 0x7fc00000u}); // 2^128 * (1 .. 2)
bits32 const bits_snan32 = ((bits32){.as_u = 0x7fa00000u}); // 2^128 * (1 .. 2)

bits64 const bits_min64  = ((bits64){.as_u = 0x0000000000000001ull});
bits64 const bits_seq64  = ((bits64){.as_u = 0x4340000000000000ull}); // 2^53
bits64 const bits_lim64  = ((bits64){.as_u = 0x7fefffffffffffffull}); // 2^1023 * [1 .. 2)
bits64 const bits_inf64  = ((bits64){.as_u = 0x7ff0000000000000ull}); // 2^1024
bits64 const bits_qnan64 = ((bits64){.as_u = 0x7ff8000000000000ull}); // 2^1024 * (1 .. 2)
bits64 const bits_snan64 = ((bits64){.as_u = 0x7ff4000000000000ull}); // 2^1024 * (1 .. 2)

// ground truth, 20 digits after the decimal searator                   6.28318530717958647693
bits32 const bits_tau32 = ((bits32){.as_u = 0x40c90fdbu});           // 6.2831854f
bits64 const bits_tau64 = ((bits64){.as_u = 0x401921fb54442d18ull}); // 6.283185307179586

// ground truth, 20 digits after the decimal searator                  3.14159265358979323846
bits32 const bits_pi32 = ((bits32){.as_u = 0x40490fdbu});           // 3.1415927f
bits64 const bits_pi64 = ((bits64){.as_u = 0x400921fb54442d18ull}); // 3.141592653589793

// ground truth, 20 digits after the decimal searator                  2.71828182845904523536
bits32 const bits_e32  = ((bits32){.as_u = 0x402df855u});           // 2.7182819f
bits64 const bits_e64  = ((bits64){.as_u = 0x4005bf0a8b145769ull}); // 2.718281828459045

vec2 const vec2_0  = (vec2){0, 0};
vec2 const vec2_1  = (vec2){1, 1};
vec2 const vec2_x1 = (vec2){1, 0};
vec2 const vec2_y1 = (vec2){0, 1};

vec3 const vec3_0  = (vec3){0, 0, 0};
vec3 const vec3_1  = (vec3){1, 1, 1};
vec3 const vec3_x1 = (vec3){1, 0, 0};
vec3 const vec3_y1 = (vec3){0, 1, 0};
vec3 const vec3_z1 = (vec3){0, 0, 1};

vec4 const vec4_0  = (vec4){0, 0, 0, 0};
vec4 const vec4_1  = (vec4){1, 1, 1, 1};
vec4 const vec4_x1 = (vec4){1, 0, 0, 0};
vec4 const vec4_y1 = (vec4){0, 1, 0, 0};
vec4 const vec4_z1 = (vec4){0, 0, 1, 0};
vec4 const vec4_w1 = (vec4){0, 0, 0, 1};

quat const quat_i = (quat){0, 0, 0, 1};

mat2 const mat2_i = (mat2){
	{1, 0},
	{0, 1},
};

mat3 const mat3_i = (mat3){
	{1, 0, 0},
	{0, 1, 0},
	{0, 0, 1},
};

mat4 const mat4_i = (mat4){
	{1, 0, 0, 0},
	{0, 1, 0, 0},
	{0, 0, 1, 0},
	{0, 0, 0, 1},
};

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
		AssertF(required <= ret.capacity, "file \"%s\" is too large %llu / %zu\n", name, required, ret.capacity);
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
		fwrite(input, sizeof(*input), (size_t)length, stdout);
	}
	return ctx->scratch;
}

uint32_t fmt_print(char * fmt, ...) {
	va_list args;
	va_start(args, fmt);

	struct Fmt_Print_Ctx ctx;
	int const written = stbsp_vsprintfcb(fmt_print_write, &ctx, ctx.scratch, fmt, args);

	va_end(args);
	return (uint32_t)written;
}

struct Fmt_Buffer_Ctx {
	char scratch[STB_SPRINTF_MIN];
	char * output;
};

AttrFileLocal()
char * fmt_buffer_write(char const * input, void * user, int length) {
	struct Fmt_Buffer_Ctx * ctx = user;
	if (length > 0) {
		mem_copy(ctx->scratch, ctx->output, (size_t)length);
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
	return (uint32_t)written;
}
