#ifndef TIER0_CACHE_HINTS_HDR
#define TIER0_CACHE_HINTS_HDR

#if defined(_X360)
#define PREFETCH_CACHE_LINE(POINTER,OFFSET) {__dcbt((OFFSET), (POINTER));}
#elif defined(WIN32)
#define PREFETCH_CACHE_LINE(POINTER,OFFSET) {_mm_prefetch((const char*)(POINTER),(OFFSET));}
#else
#define PREFETCH_CACHE_LINE(POINTER,OFFSET) {}
#endif

#ifdef IVP_VECTOR_INCLUDED
template<class T>
inline void UnsafePrefetchLastElementOf(IVP_U_Vector<T>&array)
{
	PREFETCH_CACHE_LINE(array.element_at(array.len()-1),0);
}
template<class T>
inline void PrefetchLastElementOf(IVP_U_Vector<T>&array)
{
	if(array.len() > 0)
		PREFETCH_CACHE_LINE(array.element_at(array.len()-1),0);
}
#endif

#endif 