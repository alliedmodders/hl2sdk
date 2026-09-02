//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//===========================================================================//

#ifndef ISOUNDSYSTEM_H
#define ISOUNDSYSTEM_H
#ifdef _WIN32
#pragma once
#endif

#include "appframework/IAppSystem.h"
#include "soundflags.h"

class CAudioState;

#define SOUNDSYSTEM_INTERFACE_VERSION "SoundSystem001"

abstract_class ISoundSystem : public IAppSystem
{
private:
	virtual void unk001() = 0;
	virtual void unk002() = 0;
	virtual void unk003() = 0;
	virtual void unk004() = 0;
	virtual void unk005() = 0;
	virtual void unk006() = 0;
	virtual void unk007() = 0; // Behaves like IEngineSound::CreateOutputStream
	virtual void unk008() = 0;
	virtual void unk009() = 0; // Behaves like IEngineSound::DestroyOutputStream
	virtual void unk010() = 0;
	virtual void unk011() = 0;

public:
	virtual void Update( const CAudioState *pAudioState, bool bUnk ) = 0;

private:
	virtual void unk101() = 0; // Behaves like IEngineSound::StopAllSounds
	virtual void unk102() = 0;
	virtual void unk103() = 0;
	virtual void unk104() = 0; // Behaves like IEngineSound::StopSoundByGuid
	virtual void unk105() = 0;
	virtual void unk106() = 0;
	virtual void unk107() = 0;
	virtual void unk108() = 0; // Behaves like IEngineSound::IsSoundStillPlaying
	virtual void unk109() = 0; // Behaves like IEngineSound::SetVolumeByGuid
	virtual void unk110() = 0;
	virtual void unk111() = 0;
	virtual void unk112() = 0;
	virtual void unk113() = 0;
	virtual void unk114() = 0;
	virtual void unk115() = 0;

public:
	virtual float GetDistGainFromSoundLevel( soundlevel_t soundLevel, float flDistance, float flDbLoss = 0.0f ) = 0;
};

#endif // ISOUNDSYSTEM_H
