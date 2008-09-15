//========= Copyright © 1996-2004, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#ifndef STEAMCLIENTPUBLIC_H
#define STEAMCLIENTPUBLIC_H
#ifdef _WIN32
#pragma once
#endif
//lint -save -e1931 -e1927 -e1924 -e613 -e726

// This header file defines the interface between the calling application and the code that
// knows how to communicate with the connection manager (CM) from the Steam service 

// This header file is intended to be portable; ideally this 1 header file plus a lib or dll
// is all you need to integrate the client library into some other tree.  So please avoid
// including or requiring other header files if possible.  This header should only describe the 
// interface layer, no need to include anything about the implementation.

#include "steamtypes.h"

// General result codes
enum EResult
{
	k_EResultOK	= 1,							// success
	k_EResultFail = 2,							// generic failure 
	k_EResultNoConnection = 3,					// no/failed network connection
//	k_EResultNoConnectionRetry = 4,				// OBSOLETE - removed
	k_EResultInvalidPassword = 5,				// password/ticket is invalid
	k_EResultLoggedInElsewhere = 6,				// same user logged in elsewhere
	k_EResultInvalidProtocolVer = 7,			// protocol version is incorrect
	k_EResultInvalidParam = 8,					// a parameter is incorrect
	k_EResultFileNotFound = 9,					// file was not found
	k_EResultBusy = 10,							// called method busy - action not taken
	k_EResultInvalidState = 11,					// called object was in an invalid state
	k_EResultInvalidName = 12,					// name is invalid
	k_EResultInvalidEmail = 13,					// email is invalid
	k_EResultDuplicateName = 14,				// name is not unique
	k_EResultAccessDenied = 15,					// access is denied
	k_EResultTimeout = 16,						// operation timed out
	k_EResultBanned = 17,						// VAC2 banned
	k_EResultAccountNotFound = 18,				// account not found
	k_EResultInvalidSteamID = 19,				// steamID is invalid
	k_EResultServiceUnavailable = 20,			// The requested service is currently unavailable
	k_EResultNotLoggedOn = 21,					// The user is not logged on
	k_EResultPending = 22,						// Request is pending (may be in process, or waiting on third party)
};

// Result codes to GSHandleClientDeny/Kick
typedef enum
{
	k_EDenyInvalidVersion = 1,
	k_EDenyGeneric = 2,
	k_EDenyNotLoggedOn = 3,
	k_EDenyNoLicense = 4,
	k_EDenyCheater = 5,
	k_EDenyLoggedInElseWhere = 6,
	k_EDenyUnknownText = 7,
	k_EDenyIncompatibleAnticheat = 8,
	k_EDenyMemoryCorruption = 9,
	k_EDenyIncompatibleSoftware = 10,
	k_EDenySteamConnectionLost = 11,
	k_EDenySteamConnectionError = 12,
	k_EDenySteamResponseTimedOut = 13,
	k_EDenySteamValidationStalled = 14,
} EDenyReason;

// Steam universes.  Each universe is a self-contained Steam instance.
enum EUniverse
{
	k_EUniverseInvalid = 0,
	k_EUniversePublic = 1,
	k_EUniverseBeta = 2,
	k_EUniverseInternal = 3,
	k_EUniverseDev = 4,
	k_EUniverseRC = 5,

	k_EUniverseMax
};

// Steam account types
enum EAccountType
{
	k_EAccountTypeInvalid = 0,			
	k_EAccountTypeIndividual = 1,		// single user account
	k_EAccountTypeMultiseat = 2,		// multiseat (e.g. cybercafe) account
	k_EAccountTypeGameServer = 3,		// game server account
	k_EAccountTypeAnonGameServer = 4,	// anonomous game server account
	k_EAccountTypePending = 5			// pending
};

// Enums for all personal questions supported by the system.
enum EPersonalQuestion
{
	// Never ever change these after initial release.
	k_EPSMsgNameOfSchool = 0,			// Question: What is the name of your school?
	k_EPSMsgFavoriteTeam = 1,			// Question: What is your favorite team?
	k_EPSMsgMothersName = 2,			// Question: What is your mother's maiden name?
	k_EPSMsgNameOfPet = 3,				// Question: What is the name of your pet?
	k_EPSMsgChildhoodHero = 4,			// Question: Who was your childhood hero?
	k_EPSMsgCityBornIn = 5,				// Question: What city were you born in?

	k_EPSMaxPersonalQuestion
};

