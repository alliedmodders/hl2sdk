//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//


#include "cbase.h"
#include "soundscape_system.h"
#include "soundscape.h"
#include "KeyValues.h"
#include "filesystem.h"
#include "game.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define SOUNDSCAPE_MANIFEST_FILE				"scripts/soundscapes_manifest.txt"

CON_COMMAND(soundscape_flush, "Flushes the server & client side soundscapes")
{
	g_SoundscapeSystem.FlushSoundscapes();	// don't bother forgetting about the entities
	g_SoundscapeSystem.Init();
	engine->ClientCommand( UTIL_GetCommandClient()->edict(), "cl_soundscape_flush\n" );
}

CSoundscapeSystem g_SoundscapeSystem( "CSoundscapeSystem" );


void CSoundscapeSystem::AddSoundscapeFile( const char *filename )
{
	MEM_ALLOC_CREDIT();
	// Open the soundscape data file, and abort if we can't
	KeyValues *pKeyValuesData = new KeyValues( filename );
	if ( filesystem->LoadKeyValues( *pKeyValuesData, IFileSystem::TYPE_SOUNDSCAPE, filename, "GAME" ) )
	{
		// parse out all of the top level sections and save their names
		KeyValues *pKeys = pKeyValuesData;
		while ( pKeys )
		{
			if ( pKeys->GetFirstSubKey() )
			{
				if ( g_pDeveloper->GetBool() )
				{
					if ( strstr( pKeys->GetName(), "{" ) )
					{
						Msg("Error parsing soundscape file %s after %s\n", filename, m_soundscapeCount>0 ?m_soundscapes.GetStringText( m_soundscapeCount-1 ) : "FIRST" );
					}
				}
				m_soundscapes.AddString( pKeys->GetName(), m_soundscapeCount );
#ifdef _XBOX
				const char *pStr = pKeys->GetName();
				AddSoundscapeSounds( pKeys, m_soundscapeCount );
#endif
				m_soundscapeCount++;
			}
			pKeys = pKeys->GetNextKey();
		}
	}
	pKeyValuesData->deleteThis();
}

CON_COMMAND(sv_soundscape_printdebuginfo, "print soundscapes")
{
	g_SoundscapeSystem.PrintDebugInfo();
}


void CSoundscapeSystem::PrintDebugInfo()
{
	Msg( "\n------- SERVER SOUNDSCAPES -------\n" );
	for ( int key=m_soundscapes.First(); key != m_soundscapes.InvalidIndex(); key = m_soundscapes.Next( key ) )
	{
		int id = m_soundscapes.GetIDForKey( key );
		const char *pName = m_soundscapes.GetStringForKey( key );

		Msg( "- %d: %s\n", id, pName );
	}
	Msg( "-------- SOUNDSCAPE ENTITIES -----\n" );
	for( int entityIndex = 0; entityIndex < m_soundscapeEntities.Size(); ++entityIndex )
	{
		CEnvSoundscape *currentSoundscape = m_soundscapeEntities[entityIndex];
		Msg("- %d: %s x:%.4f y:%.4f z:%.4f\n", 
			entityIndex, 
			currentSoundscape->GetSoundscapeName(), 
			currentSoundscape->GetAbsOrigin().x,
			currentSoundscape->GetAbsOrigin().y,
			currentSoundscape->GetAbsOrigin().z
			);
	}
	Msg( "----------------------------------\n\n" );
}

bool CSoundscapeSystem::Init()
{
	m_soundscapeCount = 0;

	const char *mapname = STRING( gpGlobals->mapname );
	const char *mapSoundscapeFilename = NULL;
	if ( mapname && *mapname )
	{
		mapSoundscapeFilename = UTIL_VarArgs( "scripts/soundscapes_%s.txt", mapname );
	}

	KeyValues *manifest = new KeyValues( SOUNDSCAPE_MANIFEST_FILE );
	if ( filesystem->LoadKeyValues( *manifest, IFileSystem::TYPE_SOUNDSCAPE, SOUNDSCAPE_MANIFEST_FILE, "GAME" ) )
	{
		for ( KeyValues *sub = manifest->GetFirstSubKey(); sub != NULL; sub = sub->GetNextKey() )
		{
			if ( !Q_stricmp( sub->GetName(), "file" ) )
			{
				// Add
				AddSoundscapeFile( sub->GetString() );
				if ( mapSoundscapeFilename && FStrEq( sub->GetString(), mapSoundscapeFilename ) )
				{
					mapSoundscapeFilename = NULL; // we've already loaded the map's soundscape
				}
				continue;
			}

			Warning( "CSoundscapeSystem::Init:  Manifest '%s' with bogus file type '%s', expecting 'file'\n", 
				SOUNDSCAPE_MANIFEST_FILE, sub->GetName() );
		}

		if ( mapSoundscapeFilename && filesystem->FileExists( mapSoundscapeFilename ) )
		{
			AddSoundscapeFile( mapSoundscapeFilename );
		}
	}
	else
	{
		Error( "Unable to load manifest file '%s'\n", SOUNDSCAPE_MANIFEST_FILE );
	}
	manifest->deleteThis();
	m_activeIndex = -1;

	return true;
}

