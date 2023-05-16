//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: linux dependant ASM code for CPU capability detection
//
// $Workfile:     $
// $NoKeywords: $
//=============================================================================//

#define cpuid(in,a,b,c,d)												\
	asm("pushl %%ebx\n\t" "cpuid\n\t" "movl %%ebx,%%esi\n\t" "pop %%ebx": "=a" (a), "=S" (b), "=c" (c), "=d" (d) : "a" (in));

bool CheckMMXTechnology(void)
{
#ifndef PLATFORM_64BITS
    unsigned long eax,ebx,edx,unused;
    cpuid(1,eax,ebx,unused,edx);

    return edx & 0x800000;
#else
    return true;
#endif
}

bool CheckSSETechnology(void)
{
#ifndef PLATFORM_64BITS
    unsigned long eax,ebx,edx,unused;
    cpuid(1,eax,ebx,unused,edx);

    return edx & 0x2000000L;
#else
    return true;
#endif
}

bool CheckSSE2Technology(void)
{
#ifndef PLATFORM_64BITS
    unsigned long eax,ebx,edx,unused;
    cpuid(1,eax,ebx,unused,edx);

    return edx & 0x04000000;
#else
    return true;
#endif
}

bool Check3DNowTechnology(void)
{
#ifndef PLATFORM_64BITS
    unsigned long eax, unused;
    cpuid(0x80000000,eax,unused,unused,unused);

    if ( eax > 0x80000000L )
    {
     	cpuid(0x80000001,unused,unused,unused,eax);
		return ( eax & 1<<31 );
    }
    return false;
#else
    return true;
#endif
}
