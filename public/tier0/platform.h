//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose:
//
// $NoKeywords: $
//
//===========================================================================//

#ifndef PLATFORM_H
#define PLATFORM_H

#ifdef COMPILER_MSVC
#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#if defined( _X360 )
#define NO_STEAM
#define NO_VOICE
// for the 360, the ppc platform and the rtos are tightly coupled
// setup the 360 environment here !once! for much less leaf module include wackiness
// these are critical order and purposely appear *before* anything else
#define _XBOX
#include <xaudio2.h>
#include <xbdm.h>
#include <xgraphics.h>
#include <xui.h>
#include <pmcpbsetup.h>
#include <xmahardwareabstraction.h>
#undef _XBOX

#endif

#include "wchartypes.h"
#include "tier0/valve_off.h"
#ifdef OSX
#include <malloc/malloc.h>
#else
#include <malloc.h>
#endif
#include <limits.h>
#include <float.h>
#include <stdlib.h>
#include <string.h>

#ifdef COMPILER_GCC
#include <new>
#else
#include <new.h>
#endif


//-----------------------------------------------------------------------------
// Old-school defines we don't want to use moving forward
//-----------------------------------------------------------------------------
#if CROSS_PLATFORM_VERSION < 1

// feature enables
#define NEW_SOFTWARE_LIGHTING
#if !defined( _X360 )
#define SUPPORT_PACKED_STORE
#endif
#define BINK_ENABLED_FOR_X360



// Deprecating, infavor of IsX360() which will revert to IsXbox()
// after confidence of xbox 1 code flush
#define IsXbox()	false

// C functions for external declarations that call the appropriate C++ methods
#ifndef EXPORT
	#ifdef _WIN32
		#define EXPORT	_declspec( dllexport )
	#else
		#define EXPORT	/* */
	#endif
#endif

#ifdef PLATFORM_POSIX
typedef unsigned int DWORD;
typedef unsigned short WORD;
typedef void * HINSTANCE;
#define _MAX_PATH PATH_MAX
#endif

#endif // CROSS_PLATFORM_VERSION < 1


//-----------------------------------------------------------------------------
// NOTE: All compiler defines are set up in the base VPC scripts
// COMPILER_MSVC, COMPILER_MSVC32, COMPILER_MSVC64, COMPILER_MSVCX360
// COMPILER_GCC
// The rationale for this is that we need COMPILER_MSVC for the pragma blocks
// #pragma once that occur at the top of all header files, therefore we can't
// place the defines for these in here.
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Set up platform defines.
//-----------------------------------------------------------------------------
#ifdef _WIN32
	#define IsPlatformLinux()	false
	#define IsPlatformPosix()	false
	#define IsPlatformOSX()		false
	#define IsPlatformPS3()		false
	#define IsPlatformWindows() true
	#ifndef PLATFORM_WINDOWS
	#define PLATFORM_WINDOWS	1
	#endif

	#ifndef _X360
		#define IsPlatformX360() false
		#define IsPlatformWindowsPC() true
		#define PLATFORM_WINDOWS_PC 1

		#ifdef _WIN64
			#define IsPlatformWindowsPC64() true
			#define IsPlatformWindowsPC32() false
			#define PLATFORM_WINDOWS_PC64 1
		#else
			#define IsPlatformWindowsPC64() false
			#define IsPlatformWindowsPC32() true
			#define PLATFORM_WINDOWS_PC32 1
		#endif

	#else // _X360

		#define IsPlatformWindowsPC()	false
		#define IsPlatformWindowsPC64() false
		#define IsPlatformWindowsPC32() false
		#define IsPlatformX360()		true
		#define PLATFORM_X360 1

	#endif // _X360

#elif defined(POSIX)
	#define IsPlatformX360()		false
	#define IsPlatformPS3()			false
	#define IsPlatformWindows()		false
	#define IsPlatformWindowsPC()	false
	#define IsPlatformWindowsPC64()	false
	#define IsPlatformWindowsPC32()	false
	#define IsPlatformPosix()		true
	#ifndef PLATFORM_POSIX
	#define PLATFORM_POSIX 1
	#endif

	#if defined( LINUX )
		#define IsPlatformLinux() true
		#define IsPlatformOSX() false
		#ifndef PLATFORM_LINUX
		#define PLATFORM_LINUX 1
		#endif
	#elif defined ( OSX )
		#define IsPlatformLinux() false
		#define IsPlatformOSX() true
		#ifndef PLATFORM_OSX
		#define PLATFORM_OSX 1
		#endif
	#else
		#define IsPlatformLinux() false
		#define IsPlatformOSX() false
	#endif

#else
	#error
#endif


//-----------------------------------------------------------------------------
// Old-school defines we're going to support since much code uses them
//-----------------------------------------------------------------------------
#if CROSS_PLATFORM_VERSION < 2

#define IsLinux()	IsPlatformLinux() 
#define IsOSX()		IsPlatformOSX()
#define IsPosix()	IsPlatformPosix()
#define IsX360()	IsPlatformX360()
#define IsPS3()		IsPlatformPS3()

// Setup platform defines.
#ifdef COMPILER_MSVC
#define MSVC 1
#endif

#ifdef COMPILER_GCC
#define GNUC 1
#endif

#if defined( _WIN32 )
#define _WINDOWS 1
#endif

#ifdef PLATFORM_WINDOWS_PC
#define IS_WINDOWS_PC 1
#endif

#endif // CROSS_PLATFORM_VERSION < 2


//-----------------------------------------------------------------------------
// Set up platform type defines.
//-----------------------------------------------------------------------------
#ifdef PLATFORM_X360
	#ifndef _CONSOLE
		#define _CONSOLE
	#endif
	#define IsPC()		false
	#define IsConsole() true
#else
	#define IsPC()		true
	#define IsConsole() false
#endif



//-----------------------------------------------------------------------------
// Set up build configuration defines.
//-----------------------------------------------------------------------------
#ifdef _CERT
#define IsCert() true
#else
#define IsCert() false
#endif

#ifdef _DEBUG
#define IsRelease() false
#define IsDebug() true
#else
#define IsRelease() true
#define IsDebug() false
#endif

#ifdef _RETAIL
#define IsRetail() true
#else
#define IsRetail() false
#endif


//-----------------------------------------------------------------------------
// Portable data types
//-----------------------------------------------------------------------------
typedef unsigned char				uint8;
typedef signed char					int8;

#if defined( COMPILER_MSVC )

	typedef __int16					int16;
	typedef unsigned __int16		uint16;
	typedef __int32					int32;
	typedef unsigned __int32		uint32;
	typedef __int64					int64;
	typedef unsigned __int64		uint64;

	// intp is an integer that can accomodate a pointer
	// (ie, sizeof(intp) >= sizeof(int) && sizeof(intp) >= sizeof(void *)
	typedef intptr_t				intp;		
	typedef uintptr_t				uintp;		

	#if defined( COMPILER_MSVCX360 )
		#ifdef __m128
			#undef __m128
		#endif
		#define __m128				__vector4
	#endif

#else // !COMPILER_MSVC

	typedef short					int16;
	typedef unsigned short			uint16;
	typedef int						int32;
	typedef unsigned int			uint32;
	typedef long long				int64;
	typedef unsigned long long		uint64;
	#ifdef PLATFORM_64BITS
		typedef long long			intp;
		typedef unsigned long long	uintp;
	#else
		typedef int					intp;
		typedef unsigned int		uintp;
	#endif
	typedef void *HWND;

