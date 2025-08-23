#if defined(UKWN_WARNINGS_SUPPRESSED)
#undef UKWN_WARNINGS_SUPPRESSED

#if defined(__clang__)
	#pragma clang diagnostic pop
#elif defined(_MSC_VER)
	#pragma warning(pop)
#endif

#else
	#error include "warnings_push.h" first
#endif
