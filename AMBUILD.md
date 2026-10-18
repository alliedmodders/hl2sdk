# AMBuild: mathlib, tier1, and interfaces

AMBuild builds the `mathlib`, `tier1`, and `interfaces` static libraries for
the native Windows or Linux x64 platform. Install AMBuild 2, then configure
and build:

```sh
python configure.py
ambuild
```

On Windows, run these commands from a Visual Studio developer shell. On Linux,
use a GCC or Clang toolchain. The archives are written under the AMBuild build
directory in `lib/public/win64` (`mathlib.lib`, `tier1.lib`, `interfaces.lib`)
or `lib/linux64` (`mathlib.a`, `tier1.a`, `interfaces.a`). Add `--enable-debug`
when configuring for debug symbols; `--enable-optimize` enables release
optimization flags.
`--targets` accepts a comma-separated architecture list; this SDK currently
accepts `--targets x86_64` only.

Outbreak's legacy logging wrappers (`Msg`, `Warning`, `Error`, console and
developer variants, and pointer assertion helpers) are compiled into `tier1`
from `tier1/dbg.cpp`. Rebuild `tier1`, `mathlib`, `interfaces`, and their consumers
after updating the headers: existing objects referencing `__imp_Msg`,
`__imp_Warning`, etc. must be recompiled to call the local functions.
The wrappers forward to the DLL's `LoggingSystem_LogDirect`, preserving channel
filtering, colors, and response policies. `Error` always exits with code 100.
Callstack variants log normally but do not append a stack trace. Pointer helpers
check for null in debug builds and are empty in release builds.

Channel registration now takes a sixth argument, a non-null module name.
Define `LOGGING_MODULE_NAME` when compiling tier1 or defining channels to
override the default `"HL2SDK"`. For example, an AMBuild consumer compiling
its own tier1 can add this to that target's compiler defines:

```python
# Metamod:Source's tier1 target:
binary.compiler.defines += ['LOGGING_MODULE_NAME="Metamod:Source"']

# SourceMod's tier1 target:
binary.compiler.defines += ['LOGGING_MODULE_NAME="SourceMod"']
```

The define must be passed when compiling `tier1/dbg.cpp`; defining it only in
consumer files does not rename wrappers in a previously built tier1 archive.
Apply it consistently to the consumer's other source files as well if they
register custom channels through the SDK macros. Existing `Msg`, `ConMsg`,
`Warning`, etc. calls then use the chosen source name automatically.

When building tier1 from source, link that archive instead of the SDK's
prebuilt `lib/public/win64/tier1.lib`. Linking both can select the prebuilt
`dbg.obj` with its default source name even when the rebuilt object contains
the override. `SdkHelpers.ambuild` should omit prebuilt tier1 and mathlib entries
when it has built replacements for the target architecture.

The Console channel initially allows ordinary
messages so `ConMsg` and `ConColorMsg` work in modules loaded after engine
initialization. Developer channels initially allow only errors; the engine
controls their levels using the usual channel tags.

The Windows x64 ABI changes were inferred from DLL exports, strings, and
`dumpbin /NOPDB` disassembly, without using Outbreak PDB information:
`LoggingSystem_RegisterLoggingChannel` at RVA `0xB360` passes the sixth argument
as a string; its initializer at RVA `0x1710` passes `"General"` and `"tier0"`.
`LoggingSystem_LogDirect` at RVA `0xB1A0` uses the low byte of severity and returns
a byte response. Its implementation at RVA `0xA450` constructs a context with
channel at offset 0, byte flags at 4, byte severity at 5, color at 6, and a module
string pointer at 16. Linux binary compatibility has not been established.