#endif // else COMPILER_MSVC

typedef float				float32;
typedef double				float64;

// for when we don't care about how many bits we use
typedef unsigned int		uint;


// Maximum and minimum representable values
#if !defined(PLATFORM_OSX) && !defined(__STDC_LIMIT_MACROS)

#ifndef INT8_MAX
#define  INT8_MAX			SCHAR_MAX
#endif
#ifndef INT16_MAX
#define  INT16_MAX			SHRT_MAX
#endif
#ifndef INT32_MAX
#define  INT32_MAX			LONG_MAX
#endif
#ifndef INT64_MAX
#define  INT64_MAX			(((int64)~0) >> 1)
#endif

#ifndef INT8_MIN
#define  INT8_MIN			SCHAR_MIN
#endif
#ifndef INT16_MIN
#define  INT16_MIN			SHRT_MIN
#endif
#ifndef INT32_MIN
#define  INT32_MIN			LONG_MIN
#endif
#ifndef INT64_MIN
#define  INT64_MIN			(((int64)1) << 63)
#endif

#ifndef UINT8_MAX
#define  UINT8_MAX			((uint8)~0)
#endif
#ifndef UINT16_MAX
#define  UINT16_MAX			((uint16)~0)
#endif
#ifndef UINT32_MAX
#define  UINT32_MAX			((uint32)~0)
#endif
#ifndef UINT64_MAX
#define  UINT64_MAX			((uint64)~0)
#endif

#ifndef UINT8_MIN
#define  UINT8_MIN			0
#endif
#ifndef UINT16_MIN
#define  UINT16_MIN			0
#endif
#ifndef UINT32_MIN
#define  UINT32_MIN			0
#endif
#ifndef UINT64_MIN
#define  UINT64_MIN			0
#endif

#endif // !PLATFORM_OSX && !__STDC_LIMIT_MACROS

#ifndef  UINT_MIN
#define  UINT_MIN			UINT32_MIN
#endif

#define  FLOAT32_MAX		FLT_MAX
#define  FLOAT64_MAX		DBL_MAX

#ifdef GNUC
#undef offsetof
//#define offsetof( type, var ) __builtin_offsetof( type, var ) 
#define offsetof(s,m)	(size_t)&(((s *)0)->m)
#else
#include <stddef.h>
#undef offsetof
#define offsetof(s,m)	(size_t)&(((s *)0)->m)
#endif


#define  FLOAT32_MIN		FLT_MIN
#define  FLOAT64_MIN		DBL_MIN

//-----------------------------------------------------------------------------
// Long is evil because it's treated differently by different compilers
// Preventing its use is nasty however. This #define, which should be
// turned on in individual VPC files, causes you to include tier0/valve_off.h
// before standard C + windows headers, and include tier0/valve_on.h after
// standard C + windows headers. So, there's some painful overhead to disabling long
//-----------------------------------------------------------------------------
#ifdef DISALLOW_USE_OF_LONG
	#define long			long_is_the_devil_stop_using_it_use_int32_or_int64
#endif


//-----------------------------------------------------------------------------
// Various compiler-specific keywords
//-----------------------------------------------------------------------------
#ifdef COMPILER_MSVC

	#ifdef FORCEINLINE
		#undef FORCEINLINE
	#endif
	#define STDCALL					__stdcall
	#ifndef FASTCALL
		#define  FASTCALL			__fastcall
	#endif
	#define FORCEINLINE				__forceinline
	#define FORCEINLINE_TEMPLATE	__forceinline
	#define NULLTERMINATED			__nullterminated

	// This can be used to ensure the size of pointers to members when declaring
	// a pointer type for a class that has only been forward declared
	#define SINGLE_INHERITANCE		__single_inheritance
	#define MULTIPLE_INHERITANCE	__multiple_inheritance
	#define EXPLICIT				explicit
	#define NO_VTABLE				__declspec( novtable )

	// gcc doesn't allow storage specifiers on explicit template instatiation, but visual studio needs them to avoid link errors.
	#define TEMPLATE_STATIC			static

	// Used for dll exporting and importing
	#define DLL_EXPORT				extern "C" __declspec( dllexport )
	#define DLL_IMPORT				extern "C" __declspec( dllimport )

	// Can't use extern "C" when DLL exporting a class
	#define DLL_CLASS_EXPORT		__declspec( dllexport )
	#define DLL_CLASS_IMPORT		__declspec( dllimport )

	// Can't use extern "C" when DLL exporting a global
	#define DLL_GLOBAL_EXPORT		extern __declspec( dllexport )
	#define DLL_GLOBAL_IMPORT		extern __declspec( dllimport )

	// Pass hints to the compiler to prevent it from generating unnessecary / stupid code
	// in certain situations.  Several compilers other than MSVC also have an equivilent
	// construct.
	//
	// Essentially the 'Hint' is that the condition specified is assumed to be true at
	// that point in the compilation.  If '0' is passed, then the compiler assumes that
	// any subsequent code in the same 'basic block' is unreachable, and thus usually
	// removed.
	#define HINT(THE_HINT)			__assume((THE_HINT))

	// decls for aligning data
	#define DECL_ALIGN(x)			__declspec( align( x ) )

	// GCC had a few areas where it didn't construct objects in the same order 
	// that Windows does. So when CVProfile::CVProfile() would access g_pMemAlloc,
	// it would crash because the allocator wasn't initalized yet.
	#define CONSTRUCT_EARLY

	#define SELECTANY				__declspec(selectany)

	#define RESTRICT				__restrict
	#define RESTRICT_FUNC			__declspec(restrict)
	#define FMTFUNCTION( a, b )
	#define NOINLINE

#if !defined( NO_THREAD_LOCAL )
	#define DECL_THREAD_LOCAL		__declspec(thread)
#endif 

	#define DISABLE_VC_WARNING( x ) __pragma(warning(disable:4310) )
	#define DEFAULT_VC_WARNING( x ) __pragma(warning(default:4310) )


#elif defined ( COMPILER_GCC )

	#if (CROSS_PLATFORM_VERSION >= 1) && !defined( PLATFORM_64BITS )
		#define  STDCALL			__attribute__ ((__stdcall__))
	#else
		#define  STDCALL
		#define  __stdcall			__attribute__ ((__stdcall__))
	#endif

	#define  FASTCALL
	#ifdef _LINUX_DEBUGGABLE
		#define  FORCEINLINE
	#else
		#define  FORCEINLINE		inline
	#endif

	// GCC 3.4.1 has a bug in supporting forced inline of templated functions
	// this macro lets us not force inlining in that case
	#define FORCEINLINE_TEMPLATE	inline
	#define SINGLE_INHERITANCE
	#define MULTIPLE_INHERITANCE
	#define EXPLICIT
	#define NO_VTABLE

	#define NULLTERMINATED			

	#define TEMPLATE_STATIC

	// Used for dll exporting and importing
	#define DLL_EXPORT				extern "C" __attribute__ ((visibility("default")))
	#define DLL_IMPORT				extern "C"

	// Can't use extern "C" when DLL exporting a class
	#define DLL_CLASS_EXPORT		__attribute__ ((visibility("default")))
	#define DLL_CLASS_IMPORT

	// Can't use extern "C" when DLL exporting a global
	#define DLL_GLOBAL_EXPORT		__attribute__((visibility("default")))
	#define DLL_GLOBAL_IMPORT		extern

	#define HINT(THE_HINT)			0
	#define DECL_ALIGN(x)			__attribute__( ( aligned( x ) ) )
	#define CONSTRUCT_EARLY			__attribute__((init_priority(101)))
	#define SELECTANY				__attribute__((weak))
	#define RESTRICT
	#define RESTRICT_FUNC
	#define FMTFUNCTION( fmtargnumber, firstvarargnumber ) __attribute__ (( format( printf, fmtargnumber, firstvarargnumber )))
	#define NOINLINE				__attribute__ ((noinline))