// Payment methods for purchases - BIT FLAGS so can be used to indicate
// acceptable payment methods for packages
enum EPaymentMethod
{
	k_EPaymentMethodNone = 0x00,
	k_EPaymentMethodCDKey = 0x01,		
	k_EPaymentMethodCreditCard = 0x02,
	k_EPaymentMethodPayPal = 0x04,		
	k_EPaymentMethodManual = 0x08,		// Purchase was added by Steam support
};

// License types
enum ELicenseType
{
	k_ENoLicense,								// for shipped goods
	k_ESinglePurchase,							// single purchase
	k_ESinglePurchaseLimitedUse,				// single purchase w/ expiration
	k_ERecurringCharge,							// recurring subsription
	k_ERecurringChargeLimitedUse,				// recurring subscription w/ limited minutes per period
	k_ERecurringChargeLimitedUseWithOverages,	// like above but w/ soft limit and overage charges
};

// Flags for licenses - BITS
enum ELicenseFlags
{
	k_ELicenseFlagRenew = 0x01,				// Renew this license next period
	k_ELicenseFlagRenewalFailed = 0x02,		// Auto-renew failed
	k_ELicenseFlagPending = 0x04,			// Purchase or renewal is pending
	k_ELicenseFlagExpired = 0x08,			// Regular expiration (no renewal attempted)
	k_ELicenseFlagCancelledByUser = 0x10,	// Cancelled by the user
	k_ELicenseFlagCancelledByAdmin = 0x20,	// Cancelled by customer support
};

// Status of a package
enum EPackageStatus
{
	k_EPackageAvailable = 0,		// Available for purchase and use
	k_EPackagePreorder = 1,			// Available for purchase, as a pre-order
	k_EPackageUnavailable = 2,		// Not available for new purchases, may still be owned
	k_EPackageInvalid = 3,			// Either an unknown package or a deleted one that nobody should own
};

// Enum for the types of news push items you can get
enum ENewsUpdateType
{
	k_EAppNews = 0,	 // news about a particular app
	k_ESteamAds = 1, // Marketing messages
	k_ESteamNews = 2, // EJ's corner and the like
	k_ECDDBUpdate = 3, // backend has a new CDDB for you to load
	k_EClientUpdate = 4,	// new version of the steam client is available
};

// Detailed purchase result codes for the client
enum EPurchaseResultDetail 
{
	k_EPurchaseResultNoDetail = 0,
	k_EPurchaseResultAVSFailure = 1,
	k_EPurchaseResultInsufficientFunds = 2,
	k_EPurchaseResultContactSupport = 3,
	k_EPurchaseResultTimeout = 4,

	// these are mainly used for testing
	k_EPurchaseResultInvalidPackage = 5,
	k_EPurchaseResultInvalidPaymentMethod = 6,
	k_EPurchaseResultInvalidData = 7,
	k_EPurchaseResultOthersInProgress = 8,
	k_EPurchaseResultAlreadyPurchased = 9,
	k_EPurchaseResultWrongPrice = 10
};

// Type of system IM.  The client can use this to do special UI handling in specific circumstances
enum ESystemIMType
{
	k_ESystemIMRawText = 0,
	k_ESystemIMInvalidCard = 1,
	k_ESystemIMRecurringPurchaseFailed = 2,
	k_ESystemIMCardWillExpire = 3,
	k_ESystemIMSubscriptionExpired = 4,

	//
	k_ESystemIMTypeMax
};

#pragma pack( push, 1 )		

// Steam ID structure (64 bits total)
class CSteamID
{
public:

	//-----------------------------------------------------------------------------
	// Purpose: Constructor
	//-----------------------------------------------------------------------------
	CSteamID()
	{
		m_unAccountID = 0;
		m_EAccountType = k_EAccountTypeInvalid;
		m_EUniverse = k_EUniverseInvalid;
		m_unAccountInstance = 0;
	}


	//-----------------------------------------------------------------------------
	// Purpose: Constructor
	// Input  : unAccountID -	32-bit account ID
	//			eUniverse -		Universe this account belongs to
	//			eAccountType -	Type of account
	//-----------------------------------------------------------------------------
	CSteamID( uint32 unAccountID, EUniverse eUniverse, EAccountType eAccountType )
	{
		Set( unAccountID, eUniverse, eAccountType );
	}


