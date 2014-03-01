//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose:
//
//===========================================================================//

#if defined(WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#else
#include <dlfcn.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>
#endif

// Reverse engineered file

// Entry point of dedicated binary
#if defined(WIN32)
typedef int (*DedicatedMain_t)(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
#else
typedef int (*DedicatedMain_t)(int argc, char **argv);
#endif

#if defined(WIN32)
#define ENGINE_BIN(x) 		"bin\\" #x ".dll"
#elif defined(LINUX)
#define ENGINE_BIN(x) 		"bin/" #x "_srv.so"
#define ENGINE_BIN_ALT(x)	"bin/" #x ".so"
#define LIB_PATH 		"LD_LIBRARY_PATH"
#elif defined(OSX)
#define ENGINE_BIN(x) 		"bin/" #x ".dylib"
#define LIB_PATH		"DYLD_LIBRARY_PATH"
#endif

#if defined(WANTS_DEBUGSTR_HACK) && !defined(WIN32)

#if !defined(PAGE_SIZE)
#define PAGE_SIZE 4096
#endif

#define ALIGN(ar) ((long)ar & ~(PAGE_SIZE-1))

void BlockDebugString()
{
	void *tier0;
	void *func;
	void *addr;
	int ret;
	const uint8_t OP_RET = 0xC3;

	tier0 = dlopen(ENGINE_BIN(libtier0), RTLD_NOW);

#if defined(ENGINE_BIN_ALT)
	if (!tier0)
		tier0 = dlopen(ENGINE_BIN_ALT(libtier0), RTLD_NOW);
#endif

	if (!tier0)
		return;

	func = dlsym(tier0, "Plat_DebugString");
	dlclose(tier0);
	if (!func)
		return;

	addr = (void *)ALIGN(func);
	ret = mprotect(addr, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE|PROT_EXEC);
	if (ret != 0)
		return;

	*(uint8_t *)func = OP_RET;

	mprotect(addr, sysconf(_SC_PAGESIZE), PROT_READ|PROT_EXEC);
}
#endif

#if defined(WIN32)
static const char *GetBaseDir(const char *file)
{
	static char basedir[MAX_PATH];

	strcpy(basedir, file);

	char *rslash = strrchr(basedir, '\\');

	if (rslash)
		*(rslash + 1) = '\0';

	return basedir;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	char exeName[MAX_PATH];
	char pathBuf[4096];

	char *path = getenv("PATH");

	if (!GetModuleFileName(hInstance, exeName, sizeof(exeName)))
	{
		MessageBox(0, "Failed calling GetModuleFileName", "Launcher Error", MB_OK);
		return 0;
	}

	const char *root = GetBaseDir(exeName);

	_snprintf(pathBuf, sizeof(pathBuf) - 1, "PATH=%s\\bin\\;%s", root, path);
	_putenv(pathBuf);

	HMODULE dedicated = LoadLibrary(ENGINE_BIN(dedicated));

	if (!dedicated)
	{
		char *error;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&error,
			0,
			NULL);

		char msg[1024];
		_snprintf(msg, sizeof(msg) - 1,  "Failed to load the launcher DLL:\n\n%s", error);
		MessageBox(0, msg, "Launcher Error", MB_OK);

		LocalFree(error);
		return 0;
	}

	DedicatedMain_t main = (DedicatedMain_t)GetProcAddress(dedicated, "DedicatedMain");

	int ret = main(hInstance, hPrevInstance, lpCmdLine, nCmdShow);

	FreeLibrary(dedicated);

	return ret;
}

#else

int main(int argc, char **argv)
{
	char cwd[PATH_MAX];
	char pathBuf[4096];

	char *path = getenv(LIB_PATH);

	if (!getcwd(cwd, sizeof(cwd)))
	{
		printf("getcwd failed (%s)\n", strerror(errno));
		return -1;
	}

	snprintf(pathBuf, sizeof(pathBuf), LIB_PATH "=%s/bin:%s", cwd, path);
	if (putenv(pathBuf) != 0)
	{
		printf("%s\n", strerror(errno));
		return -1;
	}

	void *dedicated = dlopen(ENGINE_BIN(dedicated), RTLD_NOW);

#if defined(ENGINE_BIN_ALT)
	if (!dedicated)
		dedicated = dlopen(ENGINE_BIN_ALT(dedicated), RTLD_NOW);
#endif

	if (!dedicated)
	{
		printf("Failed to open %s (%s)\n", ENGINE_BIN(dedicated), dlerror());
		return -1;
	}

#if defined(WANTS_DEBUGSTR_HACK)
	BlockDebugString();
#endif

	DedicatedMain_t main = (DedicatedMain_t)dlsym(dedicated, "DedicatedMain");
	if (!main)
	{
		printf("Failed to find dedicated server entry point (%s)\n", dlerror());
		return -1;
	}

	int ret = main(argc, argv);

	dlclose(dedicated);

	return ret;
}
#endif
