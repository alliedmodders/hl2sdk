//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef SOUNDINFO_H
#define SOUNDINFO_H

#ifdef _WIN32
#pragma once
#endif

#include "bitbuf.h"
#include "const.h"
#include "soundflags.h"
#include "coordsize.h"
#include "vector.h"


#define WRITE_DELTA_UINT( name, length )	\
	if ( name == delta->name )		\
		buffer.WriteOneBit(0);		\
	else					\
	{					\
		buffer.WriteOneBit(1);		\
		buffer.WriteUBitLong( name, length ); \
	}

#define READ_DELTA_UINT( name, length )			\
	if ( buffer.ReadOneBit() != 0 )			\
	{ name = buffer.ReadUBitLong( length ); }\
	else { name = delta->name; }

#define WRITE_DELTA_SINT( name, length )	\
	if ( name == delta->name )		\
		buffer.WriteOneBit(0);		\
	else					\
	{					\
		buffer.WriteOneBit(1);		\
		buffer.WriteSBitLong( name, length ); \
	}

#define READ_DELTA_SINT( name, length )			\
	if ( buffer.ReadOneBit() != 0 )			\
	{ name = buffer.ReadSBitLong( length ); }	\
	else { name = delta->name; }


//-----------------------------------------------------------------------------
struct SoundInfo_t
{
	int				nSequenceNumber;
	int				nEntityIndex;
	int				nChannel;
	const char		*pszName;
	Vector			vOrigin;
	Vector			vDirection;
	float			fVolume;
	soundlevel_t	Soundlevel;
	bool			bLooping;
	int				nPitch;
	Vector			vListenerOrigin;
	int				nFlags;
	int 			nSoundNum;
	float			fDelay;
	bool			bIsSentence;
	bool			bIsAmbient;
	int				nSpeakerEntity;
	
	//---------------------------------
	
	void Set(int newEntity, int newChannel, const char *pszNewName, const Vector &newOrigin, const Vector& newDirection, 
			float newVolume, soundlevel_t newSoundLevel, bool newLooping, int newPitch, const Vector &vecListenerOrigin, int speakerentity )
	{
		nEntityIndex = newEntity;
		nChannel = newChannel;
		pszName = pszNewName;
		vOrigin = newOrigin;
		vDirection = newDirection;
		fVolume = newVolume;
		Soundlevel = newSoundLevel;
		bLooping = newLooping;
		nPitch = newPitch;
		vListenerOrigin = vecListenerOrigin;
		nSpeakerEntity = speakerentity;
	}

	void SetDefault()
	{
		fDelay = DEFAULT_SOUND_PACKET_DELAY;
		fVolume = DEFAULT_SOUND_PACKET_VOLUME;
		Soundlevel = SNDLVL_NORM;
		nPitch = DEFAULT_SOUND_PACKET_PITCH;

		nEntityIndex = 0;
		nSpeakerEntity = -1;
		nChannel = CHAN_STATIC;
		nSoundNum = 0;
		nFlags = 0;
		nSequenceNumber = 0;

		pszName = NULL;
	
		bLooping = false;
		bIsSentence = false;
		bIsAmbient = false;
		
		vOrigin.Init();
		vDirection.Init();
		vListenerOrigin.Init();
	}

