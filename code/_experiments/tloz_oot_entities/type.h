#if !defined (UKWN_ENTITY_TYPE_H)
#define UKWN_ENTITY_TYPE_H

#define DEFINE_ENTITY(type) ENTITY_TYPE_ ## type,
enum Entity_Type {
	ENTITY_TYPE_NONE,
	#include "_generator.h"
	ENTITY_TYPE_MAX,
};

#endif
