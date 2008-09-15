//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef ISOUNDEMITTERSYSTEMBASE_V1_H
#define ISOUNDEMITTERSYSTEMBASE_V1_H
#ifdef _WIN32
#pragma once
#endif

#include "interface.h"

#include "utldict.h"
#include "soundflags.h"
#include "interval.h"

#define SOUNDEMITTERSYSTEM_INTERFACE_VERSION_1	"VSoundEmitter001"

namespace SoundEmitterSystemBaseV1
{

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
};

enum gender_t
{
	NONE = 0,
	MALE,
	FEMALE,
};

struct SoundFile
{
	SoundFile()
	{
		symbol = UTL_INVAL_SYMBOL;
		gender = NONE;
		available = true;
	}

	CUtlSymbol	symbol;
	gender_t	gender;
	bool		available;
};

struct CSoundParametersInternal
{
	// CSoundParametersInternal();
	// CSoundParametersInternal( const CSoundParametersInternal& src );

	bool CompareInterval( const interval_t& i1, const interval_t& i2 ) const;
	bool CSoundParametersInternal::operator == ( const CSoundParametersInternal& other ) const;

	const char *VolumeToString( void );
	const char *ChannelToString( void );
	const char *SoundLevelToString( void );
	const char *PitchToString( void );

	void		VolumeFromString( const char *sz );
	void		ChannelFromString( const char *sz );
	void		PitchFromString( const char *sz );
	void		SoundLevelFromString( const char *sz );

	int				channel;
	interval_t		volume;
	interval_t		pitch;
	interval_t		soundlevel;
	int				delay_msec;
	// For weapon sounds...
	bool			play_to_owner_only;

	CUtlVector< SoundFile >	soundnames;
	CUtlVector< SoundFile > convertednames;
	// Internal use, for warning about missing .wav files
	bool			had_missing_wave_files;
	bool			uses_gender_token;

	enum
	{
		MAX_SOUND_ATTRIB_STRING = 32,
	};

	char			m_szVolume[ MAX_SOUND_ATTRIB_STRING ];
	char			m_szChannel[ MAX_SOUND_ATTRIB_STRING ];
	char			m_szSoundLevel[ MAX_SOUND_ATTRIB_STRING ];
	char			m_szPitch[ MAX_SOUND_ATTRIB_STRING ];

	bool			m_bShouldPreload;
};

//-----------------------------------------------------------------------------
// Purpose: Base class for sound emitter system handling (can be used by tools)
//-----------------------------------------------------------------------------
class ISoundEmitterSystemBase
{
public:
	// 
	virtual bool			Connect( CreateInterfaceFn factory ) = 0;

	virtual bool			Init( ) = 0;
	virtual void			Shutdown() = 0;


public:

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
};

}	// end namespace SoundEmitterSystemBaseV1


#endif // ISOUNDEMITTERSYSTEMBASE_V1_H
