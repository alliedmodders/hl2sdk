//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef ISOUNDEMITTERSYSTEMBASE_H
#define ISOUNDEMITTERSYSTEMBASE_H
#ifdef _WIN32
#pragma once
#endif


#include "tier1/utldict.h"
#include "soundflags.h"
#include "mathlib/compressed_vector.h"
#include "appframework/IAppSystem.h"
#include "tier3/tier3.h"


#define SOUNDGENDER_MACRO "$gender"
#define SOUNDGENDER_MACRO_LENGTH 7		// Length of above including $

class KeyValues;
typedef unsigned int HSOUNDSCRIPTHASH;
#define SOUNDEMITTER_INVALID_HASH	(HSOUNDSCRIPTHASH)-1


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
struct CSoundParameters
{
	CSoundParameters()
	{
		channel		= CHAN_AUTO; // 0
		volume		= VOL_NORM;  // 1.0f
		pitch		= PITCH_NORM; // 100

		pitchlow	= PITCH_NORM;
		pitchhigh	= PITCH_NORM;

		soundlevel	= SNDLVL_NORM; // 75dB
		soundname[ 0 ] = 0;
		play_to_owner_only = false;
		count		= 0;

		delay_msec	= 0;

		m_nSoundEntryVersion = 1;
		m_hSoundScriptHash = SOUNDEMITTER_INVALID_HASH;
		m_pOperatorsKV = NULL;
		m_nRandomSeed = -1;

		m_bHRTFFollowEntity = false;
		m_bHRTFBilinear = false;

	}

	int				channel;
	float			volume;
	int				pitch;
	int				pitchlow, pitchhigh;
	soundlevel_t	soundlevel;
	// For weapon sounds...
	bool			play_to_owner_only;
	int				count;
	char 			soundname[ 128 ];
	int				delay_msec;
	HSOUNDSCRIPTHASH m_hSoundScriptHash;
	int			    m_nSoundEntryVersion;
	KeyValues		*m_pOperatorsKV;
	int				m_nRandomSeed;

	bool			m_bHRTFFollowEntity;
	bool			m_bHRTFBilinear;
};


// A bit of a hack, but these are just utility function which are implemented in the SouneParametersInternal.cpp file which all users of this lib also compile
const char *SoundLevelToString( soundlevel_t level );
const char *ChannelToString( int channel );
const char *VolumeToString( float volume );
const char *PitchToString( float pitch );
soundlevel_t TextToSoundLevel( const char *key );
int TextToChannel( const char *name );


enum gender_t
{
	GENDER_NONE = 0,
	GENDER_MALE,
	GENDER_FEMALE,
};


#pragma pack(1)
struct SoundFile
{
	SoundFile()
	{
		symbol = UTL_INVAL_SYMBOL;
		gender = GENDER_NONE;
		available = true;
		COMPILE_TIME_ASSERT( sizeof(SoundFile) == 4 );
	}

	CUtlSymbol	symbol;
	byte		gender;
	byte		available;
};

#pragma pack()

#pragma pack(1)
template<typename T>
struct sound_interval_t
{
	T start;
	T range;

	interval_t &ToInterval( interval_t &dest ) const	{ dest.start = start; dest.range = range; return dest; }
	void FromInterval( const interval_t &from )			{ start = from.start; range = from.range; }
	float Random() const								{ return RandomFloat( start, start+range ); }
};


#pragma pack()

typedef sound_interval_t<float16_with_assign> volume_interval_t;
typedef sound_interval_t<uint16> soundlevel_interval_t;
typedef sound_interval_t<uint8> pitch_interval_t;

#pragma pack(1)
struct CSoundParametersInternal
{
	CSoundParametersInternal();
	~CSoundParametersInternal();

	void CopyFrom( const CSoundParametersInternal& src );

	bool operator == ( const CSoundParametersInternal& other ) const;

	const char *VolumeToString( void ) const;
	const char *ChannelToString( void ) const;
	const char *SoundLevelToString( void ) const;
	const char *PitchToString( void ) const;

	void		VolumeFromString( const char *sz );
	void		ChannelFromString( const char *sz );
	void		PitchFromString( const char *sz );
	void		SoundLevelFromString( const char *sz );

