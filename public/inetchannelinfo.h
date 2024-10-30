//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//
#if !defined( INETCHANNELINFO_H )
#define INETCHANNELINFO_H
#ifdef _WIN32
#pragma once
#endif


#define FLOW_OUTGOING	0		
#define FLOW_INCOMING	1
#define MAX_FLOWS		2		// in & out

struct SNetChannelLatencyStats;

struct NetChanStat_t
{
	float m_flAvg;
	float m_flMin;
	float m_flMax;
	float m_flStdDev;
};

class INetChannelInfo
{
public:

	enum {
		GENERIC = 0,	// must be first and is default group
		LOCALPLAYER,	// bytes for local player entity update
		OTHERPLAYERS,	// bytes for other players update
		ENTITIES,		// all other entity bytes
		SOUNDS,			// game sounds
		EVENTS,			// event messages
		TEMPENTS,		// temp entities
		USERMESSAGES,	// user messages
		ENTMESSAGES,	// entity messages
		VOICE,			// voice data
		STRINGTABLE,	// a stringtable update
		MOVE,			// client move cmds
		STRINGCMD,		// string command
		SIGNON,			// various signondata
		TOTAL,			// must be last and is not a real group
	};
	

	virtual const char  *GetName( void ) const = 0;			// get channel name						// 0
	virtual const char  *GetAddress( void ) const = 0;		// get channel IP address as string		// 1
	virtual float		GetTime( void ) const = 0;			// current net time						// 2
	virtual float		GetTimeConnected( void ) const = 0;	// get connection time in seconds		// 3
	virtual int			GetBufferSize( void ) const = 0;	// netchannel packet history size		// 4
	virtual int			GetDataRate( void ) const = 0;		// send data rate in byte/sec			// 5
	virtual bool		IsLocalHost( void ) const = 0;	// true if localhost						// 6	
	virtual bool		IsLoopback( void ) const = 0;	// true if loopback channel					// 7
	virtual bool		IsTimingOut( void ) const = 0;	// true if timing out						// 8
	virtual bool		IsPlayback( void ) const = 0;	//always true								// 9
	virtual float		GetLatency(void) const = 0;	// average packet latency in seconds			// 10
	virtual float		GetAvgLatency( void ) const = 0;	// average packet latency in seconds	// 11
	virtual float		GetAvgLoss( int flow ) const = 0;	 // avg packet loss[0..1]				// 12
	virtual float		GetAvgChoke(int flow) const = 0;	 //										// 13
	virtual float		GetAvgData( int flow ) const = 0;	 // data flow in bytes/sec				// 14
	virtual float		GetAvgPackets( int flow ) const = 0; // avg packets/sec						// 15
	virtual int			GetTotalData( int flow ) const = 0;	 // total flow in/out in bytes			// 16
	virtual int			GetTotalPackets( int flow ) const = 0;										// 17
	virtual int			GetSequenceNr( int flow ) const = 0;	// last send seq number				// 18
	virtual float		GetTimeSinceLastReceived( void ) const = 0;	// get time since last recieved packet in seconds	//19
	virtual void		GetRemoteFramerate( float *a1, float *a2, float *a3 ) const = 0;			//20
	virtual float		GetTimeoutSeconds( void ) const = 0;										//21
	virtual float		GetTimeUntilTimeout( void ) const = 0;										//22
	virtual void		sub_180096C10_GetUnknownFloat() const = 0;									//23
	virtual void		ResetLatencyStats( int channel ) = 0;										//24
	virtual SNetChannelLatencyStats *GetLatencyStats( int channel ) const = 0;						//25
	virtual void		SetLatencyStats( int channel, const SNetChannelLatencyStats &stats ) = 0;	//26
	virtual void		SetInterpolationAmount(float flInterpolationAmount, float flUpdateRate) = 0;//27
	virtual void		SetNumPredictionErrors( int a1 ) = 0;										//28
	virtual void		SetShowNetMessages(bool bShow) = 0;											//29
	virtual int			sub_18009CF60_GetUnknownCount(int a1, unsigned short* a2, int maxCount) = 0;		//30
};

#endif // INETCHANNELINFO_H