#if !defined( NO_THREAD_LOCAL )
	#define DECL_THREAD_LOCAL		__thread
#endif

	#define DISABLE_VC_WARNING( x )
	#define DEFAULT_VC_WARNING( x )

#else

	#define DECL_ALIGN(x)			/* */
	#define SELECTANY				static

#endif

#if defined( GNUC )
// gnuc has the align decoration at the end
#define ALIGN4
#define ALIGN8 
#define ALIGN16
#define ALIGN32
#define ALIGN128

#define ALIGN4_POST DECL_ALIGN(4)
#define ALIGN8_POST DECL_ALIGN(8)
#define ALIGN16_POST DECL_ALIGN(16)
#define ALIGN32_POST DECL_ALIGN(32)
#define ALIGN128_POST DECL_ALIGN(128)
#else
// MSVC has the align at the start of the struct
#define ALIGN4 DECL_ALIGN(4)
#define ALIGN8 DECL_ALIGN(8)
#define ALIGN16 DECL_ALIGN(16)
#define ALIGN32 DECL_ALIGN(32)
#define ALIGN128 DECL_ALIGN(128)

#define ALIGN4_POST
#define ALIGN8_POST
#define ALIGN16_POST
#define ALIGN32_POST
#define ALIGN128_POST
#endif


//-----------------------------------------------------------------------------
// Convert int<-->pointer, avoiding 32/64-bit compiler warnings:
//-----------------------------------------------------------------------------
#define INT_TO_POINTER( i ) (void *)( ( i ) + (char *)NULL )
#define POINTER_TO_INT( p ) ( (int)(uintp)( p ) )


// This can be used to declare an abstract (interface only) class.
// Classes marked abstract should not be instantiated.  If they are, and access violation will occur.
//
// Example of use:
//
// abstract_class CFoo
// {
//      ...
// }
//
// MSDN __declspec(novtable) documentation: http://msdn.microsoft.com/library/default.asp?url=/library/en-us/vclang/html/_langref_novtable.asp
//
// Note: NJS: This is not enabled for regular PC, due to not knowing the implications of exporting a class with no no vtable.
//       It's probable that this shouldn't be an issue, but an experiment should be done to verify this.
//
#ifndef COMPILER_MSVCX360
	#define abstract_class class
#else
	#define abstract_class class NO_VTABLE
#endif


//-----------------------------------------------------------------------------
// Why do we need this? It would be nice to make it die die die
//-----------------------------------------------------------------------------
// Alloca defined for this platform
#if defined( COMPILER_MSVC ) && !defined( WINDED )
	#if defined(_M_IX86)
		#define __i386__	1
	#endif
#endif

#if defined __i386__ && !defined __linux__
	#define id386	1
#else
	#define id386	0
#endif  // __i386__


//-----------------------------------------------------------------------------
// Disable annoying unhelpful warnings
//-----------------------------------------------------------------------------
#ifdef COMPILER_MSVC
// Remove warnings from warning level 4.
#pragma warning(disable : 4514) // warning C4514: 'acosl' : unreferenced inline function has been removed
#pragma warning(disable : 4100) // warning C4100: 'hwnd' : unreferenced formal parameter
#pragma warning(disable : 4127) // warning C4127: conditional expression is constant
#pragma warning(disable : 4512) // warning C4512: 'InFileRIFF' : assignment operator could not be generated
#pragma warning(disable : 4611) // warning C4611: interaction between '_setjmp' and C++ object destruction is non-portable
#pragma warning(disable : 4710) // warning C4710: function 'x' not inlined
#pragma warning(disable : 4702) // warning C4702: unreachable code
#pragma warning(disable : 4505) // unreferenced local function has been removed
#pragma warning(disable : 4239) // nonstandard extension used : 'argument' ( conversion from class Vector to class Vector& )
#pragma warning(disable : 4097) // typedef-name 'BaseClass' used as synonym for class-name 'CFlexCycler::CBaseFlex'
#pragma warning(disable : 4324) // Padding was added at the end of a structure
#pragma warning(disable : 4244) // type conversion warning.
#pragma warning(disable : 4305)	// truncation from 'const double ' to 'float '
#pragma warning(disable : 4786)	// Disable warnings about long symbol names
#pragma warning(disable : 4250) // 'X' : inherits 'Y::Z' via dominance
#pragma warning(disable : 4201) // nonstandard extension used : nameless struct/union

#if _MSC_VER >= 1300
#pragma warning(disable : 4511)	// Disable warnings about private copy constructors
#pragma warning(disable : 4121)	// warning C4121: 'symbol' : alignment of a member was sensitive to packing
#pragma warning(disable : 4530)	// warning C4530: C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc (disabled due to std headers having exception syntax)
#endif

#if _MSC_VER >= 1400
#pragma warning(disable : 4996)	// functions declared deprecated
#endif

// When we port to 64 bit, we'll have to resolve the int, ptr vs size_t 32/64 bit problems...
#if !defined( COMPILER_MSVC64 )
#if ( CROSS_PLATFORM_VERSION < 1 )
#pragma warning( disable : 4267 )	// conversion from 'size_t' to 'int', possible loss of data
#pragma warning( disable : 4311 )	// pointer truncation from 'char *' to 'int'
#pragma warning( disable : 4312 )	// conversion from 'unsigned int' to 'memhandle_t' of greater size
#endif
#endif

#endif


//-----------------------------------------------------------------------------
// Stack-based allocation related helpers
//-----------------------------------------------------------------------------
#if defined( COMPILER_GCC )

	#define stackalloc( _size )		alloca( ALIGN_VALUE( _size, 16 ) )

	#ifdef PLATFORM_OSX
		#define mallocsize( _p )	( malloc_size( _p ) )
	#else
		#define mallocsize( _p )	( malloc_usable_size( _p ) )
	#endif

#elif defined ( COMPILER_MSVC )

	#define stackalloc( _size )		_alloca( ALIGN_VALUE( _size, 16 ) )
	#define mallocsize( _p )		( _msize( _p ) )

#endif

#define  stackfree( _p )			0


//-----------------------------------------------------------------------------
// Used to break into the debugger
//-----------------------------------------------------------------------------
#ifdef COMPILER_MSVC64
	#define DebuggerBreak()		__debugbreak()
#elif COMPILER_MSVC32
	#define DebuggerBreak()		__asm { int 3 }
#elif COMPILER_MSVCX360
	#define DebuggerBreak()		DebugBreak()
#elif COMPILER_GCC
	#if defined( PLATFORM_OSX )
		// On OSX, SIGTRAP doesn't really stop the thread cold when debugging.
		// So if being debugged, use INT3 which is precise.
		#define DebuggerBreak()  { if ( Plat_IsInDebugSession() ) { __asm ( "int $3" ); } else { raise(SIGTRAP); } }
	#elif defined( PLATFORM_CYGWIN ) || defined( PLATFORM_POSIX )
		#define DebuggerBreak()		__asm__( "int $0x3;")
	#else
		#define DebuggerBreak()		asm( "int3" )
	#endif
#endif