	int			GetChannel() const							{ return channel; }
	const volume_interval_t &GetVolume() const				{ return volume; }
	const pitch_interval_t &GetPitch() const				{ return pitch; }
	const soundlevel_interval_t &GetSoundLevel() const		{ return soundlevel; }
	int			GetDelayMsec() const						{ return delay_msec; }
	int			GetSoundEntryVersion() const				{ return m_nSoundEntryVersion; }
	bool		OnlyPlayToOwner() const						{ return play_to_owner_only; }
	bool		HadMissingWaveFiles() const					{ return had_missing_wave_files; }
	bool		UsesGenderToken() const						{ return uses_gender_token; }
	bool		ShouldPreload() const						{ return m_bShouldPreload; }
	bool		ShouldAutoCache() const						{ return m_bShouldAutoCache; }
	bool        HasCached() const	                        { return m_bHasCached; }
	bool		HasHRTFFollowEntity() const					{ return m_bHRTFFollowEntity; }
	bool		HasHRTFBilinear() const						{ return m_bHRTFBilinear; }

	void		SetChannel( int newChannel )				{ channel = newChannel; }
	void		SetVolume( float start, float range = 0.0 )	{ volume.start = ( uint8 )start; volume.range = ( uint8 )range; }
	void		SetPitch( float start, float range = 0.0 )	{ pitch.start = ( uint8 )start; pitch.range = ( uint8 )range; }
	void		SetSoundLevel( float start, float range = 0.0 )	{ soundlevel.start = ( uint16 )start; soundlevel.range = ( uint16 )range; }
	void		SetDelayMsec( int delay )					{ delay_msec = delay; }
	void		SetSoundEntryVersion( int gameSoundVersion )	{ m_nSoundEntryVersion = gameSoundVersion; }
	void		SetShouldPreload( bool bShouldPreload )		{ m_bShouldPreload = bShouldPreload; }
	void		SetShouldAutoCache( bool bShouldAutoCache )	{ m_bShouldAutoCache = bShouldAutoCache; }
	void		SetOnlyPlayToOwner( bool b )				{ play_to_owner_only = b; }
	void		SetHadMissingWaveFiles( bool b )			{ had_missing_wave_files = b; }
	void		SetUsesGenderToken( bool b )				{ uses_gender_token = b; }
	void		SetCached( bool b )							{ m_bHasCached = b; }
	void		SetHRTFFollowEntity( bool b )				{ m_bHRTFFollowEntity = b;  }
	void		SetHRTFBilinear( bool b )					{ m_bHRTFBilinear = b; }

	void		AddSoundName( const SoundFile &soundFile )	{ AddToTail( &m_pSoundNames, &m_nSoundNames, soundFile ); }
	int			NumSoundNames() const						{ return m_nSoundNames; }
	SoundFile * GetSoundNames()								{ return ( m_nSoundNames == 1 ) ? (SoundFile *)&m_pSoundNames : m_pSoundNames; }
	const SoundFile *GetSoundNames() const					{ return ( m_nSoundNames == 1 ) ? (SoundFile *)&m_pSoundNames : m_pSoundNames; }

	void		AddConvertedName( const SoundFile &soundFile ) { AddToTail( &m_pConvertedNames, &m_nConvertedNames, soundFile ); }
	int			NumConvertedNames() const					{ return m_nConvertedNames; }
	SoundFile * GetConvertedNames()							{ return ( m_nConvertedNames == 1 ) ? (SoundFile *)&m_pConvertedNames : m_pConvertedNames; }
	const SoundFile *GetConvertedNames() const				{ return ( m_nConvertedNames == 1 ) ? (SoundFile *)&m_pConvertedNames : m_pConvertedNames; }

	// Sound Operator System: this should be optimized into something less heavy
	KeyValues *GetOperatorsKV( void ) const					{ return m_pOperatorsKV; }
	void SetOperatorsKV( KeyValues *src );

private:
	void operator=( const CSoundParametersInternal& src ); // disallow implicit copies
	CSoundParametersInternal( const CSoundParametersInternal& src );

	void		AddToTail( SoundFile **pDest, uint16 *pDestCount, const SoundFile &source );

	SoundFile *				m_pSoundNames;			// 4
	SoundFile *				m_pConvertedNames;		// 8
	uint16					m_nSoundNames;			// 10
	uint16					m_nConvertedNames;		// 12

	volume_interval_t		volume;					// 16
	soundlevel_interval_t	soundlevel;				// 20
	pitch_interval_t		pitch;					// 22
	uint16					channel;				// 24
	uint16					delay_msec;				// 26
	uint16					m_nSoundEntryVersion;     // 28
	
	bool			play_to_owner_only:1; // For weapon sounds...	// 29
	// Internal use, for warning about missing .wav files
	bool			had_missing_wave_files:1;
	bool			uses_gender_token:1;
	bool			m_bShouldPreload:1;
	bool			m_bHasCached:1;
	bool			m_bShouldAutoCache:1;
	bool			m_bHRTFFollowEntity : 1;
	bool			m_bHRTFBilinear : 1;

