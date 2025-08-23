#if !defined (UKWN_ENTITY_H)
#define UKWN_ENTITY_H

#include "base.h"

struct Entity {
	u16 type; // `enum Entity_Type`
};

struct Entity * entity_create(u16 type);
void entity_delete(struct Entity * inst);

void entity_tick(struct Entity * inst);
void entity_draw(struct Entity * inst);

#endif
