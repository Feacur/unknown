#if !defined(UKWN_WARNINGS_SUPPRESSED)
#define UKWN_WARNINGS_SUPPRESSED

#if defined(__clang__)
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Weverything"
#elif defined(_MSC_VER)
	#pragma warning(push, 0)
#endif

#else
	#error include "warnings_pop.h" first
#endif