//-----------------------------------------------------------------------------
// DLL export for platform utilities
//-----------------------------------------------------------------------------
#ifndef STATIC_TIER0

#ifdef TIER0_DLL_EXPORT
#define PLATFORM_INTERFACE	DLL_EXPORT
#define PLATFORM_OVERLOAD	DLL_GLOBAL_EXPORT
#define PLATFORM_CLASS		DLL_CLASS_EXPORT
#else
#define PLATFORM_INTERFACE	DLL_IMPORT
#define PLATFORM_OVERLOAD	DLL_GLOBAL_IMPORT
#define PLATFORM_CLASS		DLL_CLASS_IMPORT
#endif

#else	// BUILD_AS_DLL

#define PLATFORM_INTERFACE	extern
#define PLATFORM_OVERLOAD
#define PLATFORM_CLASS

#endif	// BUILD_AS_DLL


//-----------------------------------------------------------------------------
// Returns true if debugger attached, false otherwise
//-----------------------------------------------------------------------------
#if defined( PLATFORM_WINDOWS ) || defined( PLATFORM_LINUX ) || defined( PLATFORM_OSX )
PLATFORM_INTERFACE bool Plat_IsInDebugSession();
PLATFORM_INTERFACE void Plat_DebugString( const tchar * );
#else
inline bool Plat_IsInDebugSession() { return false; }
#define Plat_DebugString(s) ((void)0)
#endif

#define	DebuggerBreakIfDebugging() if ( !Plat_IsInDebugSession() ) ; else DebuggerBreak()


//-----------------------------------------------------------------------------
// Message Box
//-----------------------------------------------------------------------------
#if defined( PLATFORM_WINDOWS_PC )
PLATFORM_INTERFACE void Plat_MessageBox( const char *pTitle, const tchar *pMessage );
#else
#define Plat_MessageBox( t, m ) ((void)0)
#endif


PLATFORM_INTERFACE void Plat_Printf ( const tchar *pFmt, ... ) FMTFUNCTION( 1, 2 );

//-----------------------------------------------------------------------------
// Posix platform helpers
//-----------------------------------------------------------------------------
#ifdef PLATFORM_POSIX

// Visual Studio likes to put an underscore in front of anything that looks like a portable function.
#define _strupr strupr
#define _getcwd getcwd
#define _open open
#define _lseek lseek
#define _read read
#define _close close
#define _vsnprintf vsnprintf
#define _stat stat
#define _O_RDONLY O_RDONLY
#define _stricmp strcasecmp
#define _finite finite
#define _unlink unlink
#define _putenv putenv
#define _chdir chdir
#define _access access
#define _strtoi64 strtoll

#if !defined( _snprintf )	// some vpc's define this on the command line
#define _snprintf snprintf
#endif

#include <alloca.h>
#include <unistd.h>											// get unlink
#include <signal.h>
#include <errno.h>

#endif // PLATFORM_POSIX


//-----------------------------------------------------------------------------
// Generally useful platform-independent macros (move to another file?)
//-----------------------------------------------------------------------------

// need macro for constant expression
#define ALIGN_VALUE( val, alignment ) ( ( val + alignment - 1 ) & ~( alignment - 1 ) ) 

// Force a function call site -not- to inlined. (useful for profiling)
#define DONT_INLINE(a) (((int)(a)+1)?(a):(a))

// Marks the codepath from here until the next branch entry point as unreachable,
// and asserts if any attempt is made to execute it.
#define UNREACHABLE() { Assert(0); HINT(0); }

// In cases where no default is present or appropriate, this causes MSVC to generate
// as little code as possible, and throw an assertion in debug.
#define NO_DEFAULT default: UNREACHABLE();

// Defines MAX_PATH
#ifndef MAX_PATH
	#define MAX_PATH  260
#endif


//-----------------------------------------------------------------------------
// FP exception handling
//-----------------------------------------------------------------------------
//#define CHECK_FLOAT_EXCEPTIONS		1
//#define CHECK_FPU_CONTROL_WORD_SET	1	// x360 only

#if defined( COMPILER_MSVC64 )

	inline void SetupFPUControlWord()
	{
	}

#elif defined ( COMPILER_MSVC32 )

	inline void SetupFPUControlWordForceExceptions()
	{
		// use local to get and store control word
		uint16 tmpCtrlW;
		__asm
		{
			fnclex						/* clear all current exceptions */
			fnstcw word ptr [tmpCtrlW]	/* get current control word */
			and [tmpCtrlW], 0FCC0h		/* Keep infinity control + rounding control */
			or [tmpCtrlW], 0230h		/* set to 53-bit, mask only inexact, underflow */
			fldcw word ptr [tmpCtrlW]	/* put new control word in FPU */
		}
	}

	#ifdef CHECK_FLOAT_EXCEPTIONS

		inline void SetupFPUControlWord()
		{
			SetupFPUControlWordForceExceptions();
		}

	#else

		inline void SetupFPUControlWord()
		{
			// use local to get and store control word
			uint16 tmpCtrlW;
			__asm
			{
				fnstcw word ptr [tmpCtrlW]	/* get current control word */
				and [tmpCtrlW], 0FCC0h		/* Keep infinity control + rounding control */
				or [tmpCtrlW], 023Fh		/* set to 53-bit, mask only inexact, underflow */
				fldcw word ptr [tmpCtrlW]	/* put new control word in FPU */
			}
		}

	#endif

#elif defined ( COMPILER_GCC )

	inline void SetupFPUControlWord()
	{
		__volatile unsigned short int __cw;
		__asm __volatile ("fnstcw %0" : "=m" (__cw));
		__cw = __cw & 0x0FCC0;	// keep infinity control, keep rounding mode
		__cw = __cw | 0x023F;	// set 53-bit, no exceptions
		__asm __volatile ("fldcw %0" : : "m" (__cw));
	}

#elif defined( COMPILER_MSVCX360 )

	#ifdef CHECK_FPU_CONTROL_WORD_SET
		FORCEINLINE bool IsFPUControlWordSet()
		{
			float f = 0.996f;
			union
			{
				double flResult;
				int pResult[2];
			};
			flResult = __fctiw( f );
			return ( pResult[1] == 1 );
		}
	#else
		#define IsFPUControlWordSet() true
	#endif

	inline void SetupFPUControlWord()
	{
		// Set round-to-nearest in FPSCR
		// (cannot assemble, must use op-code form)
		__emit( 0xFF80010C );	// mtfsfi  7,0

		// Favour compatibility over speed (make sure the VPU set to Java-compliant mode)
		// NOTE: the VPU *always* uses round-to-nearest
		__vector4  a = { 0.0f, 0.0f, 0.0f, 0.0f };
		a;				//	Avoid compiler warning
		__asm
		{
			mtvscr a;	// Clear the Vector Status & Control Register to zero
		}
	}

#endif // COMPILER_MSVCX360


//-----------------------------------------------------------------------------
// Purpose: Standard functions for handling endian-ness
//-----------------------------------------------------------------------------

//-------------------------------------
// Basic swaps
//-------------------------------------

template <typename T>
inline T WordSwapC( T w )
{
   uint16 temp;

   temp  = ((*((uint16 *)&w) & 0xff00) >> 8);
   temp |= ((*((uint16 *)&w) & 0x00ff) << 8);

   return *((T*)&temp);
}

