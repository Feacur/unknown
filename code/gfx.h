#if !defined (UKWN_GFX_H)
#define UKWN_GFX_H

#include "base.h" // IWYU pragma: keep

void gfx_init(void);
void gfx_free(void);

void gfx_tick(void);

void gfx_notify_surface_resized(void);

mat4 gfx_mat4_projection(
	vec2 scale_xy, vec2 offset_xy,
	f32 view_near, f32 view_far, f32 ortho
);

#endif