	//-----------------------------------------------------------------------------
	// Purpose: Constructor
	// Input  : unAccountID -	32-bit account ID
	//			unAccountInstance - instance 
	//			eUniverse -		Universe this account belongs to
	//			eAccountType -	Type of account
	//-----------------------------------------------------------------------------
	CSteamID( uint32 unAccountID, unsigned int unAccountInstance, EUniverse eUniverse, EAccountType eAccountType )
	{
#if defined(_SERVER) && defined(Assert)
		Assert( ! ( ( k_EAccountTypeIndividual == eAccountType ) && ( 1 != unAccountInstance ) ) );	// enforce that for individual accounts, instance is always 1
#endif // _SERVER
		InstancedSet( unAccountID, unAccountInstance, eUniverse, eAccountType );
	}


	//-----------------------------------------------------------------------------
	// Purpose: Constructor
	// Input  : ulSteamID -		64-bit representation of a Steam ID
	// Note:	Will not accept a uint32 or int32 as input, as that is a probable mistake.
	//			See the stubbed out overloads in the private: section for more info.
	//-----------------------------------------------------------------------------
	CSteamID( uint64 ulSteamID )
	{
		SetFromUint64( ulSteamID );
	}


	//-----------------------------------------------------------------------------
	// Purpose: Sets parameters for steam ID
	// Input  : unAccountID -	32-bit account ID
	//			eUniverse -		Universe this account belongs to
	//			eAccountType -	Type of account
	//-----------------------------------------------------------------------------
	void Set( uint32 unAccountID, EUniverse eUniverse, EAccountType eAccountType )
	{
		m_unAccountID = unAccountID;
		m_EUniverse = eUniverse;
		m_EAccountType = eAccountType;
		m_unAccountInstance = 1;
	}


	//-----------------------------------------------------------------------------
	// Purpose: Sets parameters for steam ID
	// Input  : unAccountID -	32-bit account ID
	//			eUniverse -		Universe this account belongs to
	//			eAccountType -	Type of account
	//-----------------------------------------------------------------------------
	void InstancedSet( uint32 unAccountID, uint32 unInstance, EUniverse eUniverse, EAccountType eAccountType )
	{
		m_unAccountID = unAccountID;
		m_EUniverse = eUniverse;
		m_EAccountType = eAccountType;
		m_unAccountInstance = unInstance;
	}


	//-----------------------------------------------------------------------------
	// Purpose: Initializes a steam ID from its 52 bit parts and universe/type
	// Input  : ulIdentifier - 52 bits of goodness
	//-----------------------------------------------------------------------------
	void FullSet( uint64 ulIdentifier, EUniverse eUniverse, EAccountType eAccountType )
	{
		m_unAccountID = ( ulIdentifier & 0xFFFFFFFF );						// account ID is low 32 bits
		m_unAccountInstance = ( ( ulIdentifier >> 32 ) & 0xFFFFF );			// account instance is next 20 bits
		m_EUniverse = eUniverse;
		m_EAccountType = eAccountType;
	}


	//-----------------------------------------------------------------------------
	// Purpose: Initializes a steam ID from its 64-bit representation
	// Input  : ulSteamID -		64-bit representation of a Steam ID
	//-----------------------------------------------------------------------------
	void SetFromUint64( uint64 ulSteamID )
	{
		m_unAccountID = ( ulSteamID & 0xFFFFFFFF );							// account ID is low 32 bits
		m_unAccountInstance = ( ( ulSteamID >> 32 ) & 0xFFFFF );			// account instance is next 20 bits

		m_EAccountType = ( EAccountType ) ( ( ulSteamID >> 52 ) & 0xF );	// type is next 4 bits
		m_EUniverse = ( EUniverse ) ( ( ulSteamID >> 56 ) & 0xFF );			// universe is next 8 bits
	}


#if defined( INCLUDED_STEAM_COMMON_STEAMCOMMON_H ) 
	//-----------------------------------------------------------------------------
	// Purpose: Initializes a steam ID from a Steam2 ID structure
	// Input:	pTSteamGlobalUserID -	Steam2 ID to convert
	//			eUniverse -				universe this ID belongs to
	//-----------------------------------------------------------------------------
	void SetFromSteam2( TSteamGlobalUserID *pTSteamGlobalUserID, EUniverse eUniverse )
	{
		m_unAccountID = pTSteamGlobalUserID->m_SteamLocalUserID.Split.Low32bits * 2 + 
			pTSteamGlobalUserID->m_SteamLocalUserID.Split.High32bits;
		m_EUniverse = eUniverse;		// set the universe
		m_EAccountType = k_EAccountTypeIndividual; // Steam 2 accounts always map to account type of individual
		m_unAccountInstance = 1;	// individual accounts always have an account instance ID of 1
	}