	byte			reserved;						// 30

	KeyValues *				m_pGameData;			// 34
	KeyValues *				m_pOperatorsKV;			// 38

};
#pragma pack()


//-----------------------------------------------------------------------------
// Purpose: Base class for sound emitter system handling (can be used by tools)
//-----------------------------------------------------------------------------
abstract_class ISoundEmitterSystemBase : public IAppSystem
{
public:
	// Unused, left in the interface so I don't have to rebuild all
	virtual void			Unused1() {}
	virtual void			Unused2() {}

	virtual int				GetSoundIndex( const char *pName ) const = 0;
	virtual bool			IsValidIndex( int index ) = 0;
	virtual int				GetSoundCount( void ) = 0;

	virtual const char		*GetSoundName( int index ) = 0;
	virtual bool			GetParametersForSound( const char *soundname, CSoundParameters& params, gender_t gender, bool isbeingemitted = false ) = 0;

	virtual const char		*GetWaveName( CUtlSymbol& sym ) = 0;
	virtual CUtlSymbol		AddWaveName( const char *name ) = 0;

	virtual soundlevel_t	LookupSoundLevel( const char *soundname ) = 0;
	virtual const char		*GetWavFileForSound( const char *soundname, char const *actormodel ) = 0;
	virtual const char		*GetWavFileForSound( const char *soundname, gender_t gender ) = 0;
	virtual int				CheckForMissingWavFiles( bool verbose ) = 0;
	virtual const char		*GetSourceFileForSound( int index ) const = 0;

	// Iteration methods
	virtual int				First() const = 0;
	virtual int				Next( int i ) const = 0;
	virtual int				InvalidIndex() const = 0;

	virtual CSoundParametersInternal *InternalGetParametersForSound( int index ) = 0;

	// The host application is responsible for dealing with dirty sound scripts, etc.
	virtual bool			AddSound( const char *soundname, const char *scriptfile, const CSoundParametersInternal& params ) = 0;
	virtual void			RemoveSound( const char *soundname ) = 0;
	virtual void			MoveSound( const char *soundname, const char *newscript ) = 0;
	virtual void			RenameSound( const char *soundname, const char *newname ) = 0;

	virtual void			UpdateSoundParameters( const char *soundname, const CSoundParametersInternal& params ) = 0;

	virtual int				GetNumSoundScripts() const = 0;
	virtual char const		*GetSoundScriptName( int index ) const = 0;
	virtual bool			IsSoundScriptDirty( int index ) const = 0;
	virtual int				FindSoundScript( const char *name ) const = 0;
	virtual void			SaveChangesToSoundScript( int scriptindex ) = 0;

	virtual void			ExpandSoundNameMacros( CSoundParametersInternal& params, char const *wavename ) = 0;
	virtual gender_t		GetActorGender( char const *actormodel ) = 0;
	virtual void			GenderExpandString( char const *actormodel, char const *in, char *out, int maxlen ) = 0;
	virtual void			GenderExpandString( gender_t gender, char const *in, char *out, int maxlen ) = 0;
	virtual bool			IsUsingGenderToken( char const *soundname ) = 0;

	// For blowing away caches based on filetimstamps of the manifest, or of any of the
	//  .txt files that are read into the sound emitter system
	virtual unsigned int	GetManifestFileTimeChecksum() = 0;

	virtual bool			GetParametersForSoundEx( const char *soundname, HSOUNDSCRIPTHASH& handle, CSoundParameters& params, gender_t gender, bool isbeingemitted = false ) = 0;
	virtual soundlevel_t	LookupSoundLevelByHandle( char const *soundname, HSOUNDSCRIPTHASH& handle ) = 0;
	virtual KeyValues		*GetOperatorKVByHandle( HSOUNDSCRIPTHASH& handle ) = 0;

	virtual char const		*GetSoundNameForHash( HSOUNDSCRIPTHASH hash ) const = 0; // Returns NULL if hash not found!!!
	virtual int 			GetSoundIndexForHash( HSOUNDSCRIPTHASH hash ) const = 0;
	virtual HSOUNDSCRIPTHASH HashSoundName( char const *pchSndName ) const = 0;
	virtual bool			IsValidHash( HSOUNDSCRIPTHASH hash ) const = 0;

	virtual void			DescribeSound( char const *soundname ) = 0;
	// Flush and reload
	virtual void			Flush() = 0;

	virtual void			AddSoundsFromFile( const char *filename, bool bPreload, bool bAutoCache, bool bIsOverride = false ) = 0;

};

#endif // ISOUNDEMITTERSYSTEMBASE_H