template <typename T>
inline T DWordSwapC( T dw )
{
   uint32 temp;

   temp  =   *((uint32 *)&dw) 				>> 24;
   temp |= ((*((uint32 *)&dw) & 0x00FF0000) >> 8);
   temp |= ((*((uint32 *)&dw) & 0x0000FF00) << 8);
   temp |= ((*((uint32 *)&dw) & 0x000000FF) << 24);

   return *((T*)&temp);
}

//-------------------------------------
// Fast swaps
//-------------------------------------

#if defined( COMPILER_MSVCX360 )

	#define WordSwap  WordSwap360Intr
	#define DWordSwap DWordSwap360Intr

	template <typename T>
	inline T WordSwap360Intr( T w )
	{
		T output;
		__storeshortbytereverse( w, 0, &output );
		return output;
	}

	template <typename T>
	inline T DWordSwap360Intr( T dw )
	{
		T output;
		__storewordbytereverse( dw, 0, &output );
		return output;
	}

#elif defined( COMPILER_MSVC32 )

	#define WordSwap  WordSwapAsm
	#define DWordSwap DWordSwapAsm

	#pragma warning(push)
	#pragma warning (disable:4035) // no return value

	template <typename T>
	inline T WordSwapAsm( T w )
	{
	   __asm
	   {
		  mov ax, w
		  xchg al, ah
	   }
	}

	template <typename T>
	inline T DWordSwapAsm( T dw )
	{
	   __asm
	   {
		  mov eax, dw
		  bswap eax
	   }
	}

	#pragma warning(pop)

#else

	#define WordSwap  WordSwapC
	#define DWordSwap DWordSwapC

#endif

//-------------------------------------
// The typically used methods.
//-------------------------------------

#if defined( _SGI_SOURCE ) || defined( PLATFORM_X360 )
#define	PLAT_BIG_ENDIAN 1
#else
#define PLAT_LITTLE_ENDIAN 1
#endif


// If a swapped float passes through the fpu, the bytes may get changed.
// Prevent this by swapping floats as DWORDs.
#define SafeSwapFloat( pOut, pIn )	(*((uint*)pOut) = DWordSwap( *((uint*)pIn) ))

#if defined(PLAT_LITTLE_ENDIAN)
#define BigShort( val )				WordSwap( val )
#define BigWord( val )				WordSwap( val )
#define BigLong( val )				DWordSwap( val )
#define BigDWord( val )				DWordSwap( val )
#define LittleShort( val )			( val )
#define LittleWord( val )			( val )
#define LittleLong( val )			( val )
#define LittleDWord( val )			( val )
#define SwapShort( val )			BigShort( val )
#define SwapWord( val )				BigWord( val )
#define SwapLong( val )				BigLong( val )
#define SwapDWord( val )			BigDWord( val )

// Pass floats by pointer for swapping to avoid truncation in the fpu
#define BigFloat( pOut, pIn )		SafeSwapFloat( pOut, pIn )
#define LittleFloat( pOut, pIn )	( *pOut = *pIn )
#define SwapFloat( pOut, pIn )		BigFloat( pOut, pIn )

#elif defined(PLAT_BIG_ENDIAN)

#define BigShort( val )				( val )
#define BigWord( val )				( val )
#define BigLong( val )				( val )
#define BigDWord( val )				( val )
#define LittleShort( val )			WordSwap( val )
#define LittleWord( val )			WordSwap( val )
#define LittleLong( val )			DWordSwap( val )
#define LittleDWord( val )			DWordSwap( val )
#define SwapShort( val )			LittleShort( val )
#define SwapWord( val )				LittleWord( val )
#define SwapLong( val )				LittleLong( val )
#define SwapDWord( val )			LittleDWord( val )

// Pass floats by pointer for swapping to avoid truncation in the fpu
#define BigFloat( pOut, pIn )		( *pOut = *pIn )
#define LittleFloat( pOut, pIn )	SafeSwapFloat( pOut, pIn )
#define SwapFloat( pOut, pIn )		LittleFloat( pOut, pIn )

#else

// @Note (toml 05-02-02): this technique expects the compiler to
// optimize the expression and eliminate the other path. On any new
// platform/compiler this should be tested.
inline short BigShort( short val )		{ int test = 1; return ( *(char *)&test == 1 ) ? WordSwap( val )  : val; }
inline uint16 BigWord( uint16 val )		{ int test = 1; return ( *(char *)&test == 1 ) ? WordSwap( val )  : val; }
inline long BigLong( long val )			{ int test = 1; return ( *(char *)&test == 1 ) ? DWordSwap( val ) : val; }
inline uint32 BigDWord( uint32 val )	{ int test = 1; return ( *(char *)&test == 1 ) ? DWordSwap( val ) : val; }
inline short LittleShort( short val )	{ int test = 1; return ( *(char *)&test == 1 ) ? val : WordSwap( val ); }
inline uint16 LittleWord( uint16 val )	{ int test = 1; return ( *(char *)&test == 1 ) ? val : WordSwap( val ); }
inline long LittleLong( long val )		{ int test = 1; return ( *(char *)&test == 1 ) ? val : DWordSwap( val ); }
inline uint32 LittleDWord( uint32 val )	{ int test = 1; return ( *(char *)&test == 1 ) ? val : DWordSwap( val ); }
inline short SwapShort( short val )					{ return WordSwap( val ); }
inline uint16 SwapWord( uint16 val )				{ return WordSwap( val ); }
inline long SwapLong( long val )					{ return DWordSwap( val ); }
inline uint32 SwapDWord( uint32 val )				{ return DWordSwap( val ); }

// Pass floats by pointer for swapping to avoid truncation in the fpu
inline void BigFloat( float *pOut, const float *pIn )		{ int test = 1; ( *(char *)&test == 1 ) ? SafeSwapFloat( pOut, pIn ) : ( *pOut = *pIn ); }
inline void LittleFloat( float *pOut, const float *pIn )	{ int test = 1; ( *(char *)&test == 1 ) ? ( *pOut = *pIn ) : SafeSwapFloat( pOut, pIn ); }
inline void SwapFloat( float *pOut, const float *pIn )		{ SafeSwapFloat( pOut, pIn ); }

#endif

#if _X360
FORCEINLINE unsigned long LoadLittleDWord( const unsigned long *base, unsigned int dwordIndex )
		{
			return __loadwordbytereverse( dwordIndex<<2, base );
		}

FORCEINLINE void StoreLittleDWord( unsigned long *base, unsigned int dwordIndex, unsigned long dword )
		{
			__storewordbytereverse( dword, dwordIndex<<2, base );
		}
#else
FORCEINLINE unsigned long LoadLittleDWord( const unsigned long *base, unsigned int dwordIndex )
	{
		return LittleDWord( base[dwordIndex] );
	}

FORCEINLINE void StoreLittleDWord( unsigned long *base, unsigned int dwordIndex, unsigned long dword )
	{
		base[dwordIndex] = LittleDWord(dword);
	}
#endif


// When in benchmark mode, the timer returns a simple incremented value each time you call it.
//
// It should not be changed after startup unless you really know what you're doing. The only place
// that should do this is the benchmark code itself so it can output a legit duration.
PLATFORM_INTERFACE void				Plat_SetBenchmarkMode( bool bBenchmarkMode );	
PLATFORM_INTERFACE bool				Plat_IsInBenchmarkMode();

// Returns true if running in test mode.
// Test mode is turned on if -testmode is on the command line or the VALVE_TESTMODE environment variable is defined.
PLATFORM_INTERFACE bool				Plat_IsInTestMode();