	//-----------------------------------------------------------------------------
	// Purpose: Fills out a Steam2 ID structure
	// Input:	pTSteamGlobalUserID -	Steam2 ID to write to
	//-----------------------------------------------------------------------------
	void ConvertToSteam2( TSteamGlobalUserID *pTSteamGlobalUserID ) const
	{
		// only individual accounts have any meaning in Steam 2, only they can be mapped
		// Assert( m_EAccountType == k_EAccountTypeIndividual );

		pTSteamGlobalUserID->m_SteamInstanceID = 0;
		pTSteamGlobalUserID->m_SteamLocalUserID.Split.High32bits = m_unAccountID % 2;
		pTSteamGlobalUserID->m_SteamLocalUserID.Split.Low32bits = m_unAccountID / 2;
	}
#endif // defined( INCLUDED_STEAM_COMMON_STEAMCOMMON_H )

	//-----------------------------------------------------------------------------
	// Purpose: Converts steam ID to its 64-bit representation
	// Output : 64-bit representation of a Steam ID
	//-----------------------------------------------------------------------------
	uint64 ConvertToUint64() const
	{
		return (uint64) ( ( ( (uint64) m_EUniverse ) << 56 ) + ( ( (uint64) m_EAccountType ) << 52 ) + 
			( ( (uint64) m_unAccountInstance ) << 32 ) + m_unAccountID );
	}


	//-----------------------------------------------------------------------------
	// Purpose: Converts the static parts of a steam ID to a 64-bit representation.
	//			For multiseat accounts, all instances of that account will have the
	//			same static account key, so they can be grouped together by the static
	//			account key.
	// Output : 64-bit static account key
	//-----------------------------------------------------------------------------
	uint64 GetStaticAccountKey() const
	{
		// note we do NOT include the account instance (which is a dynamic property) in the static account key
		return (uint64) ( ( ( (uint64) m_EUniverse ) << 56 ) + ((uint64) m_EAccountType << 52 ) + m_unAccountID );
	}


	//-----------------------------------------------------------------------------
	// Purpose: create an anonomous game server login to be filled in by the AM
	//-----------------------------------------------------------------------------
	void CreateBlankAnonLogon( EUniverse eUniverse )
	{
		m_unAccountID = 0;
		m_EAccountType = k_EAccountTypeAnonGameServer;
		m_EUniverse = eUniverse;
		m_unAccountInstance = 0;
	}

	//-----------------------------------------------------------------------------
	// Purpose: Is this an anonomous game server login that will be filled in?
	//-----------------------------------------------------------------------------
	bool BBlankAnonAccount() const
	{
		return m_unAccountID == 0 && 
			m_EAccountType == k_EAccountTypeAnonGameServer &&
			m_unAccountInstance == 0;
	}

	//-----------------------------------------------------------------------------
	// Purpose: Is this a game server account id?
	//-----------------------------------------------------------------------------
	bool BGameServerAccount() const
	{
		return m_EAccountType == k_EAccountTypeGameServer || m_EAccountType == k_EAccountTypeAnonGameServer;
	}

	// simple accessors
	void SetAccountID( uint32 unAccountID )		{ m_unAccountID = unAccountID; }
	uint32 GetAccountID() const					{ return m_unAccountID; }
	uint32 GetUnAccountInstance() const			{ return m_unAccountInstance; }
	EAccountType GetEAccountType() const		{ return m_EAccountType; }
	EUniverse GetEUniverse() const				{ return m_EUniverse; }
	void SetEUniverse( EUniverse eUniverse )	{ m_EUniverse = eUniverse; }
	bool IsValid() const						{ return !( m_EAccountType == k_EAccountTypeInvalid ); }

	// this set of functions is hidden, will be moved out of class
	CSteamID( const char *pchSteamID, EUniverse eDefaultUniverse = k_EUniverseInvalid );
	char * Render() const;				// renders this steam ID to string
	static char * Render( uint64 ulSteamID );	// static method to render a uint64 representation of a steam ID to a string

	void SetFromString( const char *pchSteamID, EUniverse eDefaultUniverse );
	bool SetFromSteam2String( const char *pchSteam2ID, EUniverse eUniverse );

