#if !defined (UKWN_GFX_H)
#define UKWN_GFX_H

#include "base.h" // IWYU pragma: keep

void gfx_init(void);
void gfx_free(void);

void gfx_tick(void);

void gfx_notify_surface_resized(void);

#endif