PLATFORM_INTERFACE double			Plat_FloatTime();		// Returns time in seconds since the module was loaded.
PLATFORM_INTERFACE uint32			Plat_MSTime();			// Time in milliseconds.
PLATFORM_INTERFACE uint64			Plat_GetClockStart();	// Snapshot of the clock when app started.

// Get the local calendar time.
// Same as time() followed by localtime(), but non-crash-prone and threadsafe.
PLATFORM_INTERFACE void				Plat_GetLocalTime( struct tm *pNow );

// Convert a time_t (specified in nTime - seconds since Jan 1, 1970 UTC) to a local calendar time in a threadsafe and non-crash-prone way.
PLATFORM_INTERFACE void				Plat_ConvertToLocalTime( uint64 nTime, struct tm *pNow );

// Get a time string (same as ascstring, but threadsafe).
PLATFORM_INTERFACE void				Plat_GetTimeString( struct tm *pTime, char *pOut, int nMaxBytes );

// converts a time_t to a struct tm without the local time conversion of ConvertToLocalTime
PLATFORM_INTERFACE void Plat_gmtime( uint64 nTime, struct tm *pTime );


// Get the process' executable filename.
PLATFORM_INTERFACE void				Plat_GetModuleFilename( char *pOut, int nMaxBytes );

PLATFORM_INTERFACE void				Plat_ExitProcess( int nCode );

PLATFORM_INTERFACE bool				Plat_ShouldCollectMiniDumpsForFatalErrors();

PLATFORM_INTERFACE void				Plat_FatalErrorFunc( const tchar* pMsg, ... ) FMTFUNCTION( 1, 2 );

// b/w compatibility
#define Sys_FloatTime Plat_FloatTime

// Protect against bad auto operator=
#define DISALLOW_OPERATOR_EQUAL( _classname )			\
	private:											\
		_classname &operator=( const _classname & );	\
	public:

// Define a reasonable operator=
#define IMPLEMENT_OPERATOR_EQUAL( _classname )			\
	public:												\
		_classname &operator=( const _classname &src )	\
		{												\
			memcpy( this, &src, sizeof(_classname) );	\
			return *this;								\
		}
	
// Processor Information:
struct CPUInformation
{
	int	 m_Size;		// Size of this structure, for forward compatability.

	bool m_bRDTSC : 1,	// Is RDTSC supported?
		 m_bCMOV  : 1,  // Is CMOV supported?
		 m_bFCMOV : 1,  // Is FCMOV supported?
		 m_bSSE	  : 1,	// Is SSE supported?
		 m_bSSE2  : 1,	// Is SSE2 Supported?
		 m_b3DNow : 1,	// Is 3DNow! Supported?
		 m_bMMX   : 1,	// Is MMX supported?
		 m_bHT	  : 1;	// Is HyperThreading supported?

	uint8 m_nLogicalProcessors;		// Number op logical processors.
	uint8 m_nPhysicalProcessors;	// Number of physical processors
	
	bool m_bSSE3 : 1,
		 m_bSSSE3 : 1,
		 m_bSSE4a : 1,
		 m_bSSE41 : 1,
		 m_bSSE42 : 1;	

	int64 m_Speed;						// In cycles per second.

	tchar* m_szProcessorID;				// Processor vendor Identification.

	CPUInformation(): m_Size(0){}
};

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma clang diagnostic ignored "-Wreturn-type-c-linkage"
#endif

PLATFORM_INTERFACE const CPUInformation& GetCPUInformation();

#ifdef __clang__
#pragma clang diagnostic pop
#endif


PLATFORM_INTERFACE void GetCurrentDate( int *pDay, int *pMonth, int *pYear );
PLATFORM_INTERFACE void GetCurrentDayOfTheWeek( int *pDay );  // 0 = Sunday
PLATFORM_INTERFACE void GetCurrentDayOfTheYear( int *pDay );  // 0 = Jan 1

// ---------------------------------------------------------------------------------- //
// Performance Monitoring Events - L2 stats etc...
// ---------------------------------------------------------------------------------- //
PLATFORM_INTERFACE void InitPME();
PLATFORM_INTERFACE void ShutdownPME();


//-----------------------------------------------------------------------------
// Security related functions
//-----------------------------------------------------------------------------
// Ensure that the hardware key's drivers have been installed.
PLATFORM_INTERFACE bool Plat_VerifyHardwareKeyDriver();

// Ok, so this isn't a very secure way to verify the hardware key for now.  It
// is primarially depending on the fact that all the binaries have been wrapped
// with the secure wrapper provided by the hardware keys vendor.
PLATFORM_INTERFACE bool Plat_VerifyHardwareKey();

// The same as above, but notifies user with a message box when the key isn't in
// and gives him an opportunity to correct the situation.
PLATFORM_INTERFACE bool Plat_VerifyHardwareKeyPrompt();

// Can be called in real time, doesn't perform the verify every frame.  Mainly just
// here to allow the game to drop out quickly when the key is removed, rather than
// allowing the wrapper to pop up it's own blocking dialog, which the engine doesn't
// like much.
PLATFORM_INTERFACE bool Plat_FastVerifyHardwareKey();

//-----------------------------------------------------------------------------
// Just logs file and line to simple.log
//-----------------------------------------------------------------------------
PLATFORM_INTERFACE void* Plat_SimpleLog( const tchar* file, int line );

PLATFORM_INTERFACE const char *Plat_GetGameDirectory( int unknown=0 );


#if _X360
#define Plat_FastMemset XMemSet
#define Plat_FastMemcpy XMemCpy
#else
#define Plat_FastMemset memset
#define Plat_FastMemcpy memcpy
#endif

//-----------------------------------------------------------------------------
// Returns true if running on a 64 bit (windows) OS
//-----------------------------------------------------------------------------
PLATFORM_INTERFACE bool Is64BitOS();


//-----------------------------------------------------------------------------
// XBOX Components valid in PC compilation space
//-----------------------------------------------------------------------------

#define XBOX_DVD_SECTORSIZE			2048
#define XBOX_DVD_ECC_SIZE			32768 // driver reads in quantum ECC blocks
#define XBOX_HDD_SECTORSIZE			512

// Custom windows messages for Xbox input
#define WM_XREMOTECOMMAND					(WM_USER + 100)
#define WM_XCONTROLLER_KEY					(WM_USER + 101)
#define WM_SYS_UI							(WM_USER + 102)
#define WM_SYS_SIGNINCHANGED				(WM_USER + 103)
#define WM_SYS_STORAGEDEVICESCHANGED		(WM_USER + 104)
#define WM_SYS_PROFILESETTINGCHANGED		(WM_USER + 105)
#define WM_SYS_MUTELISTCHANGED				(WM_USER + 106)
#define WM_SYS_INPUTDEVICESCHANGED			(WM_USER + 107)
#define WM_SYS_INPUTDEVICECONFIGCHANGED		(WM_USER + 108)
#define WM_LIVE_CONNECTIONCHANGED			(WM_USER + 109)
#define WM_LIVE_INVITE_ACCEPTED				(WM_USER + 110)
#define WM_LIVE_LINK_STATE_CHANGED			(WM_USER + 111)
#define WM_LIVE_CONTENT_INSTALLED			(WM_USER + 112)
#define WM_LIVE_MEMBERSHIP_PURCHASED		(WM_USER + 113)
#define WM_LIVE_VOICECHAT_AWAY				(WM_USER + 114)
#define WM_LIVE_PRESENCE_CHANGED			(WM_USER + 115)
#define WM_FRIENDS_PRESENCE_CHANGED			(WM_USER + 116)
#define WM_FRIENDS_FRIEND_ADDED				(WM_USER + 117)
#define WM_FRIENDS_FRIEND_REMOVED			(WM_USER + 118)
#define WM_CUSTOM_GAMEBANNERPRESSED			(WM_USER + 119)
#define WM_CUSTOM_ACTIONPRESSED				(WM_USER + 120)
#define WM_XMP_STATECHANGED					(WM_USER + 121)
#define WM_XMP_PLAYBACKBEHAVIORCHANGED		(WM_USER + 122)
#define WM_XMP_PLAYBACKCONTROLLERCHANGED	(WM_USER + 123)