	bool operator==( const CSteamID &val ) const { return ( ( val.m_unAccountID == m_unAccountID ) && ( val.m_unAccountInstance == m_unAccountInstance )
													&& ( val.m_EAccountType == m_EAccountType ) &&  ( val.m_EUniverse == m_EUniverse ) ); } 
	bool operator!=( const CSteamID &val ) const { return !operator==( val ); }

	// DEBUG function
	bool BValidExternalSteamID() const;

private:

	// These are defined here to prevent accidental implicit conversion of a u32AccountID to a CSteamID.
	// If you get a compiler error about an ambiguous constructor/function then it may be because you're
	// passing a 32-bit int to a function that takes a CSteamID. You should explicitly create the SteamID
	// using the correct Universe and account Type/Instance values.
	CSteamID( uint32 );
	CSteamID( int32 );

	// 64 bits total
	uint32				m_unAccountID : 32;			// unique account identifier
	unsigned int		m_unAccountInstance : 20;	// dynamic instance ID (used for multiseat type accounts only)
	EAccountType		m_EAccountType : 4;			// type of account
	EUniverse			m_EUniverse : 8;			// universe this account belongs to
};

const int k_unSteamAccountIDMask = 0xFFFFFFFF;
const int k_unSteamAccountInstanceMask = 0x000FFFFF;

// This steamID comes from a user game connection to an out of date GS that hasnt implemented the protocol
// to provide its steamID
const CSteamID k_steamIDOutofDateGS( 0, 0, k_EUniverseInvalid, k_EAccountTypeInvalid );
// This steamID comes from a user game connection to an sv_lan GS
const CSteamID k_steamIDLanModeGS( 0, 0, k_EUniversePublic, k_EAccountTypeInvalid );
// This steamID can come from a user game connection to a GS that has just booted but hasnt yet even initialized
// its steam3 component and started logging on.
const CSteamID k_steamIDNotInitYetGS( 1, 0, k_EUniverseInvalid, k_EAccountTypeInvalid );

#pragma pack( pop )


// IVAC
// This is the wrapper class for all VAC functionaility in the client
class IVAC
{
public:
	virtual bool BVACCreateProcess(  
		void *lpVACBlob,
		unsigned int cbBlobSize,
		const char *lpApplicationName,
		char *lpCommandLine,
		uint32 dwCreationFlags,
		void *lpEnvironment,
		char *lpCurrentDirectory,
		uint32 nGameID
		) = 0;

	virtual void KillAllVAC() = 0;

	virtual uint8 *PbLoadVacBlob( int *pcbVacBlob ) = 0;
	virtual void FreeVacBlob( uint8 *pbVacBlob ) = 0;

	virtual void RealHandleVACChallenge( int nClientGameID, uint8 *pubChallenge, int cubChallenge ) = 0;
};


const int k_nGameIDUnknown = -1;
// this is a bogus number picked to be beyond any real steam2 uAppID
const int k_nGameIDNotepad = 65535;
const int k_nGameIDCSSTestApp = 65534;
// this is the real steam2 uAppID for Counter-Strike Source
const int k_nGameIDCSS = 240;
// DOD:Source
const int k_nGameIDDODSRC = 300;
// this one is half life 2 deathmatch
const int k_nGameIDHL2DM = 320;
// Counter-Strike on the HL1 engine
const int k_nGameIDCS = 10;

// Assorted HL1 Games
const int k_nGameIDTFC = 20;
const int k_nGameIDDOD = 30;
const int k_nGameIDDMC = 40;
const int k_nGameIDOpFor = 50;
const int k_nGameIDRicochet = 60;
const int k_nGameIDHL1 = 70; // this ID is also for any 3rd party HL1 mods
const int k_nGameIDCZero = 80;

// 3rd party games
const int k_nGameIDRedOrchestra = 1200;
const int k_nGameIDRedOrchestraBeta = 1210;
const int k_nGameIDSin1 = 1300;
const int k_nGameIDSin1Beta = 1309;

// there is a mapping of these numbers to strings in mpGameIDToGameDesc
// in misc.cpp, keep in sync until we get the real strings from the CDDB and remove the mapping

// Alfred's magic numbers
#define BSrcGame( nGameID ) ( ( nGameID ) >= 200 && ( nGameID ) < 1000 )
#define BGoldSRCGame( nGameID ) ( nGameID ) < 200 
//lint -restore

#endif // STEAMCLIENTPUBLIC_H
