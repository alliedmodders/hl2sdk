// AMNOTE: Legacy logging wrappers for modules using Outbreak's tier0 logging system.
#include "tier0/dbg.h"

DEFINE_LOGGING_CHANNEL_NO_TAGS( LOG_GENERAL, "General" );
DEFINE_LOGGING_CHANNEL_NO_TAGS( LOG_ASSERT, "Assert", 0, LS_ASSERT, Color( 255, 0, 0, 255 ) );

BEGIN_DEFINE_LOGGING_CHANNEL( LOG_CONSOLE, "Console", LCF_CONSOLE_ONLY, LS_MESSAGE );
ADD_LOGGING_CHANNEL_TAG( "Console" );
END_DEFINE_LOGGING_CHANNEL();

BEGIN_DEFINE_LOGGING_CHANNEL( LOG_DEVELOPER, "Developer", LCF_CONSOLE_ONLY, LS_ERROR );
ADD_LOGGING_CHANNEL_TAG( "Developer" );
END_DEFINE_LOGGING_CHANNEL();

BEGIN_DEFINE_LOGGING_CHANNEL( LOG_DEVELOPER_CONSOLE, "DeveloperConsole", LCF_CONSOLE_ONLY, LS_ERROR );
ADD_LOGGING_CHANNEL_TAG( "DeveloperVerbose" );
ADD_LOGGING_CHANNEL_TAG( "Console" );
END_DEFINE_LOGGING_CHANNEL();

BEGIN_DEFINE_LOGGING_CHANNEL( LOG_DEVELOPER_VERBOSE, "DeveloperVerbose", LCF_CONSOLE_ONLY, LS_ERROR, Color( 192, 128, 192, 255 ) );
ADD_LOGGING_CHANNEL_TAG( "DeveloperVerbose" );
END_DEFINE_LOGGING_CHANNEL();

static LoggingResponse_t LogLegacy( LoggingChannelID_t channel, LoggingSeverity_t severity,
	const Color &color, const tchar *format, va_list args )
{
	if ( !LoggingSystem_IsChannelEnabled( channel, severity ) )
		return LR_CONTINUE;

	tchar message[MAX_LOGGING_MESSAGE_LENGTH];
	Tier0Internal_vsntprintf( message, sizeof( message ) / sizeof( message[0] ), format, args );
	message[MAX_LOGGING_MESSAGE_LENGTH - 1] = 0;
	return LoggingSystem_LogDirect( channel, severity, color, message );
}

// Only the wrappers without overloads have C linkage (inherited from dbg.h).
#define DEFINE_LEGACY_LOG( Name, Channel, Severity ) \
	void Name( const tchar *format, ... ) \
	{ \
		va_list args; \
		va_start( args, format ); \
		LogLegacy( Channel, Severity, UNSPECIFIED_LOGGING_COLOR, format, args ); \
		va_end( args ); \
	}

DEFINE_LEGACY_LOG( Msg, LOG_GENERAL, LS_MESSAGE )
DEFINE_LEGACY_LOG( Warning, LOG_GENERAL, LS_WARNING )
DEFINE_LEGACY_LOG( DevMsg, LOG_DEVELOPER, LS_MESSAGE )
DEFINE_LEGACY_LOG( DevWarning, LOG_DEVELOPER, LS_WARNING )
DEFINE_LEGACY_LOG( ConMsg, LOG_CONSOLE, LS_MESSAGE )
DEFINE_LEGACY_LOG( ConDMsg, LOG_DEVELOPER_CONSOLE, LS_MESSAGE )

#undef DEFINE_LEGACY_LOG

void ConColorMsg( const Color &color, const tchar *format, ... )
{
	va_list args;
	va_start( args, format );
	LogLegacy( LOG_CONSOLE, LS_MESSAGE, color, format, args );
	va_end( args );
}

void DevMsg( int level, const tchar *format, ... )
{
	va_list args;
	va_start( args, format );
	LogLegacy( level < 2 ? LOG_DEVELOPER : LOG_DEVELOPER_VERBOSE,
		LS_MESSAGE, UNSPECIFIED_LOGGING_COLOR, format, args );
	va_end( args );
}

void DevWarning( int level, const tchar *format, ... )
{
	va_list args;
	va_start( args, format );
	LogLegacy( level < 2 ? LOG_DEVELOPER : LOG_DEVELOPER_VERBOSE,
		LS_WARNING, UNSPECIFIED_LOGGING_COLOR, format, args );
	va_end( args );
}

void Error( const tchar *format, ... )
{
	va_list args;
	va_start( args, format );
	LogLegacy( LOG_GENERAL, LS_ERROR, UNSPECIFIED_LOGGING_COLOR, format, args );
	va_end( args );
	// CS:GO's Error exits even when the channel is disabled or the policy returns continue.
	Plat_ExitProcess( 100 );
}

void Warning_SpewCallStack( int /*maxCallStackLength*/, const tchar *format, ... )
{
	va_list args;
	va_start( args, format );
	LogLegacy( LOG_GENERAL, LS_WARNING, UNSPECIFIED_LOGGING_COLOR, format, args );
	va_end( args );
}

void Error_SpewCallStack( int /*maxCallStackLength*/, const tchar *format, ... )
{
	va_list args;
	va_start( args, format );
	LogLegacy( LOG_GENERAL, LS_ERROR, UNSPECIFIED_LOGGING_COLOR, format, args );
	va_end( args );
	Plat_ExitProcess( 100 );
}

LoggingResponse_t LoggingSystem_LogAssert( const char *format, ... )
{
	va_list args;
	va_start( args, format );
	LoggingResponse_t response = LogLegacy( LOG_ASSERT, LS_ASSERT,
		UNSPECIFIED_LOGGING_COLOR, format, args );
	va_end( args );
	return response;
}

void _ExitOnFatalAssert( const tchar *file, int line )
{
	Log_Msg( LOG_ASSERT, "Fatal assert failed: %s, line %d. Application exiting.\n", file, line );
	Plat_ExitProcess( EXIT_FAILURE );
}

void _AssertValidReadPtr( void *ptr, int count )
{
	Assert( count == 0 || ptr != NULL );
}

void _AssertValidWritePtr( void *ptr, int count )
{
	Assert( count == 0 || ptr != NULL );
}

void _AssertValidReadWritePtr( void *ptr, int count )
{
	Assert( count == 0 || ptr != NULL );
}

void _AssertValidStringPtr( const tchar *ptr, int /*maxchar*/ )
{
	Assert( ptr != NULL );
}