void CSoundscapeSystem::FlushSoundscapes( void )
{
	m_soundscapeCount = 0;
	m_soundscapes.ClearStrings();
}

void CSoundscapeSystem::Shutdown()
{
	FlushSoundscapes();
	m_soundscapeEntities.RemoveAll();
	m_activeIndex = -1;
#ifdef _XBOX
	m_soundscapeSounds.Purge();
#endif

}

void CSoundscapeSystem::LevelInitPreEntity()
{
	g_SoundscapeSystem.Shutdown();
	g_SoundscapeSystem.Init();
}

void CSoundscapeSystem::LevelInitPostEntity()
{
#ifdef _XBOX
	m_soundscapeSounds.Purge();
#endif
}

int	CSoundscapeSystem::GetSoundscapeIndex( const char *pName )
{
	return m_soundscapes.GetStringID( pName );
}

bool CSoundscapeSystem::IsValidIndex( int index )
{
	if ( index >= 0 && index < m_soundscapeCount )
		return true;
	return false;
}

void CSoundscapeSystem::AddSoundscapeEntity( CEnvSoundscape *pSoundscape )
{
	if ( m_soundscapeEntities.Find( pSoundscape ) == -1 )
	{
		m_soundscapeEntities.AddToTail( pSoundscape );
	}
}

void CSoundscapeSystem::RemoveSoundscapeEntity( CEnvSoundscape *pSoundscape )
{
	m_soundscapeEntities.FindAndRemove( pSoundscape );
}

void CSoundscapeSystem::FrameUpdatePostEntityThink()
{
	int total = m_soundscapeEntities.Count();
	if ( total > 0 )
	{
		if ( !m_soundscapeEntities.IsValidIndex(m_activeIndex) )
		{
			m_activeIndex = 0;
		}

		// update 2 soundscape entities each tick
		int count = min(2, total);
		for ( int i = 0; i < count; i++ )
		{
			m_activeIndex++;
			m_activeIndex = m_activeIndex % total;
			m_soundscapeEntities[m_activeIndex]->Update();
		}
	}
}

#ifdef _XBOX
void CSoundscapeSystem::AddSoundscapeSounds( KeyValues *pSoundscape, int soundscapeIndex )
{
	int i = m_soundscapeSounds.AddToTail();
	Assert( i == soundscapeIndex );

	KeyValues *pKey = pSoundscape->GetFirstSubKey();
	while ( pKey )
	{
		if ( !Q_strcasecmp( pKey->GetName(), "playlooping" ) )
		{
			KeyValues *pAmbientKey = pKey->GetFirstSubKey();
			while ( pAmbientKey )
			{
				if ( !Q_strcasecmp( pAmbientKey->GetName(), "wave" ) )
				{
					char const *pSoundName = pAmbientKey->GetString();
					m_soundscapeSounds[i].AddToTail( pSoundName );
				}
				pAmbientKey = pAmbientKey->GetNextKey();
			}
		}
		else if ( !Q_strcasecmp( pKey->GetName(), "playrandom" ) )
		{
			KeyValues *pRandomKey = pKey->GetFirstSubKey();
			while ( pRandomKey )
			{
				if ( !Q_strcasecmp( pRandomKey->GetName(), "rndwave" ) )
				{
					KeyValues *pRndWaveKey = pRandomKey->GetFirstSubKey();
					while ( pRndWaveKey )
					{
						if ( !Q_strcasecmp( pRndWaveKey->GetName(), "wave" ) )
						{
							char const *pSoundName = pRndWaveKey->GetString();
							m_soundscapeSounds[i].AddToTail( pSoundName );
						}
						pRndWaveKey = pRndWaveKey->GetNextKey();
					}
				}
				pRandomKey = pRandomKey->GetNextKey();
			}
		}
		else if ( !Q_strcasecmp( pKey->GetName(), "playsoundscape" ) )
		{
			KeyValues *pPlayKey = pKey->GetFirstSubKey();
			while ( pPlayKey )
			{
				if ( !Q_strcasecmp( pPlayKey->GetName(), "name" ) )
				{
					char const *pSoundName = pPlayKey->GetString();
					m_soundscapeSounds[i].AddToTail( pSoundName );
				}
				pPlayKey = pPlayKey->GetNextKey();
			}
		}
		pKey = pKey->GetNextKey();
	}
}
#endif

#ifdef _XBOX
void CSoundscapeSystem::PrecacheSounds( int soundscapeIndex )
{
	if ( !IsValidIndex( soundscapeIndex ) )
	{
		return;
	}

	int count = m_soundscapeSounds[soundscapeIndex].Count();
	for ( int i=0; i<count; i++ )
	{
		const char *pSound = m_soundscapeSounds[soundscapeIndex][i];
		if ( Q_stristr( pSound, ".wav" ) )
		{
			CBaseEntity::PrecacheSound( pSound );
		}
		else
		{
			// recurse into new soundscape
			PrecacheSounds( GetSoundscapeIndex( pSound ) );
		}
	}
}
#endif