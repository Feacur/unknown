#if !defined (UKWN_GFX_H)
#define UKWN_GFX_H

#include "base.h" // IWYU pragma: keep

void rhi_init(void);
void rhi_free(void);

void rhi_tick(void);

void rhi_notify_surface_resized(void);

mat4 rhi_mat4_projection(
	vec2 scale_xy, vec2 offset_xy,
	f32 view_near, f32 view_far, f32 ortho
);

#endif