inline const char *GetPlatformExt( void )
{
	return IsPlatformX360() ? ".360" : "";
}

// flat view, 6 hw threads
#define XBOX_PROCESSOR_0			( 1<<0 )
#define XBOX_PROCESSOR_1			( 1<<1 )
#define XBOX_PROCESSOR_2			( 1<<2 )
#define XBOX_PROCESSOR_3			( 1<<3 )
#define XBOX_PROCESSOR_4			( 1<<4 )
#define XBOX_PROCESSOR_5			( 1<<5 )

// core view, 3 cores with 2 hw threads each
#define XBOX_CORE_0_HWTHREAD_0		XBOX_PROCESSOR_0
#define XBOX_CORE_0_HWTHREAD_1		XBOX_PROCESSOR_1
#define XBOX_CORE_1_HWTHREAD_0		XBOX_PROCESSOR_2
#define XBOX_CORE_1_HWTHREAD_1		XBOX_PROCESSOR_3
#define XBOX_CORE_2_HWTHREAD_0		XBOX_PROCESSOR_4
#define XBOX_CORE_2_HWTHREAD_1		XBOX_PROCESSOR_5

//-----------------------------------------------------------------------------
// Include additional dependant header components.
//-----------------------------------------------------------------------------
#if defined( PLATFORM_X360 )
#include "xbox/xbox_core.h"
#endif

//-----------------------------------------------------------------------------
// Methods to invoke the constructor, copy constructor, and destructor
//-----------------------------------------------------------------------------

template <class T>
inline T* Construct( T* pMemory )
{
	return ::new( pMemory ) T;
}

template <class T, typename ARG1>
inline T* Construct( T* pMemory, ARG1 a1 )
{
	return ::new( pMemory ) T( a1 );
}

template <class T, typename ARG1, typename ARG2>
inline T* Construct( T* pMemory, ARG1 a1, ARG2 a2 )
{
	return ::new( pMemory ) T( a1, a2 );
}

template <class T, typename ARG1, typename ARG2, typename ARG3>
inline T* Construct( T* pMemory, ARG1 a1, ARG2 a2, ARG3 a3 )
{
	return ::new( pMemory ) T( a1, a2, a3 );
}

template <class T, typename ARG1, typename ARG2, typename ARG3, typename ARG4>
inline T* Construct( T* pMemory, ARG1 a1, ARG2 a2, ARG3 a3, ARG4 a4 )
{
	return ::new( pMemory ) T( a1, a2, a3, a4 );
}

template <class T, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>
inline T* Construct( T* pMemory, ARG1 a1, ARG2 a2, ARG3 a3, ARG4 a4, ARG5 a5 )
{
	return ::new( pMemory ) T( a1, a2, a3, a4, a5 );
}

template <class T, class P>
inline void ConstructOneArg( T* pMemory, P const& arg)
{
	::new( pMemory ) T(arg);
}

template <class T, class P1, class P2 >
inline void ConstructTwoArg( T* pMemory, P1 const& arg1, P2 const& arg2)
{
	::new( pMemory ) T(arg1, arg2);
}

template <class T, class P1, class P2, class P3 >
inline void ConstructThreeArg( T* pMemory, P1 const& arg1, P2 const& arg2, P3 const& arg3)
{
	::new( pMemory ) T(arg1, arg2, arg3);
}

template <class T>
inline T* CopyConstruct( T* pMemory, T const& src )
{
	return ::new( pMemory ) T(src);
}

template <class T>
inline void Destruct( T* pMemory )
{
	pMemory->~T();

#ifdef _DEBUG
	memset( (void*)pMemory, 0xDD, sizeof(T) );
#endif
}


//
// GET_OUTER()
//
// A platform-independent way for a contained class to get a pointer to its
// owner. If you know a class is exclusively used in the context of some
// "outer" class, this is a much more space efficient way to get at the outer
// class than having the inner class store a pointer to it.
//
//	class COuter
//	{
//		class CInner // Note: this does not need to be a nested class to work
//		{
//			void PrintAddressOfOuter()
//			{
//				printf( "Outer is at 0x%x\n", GET_OUTER( COuter, m_Inner ) );
//			}
//		};
//
//		CInner m_Inner;
//		friend class CInner;
//	};

#define GET_OUTER( OuterType, OuterMember ) \
   ( ( OuterType * ) ( (uint8 *)this - offsetof( OuterType, OuterMember ) ) )


/*	TEMPLATE_FUNCTION_TABLE()

    (Note added to platform.h so platforms that correctly support templated
	 functions can handle portions as templated functions rather than wrapped
	 functions)

	Helps automate the process of creating an array of function
	templates that are all specialized by a single integer.
	This sort of thing is often useful in optimization work.

	For example, using TEMPLATE_FUNCTION_TABLE, this:

	TEMPLATE_FUNCTION_TABLE(int, Function, ( int blah, int blah ), 10)
	{
		return argument * argument;
	}

	is equivilent to the following:

	(NOTE: the function has to be wrapped in a class due to code
	generation bugs involved with directly specializing a function
	based on a constant.)

	template<int argument>
	class FunctionWrapper
	{
	public:
		int Function( int blah, int blah )
		{
			return argument*argument;
		}
	}

	typedef int (*FunctionType)( int blah, int blah );

	class FunctionName
	{
	public:
		enum { count = 10 };
		FunctionType functions[10];
	};

	FunctionType FunctionName::functions[] =
	{
		FunctionWrapper<0>::Function,
		FunctionWrapper<1>::Function,
		FunctionWrapper<2>::Function,
		FunctionWrapper<3>::Function,
		FunctionWrapper<4>::Function,
		FunctionWrapper<5>::Function,
		FunctionWrapper<6>::Function,
		FunctionWrapper<7>::Function,
		FunctionWrapper<8>::Function,
		FunctionWrapper<9>::Function
	};
*/

PLATFORM_INTERFACE bool vtune( bool resume );


#define TEMPLATE_FUNCTION_TABLE(RETURN_TYPE, NAME, ARGS, COUNT)			\
																		\
