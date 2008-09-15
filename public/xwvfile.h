//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef XWVFILE_H
#define XWVFILE_H
#ifdef _WIN32
#pragma once
#endif

#define XWV_ID	(('X'<<24)|('W'<<16)|('V'<<8)|('1'<<0))

#define XBOX_ADPCM_SAMPLES_PER_BLOCK		64
#define XBOX_ADPCM_BLOCK_SIZE_PER_CHANNEL	36

enum xwvsamplerates_t
{
	XWV_RATE_11025 = 0,
	XWV_RATE_22050 = 1,
	XWV_RATE_44100 = 2,
};

enum xwvformats_t
{
	XWV_FORMAT_PCM = 0,
	XWV_FORMAT_XBOX_ADPCM = 1,
};

struct xwvheader_t
{
	unsigned int	id;
	unsigned int	headerSize;
	unsigned int	dataSize;
	unsigned int	dataOffset;
	int				loopStart;
	unsigned short	vdatSize;
	byte			format;
	byte			bitsPerSample;
	byte			sampleRate : 4;
	byte			channels : 4;
	byte			unused;

	inline unsigned int GetPreloadSize() { return headerSize + vdatSize; }

	inline int GetBitsPerSample() const { return bitsPerSample; }

	int GetSampleRate() const
	{
		int rates[] = {11025, 22050, 44100};
		int rate = sampleRate;
		return rates[rate]; 
	}
	
	inline int GetChannels() const { return channels; }

	void SetSampleRate( int sampleRateIn )
	{
		byte rate = (sampleRateIn == 11025) ? XWV_RATE_11025 : (sampleRateIn==22050)? XWV_RATE_22050 : XWV_RATE_44100;
		sampleRate = rate;
	}

	inline void SetChannels( int channelsIn ) { channels = channelsIn; }
};


#endif // XWVFILE_H