	// this cries for Send/RecvTables:
	void WriteDelta( SoundInfo_t *delta, bf_write &buffer)
	{
		WRITE_DELTA_UINT( nEntityIndex, MAX_EDICT_BITS );

		WRITE_DELTA_UINT( nSoundNum, MAX_SOUND_INDEX_BITS );

		WRITE_DELTA_UINT( nFlags, SND_FLAG_BITS_ENCODE );

		WRITE_DELTA_UINT( nChannel, 3 );

		buffer.WriteOneBit( bIsAmbient?1:0 );
		buffer.WriteOneBit( bIsSentence?1:0 ); // NOTE: SND_STOP behavior is different depending on this flag

		if ( nFlags != SND_STOP )
		{
			if ( nSequenceNumber == delta->nSequenceNumber )
			{
				// didn't change, most often case
				buffer.WriteOneBit( 1 );
			}
			else if ( nSequenceNumber == (delta->nSequenceNumber+1) )
			{
				// increased by one
				buffer.WriteOneBit( 0 );
				buffer.WriteOneBit( 1 );
			}
			else
			{
				// send full seqnr
				buffer.WriteUBitLong( 0, 2 ); // 2 zero bits
				buffer.WriteUBitLong( nSequenceNumber, 31 ); 
			}
						
			if ( fVolume == delta->fVolume )
			{
				buffer.WriteOneBit( 0 );
			}
			else
			{
				buffer.WriteOneBit( 1 );
				buffer.WriteByte ( (int)(fVolume*255.0f) );
			}

			WRITE_DELTA_UINT( Soundlevel, MAX_SNDLVL_BITS );

			WRITE_DELTA_UINT( nPitch, 8 );

			if ( fDelay == delta->fDelay )
			{
				buffer.WriteOneBit( 0 );
			}
			else
			{
				buffer.WriteOneBit( 1 );

				// Skipahead works in 10 msec increments

				int iDelay = fDelay * 1000.0f; // transmit as milliseconds

				iDelay = clamp( iDelay, (int)(-10 * MAX_SOUND_DELAY_MSEC), (int)(MAX_SOUND_DELAY_MSEC) );

				if ( iDelay < 0 )
				{
					iDelay /=10;	
				}
				
				buffer.WriteSBitLong( iDelay , MAX_SOUND_DELAY_MSEC_ENCODE_BITS );
			}

			// don't transmit sounds with high prcesion
			WRITE_DELTA_SINT( vOrigin.x, COORD_INTEGER_BITS + 1 );
			WRITE_DELTA_SINT( vOrigin.y, COORD_INTEGER_BITS + 1 );
			WRITE_DELTA_SINT( vOrigin.z, COORD_INTEGER_BITS + 1 );

			WRITE_DELTA_SINT( nSpeakerEntity, MAX_EDICT_BITS + 1 );
		}
	};

	void ReadDelta( SoundInfo_t *delta, bf_read &buffer)
	{
		READ_DELTA_UINT( nEntityIndex, MAX_EDICT_BITS );

		READ_DELTA_UINT( nSoundNum, MAX_SOUND_INDEX_BITS );

		READ_DELTA_UINT( nFlags, SND_FLAG_BITS_ENCODE );

		READ_DELTA_UINT( nChannel, 3 );

		bIsAmbient = buffer.ReadOneBit() != 0;
		bIsSentence = buffer.ReadOneBit() != 0; // NOTE: SND_STOP behavior is different depending on this flag

		if ( nFlags == SND_STOP )
		{
			fVolume = 0;
			Soundlevel = SNDLVL_NONE;
			nPitch = PITCH_NORM;
			pszName = NULL;
			fDelay = 0.0f;
			nSequenceNumber = 0;
		}
		else
		{
			if ( buffer.ReadOneBit() != 0 )
			{
				nSequenceNumber = delta->nSequenceNumber;
			}
			else if ( buffer.ReadOneBit() != 0 )
			{
				nSequenceNumber = delta->nSequenceNumber + 1;
			}
			else
			{

				nSequenceNumber = buffer.ReadUBitLong( 31 );
			}
				
			if ( buffer.ReadOneBit() != 0 )
			{
				fVolume = (float)buffer.ReadByte()/255.0f;
			}
			else
			{
				fVolume = delta->fVolume;
			}

			if ( buffer.ReadOneBit() != 0 )
			{
				Soundlevel = (soundlevel_t)buffer.ReadUBitLong( MAX_SNDLVL_BITS );
			}
			else
			{
				Soundlevel = delta->Soundlevel;
			}

			READ_DELTA_UINT( nPitch, 8 );


			if ( buffer.ReadOneBit() != 0 )
			{
				// Up to 4096 msec delay
				fDelay = (float)buffer.ReadSBitLong( MAX_SOUND_DELAY_MSEC_ENCODE_BITS ) / 1000.0f; ;
				
				if ( fDelay < 0 )
				{
					fDelay *= 10.0f;
				}
			}
			else
			{
				fDelay = delta->fDelay;
			}

			READ_DELTA_SINT( vOrigin.x, COORD_INTEGER_BITS + 1 );
			READ_DELTA_SINT( vOrigin.y, COORD_INTEGER_BITS + 1 );
			READ_DELTA_SINT( vOrigin.z, COORD_INTEGER_BITS + 1 );

			READ_DELTA_SINT( nSpeakerEntity, MAX_EDICT_BITS + 1 );
		}
	}
};

struct SpatializationInfo_t
{
	typedef enum
	{
		SI_INCREATION = 0,
		SI_INSPATIALIZATION
	} SPATIALIZATIONTYPE;

	// Inputs
	SPATIALIZATIONTYPE	type;
	// Info about the sound, channel, origin, direction, etc.
	SoundInfo_t			info;

	// Requested Outputs ( NULL == not requested )
	Vector				*pOrigin;
	QAngle				*pAngles;
	float				*pflRadius;
};

#endif // SOUNDINFO_H