typedef RETURN_TYPE (FASTCALL *__Type_##NAME) ARGS;						\
																		\
template<const int nArgument>											\
struct __Function_##NAME												\
{																		\
	static RETURN_TYPE FASTCALL Run ARGS;								\
};																		\
																		\
template <const int i>														\
struct __MetaLooper_##NAME : __MetaLooper_##NAME<i-1>					\
{																		\
	__Type_##NAME func;													\
	inline __MetaLooper_##NAME() { func = __Function_##NAME<i>::Run; }	\
};																		\
																		\
template<>																\
struct __MetaLooper_##NAME<0>											\
{																		\
	__Type_##NAME func;													\
	inline __MetaLooper_##NAME() { func = __Function_##NAME<0>::Run; }	\
};																		\
																		\
class NAME																\
{																		\
private:																\
    static const __MetaLooper_##NAME<COUNT> m;							\
public:																	\
	enum { count = COUNT };												\
	static const __Type_##NAME* functions;								\
};																		\
const __MetaLooper_##NAME<COUNT> NAME::m;								\
const __Type_##NAME* NAME::functions = (__Type_##NAME*)&m;				\
template<const int nArgument>													\
RETURN_TYPE FASTCALL __Function_##NAME<nArgument>::Run ARGS


#define LOOP_INTERCHANGE(BOOLEAN, CODE)\
	if( (BOOLEAN) )\
	{\
		CODE;\
	} else\
	{\
		CODE;\
	}


//-----------------------------------------------------------------------------
// Dynamic libs support
//-----------------------------------------------------------------------------
#if defined( PLATFORM_WINDOWS )

PLATFORM_INTERFACE void *Plat_GetProcAddress( const char *pszModule, const char *pszName );

template <typename FUNCPTR_TYPE>
class CDynamicFunction
{
public:
	CDynamicFunction( const char *pszModule, const char *pszName, FUNCPTR_TYPE pfnFallback = NULL )
	{
		m_pfn = pfnFallback;
		void *pAddr = Plat_GetProcAddress( pszModule, pszName );
		if ( pAddr )
		{
			m_pfn = (FUNCPTR_TYPE)pAddr;
		}
	}

	operator bool()			{ return m_pfn != NULL;	}
	bool operator !()		{ return !m_pfn;	}
	operator FUNCPTR_TYPE()	{ return m_pfn; }

private:
	FUNCPTR_TYPE m_pfn;
};
#endif


//-----------------------------------------------------------------------------
// What OS version are we?
//-----------------------------------------------------------------------------
enum PlatOSVersion_t
{
	PLAT_OS_VERSION_UNKNOWN = -1,

	// X360-specific versions
	PLAT_OS_VERSION_XBOX360 = 0,

	// PC-specific OS versions
	PLAT_OS_VERSION_XP = 5,
	PLAT_OS_VERSION_VISTA = 6,
};

PLATFORM_INTERFACE PlatOSVersion_t Plat_GetOSVersion();


//  Watchdog timer support. Call BeginWatchdogTimer( nn ) to kick the timer off.  if you don't call
// EndWatchdogTimer within nn seconds, the program will kick off an exception.  This is for making
// sure that hung dedicated servers abort (and restart) instead of staying hung. Calling
// EndWatchdogTimer more than once or when there is no active watchdog is fine. Only does anything
// under linux right now. It should be possible to implement this functionality in windows via a
// thread, if desired.

#ifdef _POSIX

PLATFORM_INTERFACE void BeginWatchdogTimer( int nSecs );
PLATFORM_INTERFACE void EndWatchdogTimer( void );
PLATFORM_INTERFACE void ResetBaseTime( void );							  // reset plat_floattime to 0 for a subprocess
#else
FORCEINLINE void BeginWatchdogTimer( int nSecs )
{
}

FORCEINLINE void EndWatchdogTimer( void )
{
}
FORCEINLINE void ResetBaseTime( void )						  // reset plat_floattime to 0 for a subprocess
{
}

#endif


#ifdef COMPILER_MSVC
/*
FORCEINLINE uint8 RotateBitsLeft8( uint8 nValue, int nRotateBits )
{
	return _rotl8( nValue, nRotateBits );
}
FORCEINLINE uint16 RotateBitsLeft16( uint16 nValue, int nRotateBits )
{
	return _rotl( nValue, nRotateBits );
}
FORCEINLINE uint8 RotateBitsRight8( uint8 nValue, int nRotateBits )
{
return _rotr8( nValue, nRotateBits );
}
FORCEINLINE uint16 RotateBitsRight16( uint16 nValue, int nRotateBits )
{
return _rotr16( nValue, nRotateBits );
}
*/
FORCEINLINE uint32 RotateBitsLeft32( uint32 nValue, int nRotateBits )
{
	return _rotl( nValue, nRotateBits );
}
FORCEINLINE uint64 RotateBitsLeft64( uint64 nValue, int nRotateBits )
{
	return _rotl64( nValue, nRotateBits );
}
FORCEINLINE uint32 RotateBitsRight32( uint32 nValue, int nRotateBits )
{
	return _rotr( nValue, nRotateBits );
}
FORCEINLINE uint64 RotateBitsRight64( uint64 nValue, int nRotateBits )
{
	return _rotr64( nValue, nRotateBits );
}
#else
// GCC should compile this all into single instruction
/*
FORCEINLINE uint8 RotateBitsLeft8( uint8 nValue, int nRotateBits )
{
	return ( nValue << nRotateBits ) | ( nValue >> ( ( -nRotateBits ) & 7 ) );
}
FORCEINLINE uint16 RotateBitsLeft16( uint16 nValue, int nRotateBits )
{
	return ( nValue << nRotateBits ) | ( nValue >> ( ( -nRotateBits ) & 15 ) );
}
FORCEINLINE uint8 RotateBitsRight8( uint8 nValue, int nRotateBits )
{
	return ( nValue >> nRotateBits ) | ( nValue << ( ( -nRotateBits ) & 7 ) );
}
FORCEINLINE uint16 RotateBitsRight16( uint16 nValue, int nRotateBits )
{
	return ( nValue >> nRotateBits ) | ( nValue << ( ( -nRotateBits ) & 15 ) );
}
*/
FORCEINLINE uint32 RotateBitsLeft32( uint32 nValue, int nRotateBits )
{
	return ( nValue << nRotateBits ) | ( nValue >> ( ( -nRotateBits ) & 31 ) );
}
FORCEINLINE uint64 RotateBitsLeft64( uint64 nValue, int nRotateBits )
{
	return ( nValue << nRotateBits ) | ( nValue >> ( ( - nRotateBits ) & 63 ) );
}
FORCEINLINE uint32 RotateBitsRight32( uint32 nValue, int nRotateBits )
{
	return ( nValue >> nRotateBits ) | ( nValue << ( ( -nRotateBits ) & 31 ) );
}
FORCEINLINE uint64 RotateBitsRight64( uint64 nValue, int nRotateBits )
{
	return ( nValue >> nRotateBits ) | ( nValue << ( ( - nRotateBits ) & 63 ) );
}
#endif

#include "tier0/valve_on.h"

#if defined(TIER0_DLL_EXPORT)
extern int V_tier0_stricmp(const char *s1, const char *s2 );
#define stricmp(s1,s2) V_tier0_stricmp( s1, s2 )
#define strcmpi(s1,s2) V_tier0_stricmp( s1, s2 )
#else
int	_V_stricmp	  (const char *s1, const char *s2 );
int	V_strncasecmp (const char *s1, const char *s2, int n);

// A special high-performance case-insensitive compare function that in
// a single call distinguishes between exactly matching strings,
// strings equal in case-insensitive way, and not equal strings:
//   returns 0 if strings match exactly
//   returns >0 if strings match in a case-insensitive way, but do not match exactly
//   returns <0 if strings do not match even in a case-insensitive way
int	_V_stricmp_NegativeForUnequal	  ( const char *s1, const char *s2 );

#ifndef stricmp
#define stricmp(s1,s2) _V_stricmp(s1, s2)
#endif
#ifndef strcmpi
#define strcmpi(s1,s2) _V_stricmp(s1, s2)
#endif
#ifndef strnicmp
#define strnicmp V_strncasecmp
#endif
#endif

#endif /* PLATFORM_H */
