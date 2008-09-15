//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "hud.h"
#include "inetGraphpanel.h"
#include "kbutton.h"
#include <inetchannelinfo.h>
#include "input.h"
#include <vgui/IVgui.h>
#include "VguiMatSurface/IMatSystemSurface.h"
#include <vgui_controls/Panel.h>
#include <vgui_controls/Controls.h>
#include <vgui/ISurface.h>
#include <vgui/IScheme.h>
#include <vgui/ilocalize.h>
#include "tier0/vprof.h"

#include "materialsystem/IMaterialSystem.h"
#include "materialsystem/IMesh.h"
#include "materialsystem/IMaterial.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static ConVar	net_graph			( "net_graph","0", FCVAR_ARCHIVE, "Draw the network usage graph" );
static ConVar	net_scale			( "net_scale", "5", FCVAR_ARCHIVE );
static ConVar	net_graphpos		( "net_graphpos", "1", FCVAR_ARCHIVE );
static ConVar	net_graphsolid		( "net_graphsolid", "1", FCVAR_ARCHIVE );
static ConVar	net_graphheight		( "net_graphheight", "64" );

#define	TIMINGS	1024       // Number of values to track (must be power of 2) b/c of masking
#define FRAMERATE_AVG_FRAC 0.9
#define PACKETLOSS_AVG_FRAC 0.5
#define PACKETCHOKE_AVG_FRAC 0.5

#define NUM_LATENCY_SAMPLES 8

#define GRAPH_RED	(0.9 * 255)
#define GRAPH_GREEN (0.9 * 255)
#define GRAPH_BLUE	(0.7 * 255)

#define LERP_HEIGHT 24

#define COLOR_DROPPED	0
#define COLOR_INVALID	1
#define COLOR_SKIPPED	2
#define COLOR_CHOKED	3
#define COLOR_NORMAL	4

//-----------------------------------------------------------------------------
// Purpose: Displays the NetGraph 
//-----------------------------------------------------------------------------
class CNetGraphPanel : public vgui::Panel
{
	typedef vgui::Panel BaseClass;
private:
	typedef struct
	{
		int latency;
		int	choked;
	} packet_latency_t;

	typedef struct
	{
		unsigned short msgbytes[INetChannelInfo::TOTAL+1];
		int				sampleY;
		int				sampleHeight;

	} netbandwidthgraph_t;

	typedef struct
	{
		 float		cmd_lerp;
		int			size;
		bool		sent;
	} cmdinfo_t;

	typedef struct
	{
		byte color[3];
		byte alpha;
	} netcolor_t;

	byte colors[ LERP_HEIGHT ][3];

	byte sendcolor[ 3 ];
	byte holdcolor[ 3 ];
	byte extrap_base_color[ 3 ];

	packet_latency_t	m_PacketLatency[ TIMINGS ];
	cmdinfo_t			m_Cmdinfo[ TIMINGS ];
	netbandwidthgraph_t	m_Graph[ TIMINGS ];

	float	m_Framerate;
	float   m_AvgLatency;
	float	m_AvgPacketLoss;
	float	m_AvgPacketChoke;
	int		m_IncomingSequence;
	int		m_OutgoingSequence;
	int		m_UpdateWindowSize;
	float	m_IncomingData;
	float	m_OutgoingData;
	float	m_AvgPacketIn;
	float	m_AvgPacketOut;

	int		m_StreamRecv[MAX_FLOWS];
	int		m_StreamTotal[MAX_FLOWS];

	netcolor_t netcolors[5];

	vgui::HFont			m_hFont;
	const ConVar		*cl_updaterate;
	const ConVar		*cl_cmdrate;

public:
						CNetGraphPanel( vgui::VPANEL parent );
	virtual				~CNetGraphPanel( void );

	virtual void		ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void		Paint();
	virtual void		OnTick( void );

	virtual bool		ShouldDraw( void );

	void				InitColors( void );
	int					GraphValue( void );

	struct CLineSegment
	{
		int			x1, y1, x2, y2;
		byte		color[4];
	};

	CUtlVector< CLineSegment >	m_Rects;

	inline void			DrawLine( vrect_t *rect, unsigned char *color, unsigned char alpha );

	void				ResetLineSegments();
	void				DrawLineSegments();

	int					DrawDataSegment( vrect_t *rcFill, int bytes, byte r, byte g, byte b, byte alpha = 255);
	void				DrawUpdateRate( int x, int y );
	void				DrawHatches( int x, int y, int maxmsgbytes );
	void				DrawStreamProgress( int x, int y, int width );
	void				DrawTimes( vrect_t vrect, cmdinfo_t *cmdinfo, int x, int w );
	void				DrawTextFields( int graphvalue, int x, int y, netbandwidthgraph_t *graph, cmdinfo_t *cmdinfo );
	void				GraphGetXY( vrect_t *rect, int width, int *x, int *y );
	void				GetCommandInfo( INetChannelInfo *netchannel, cmdinfo_t *cmdinfo );
	void				GetFrameData( INetChannelInfo *netchannel, int *biggest_message, float *avg_message, float *f95thpercentile );
	void				ColorForHeight( packet_latency_t *packet, byte *color, int *ping, byte *alpha );
	void				GetColorValues( int color, byte *cv, byte *alpha );

private:

	void				PaintLineArt( int x, int y, int w, int graphtype, int maxmsgbytes );
	void				DrawLargePacketSizes( int x, int w, int graphtype, float warning_threshold );

	CMaterialReference	m_WhiteMaterial;

	int m_EstimatedWidth;
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *parent - 
//-----------------------------------------------------------------------------
CNetGraphPanel::CNetGraphPanel( vgui::VPANEL parent )
: BaseClass( NULL, "CNetGraphPanel" )
{
	int w, h;
	vgui::surface()->GetScreenSize( w, h );

	SetParent( parent );
	SetSize( w, h );
	SetPos( 0, 0 );
	SetVisible( false );
	SetCursor( null );

	m_hFont = 0;
	m_EstimatedWidth = 1;

	SetFgColor( Color( 0, 0, 0, 255 ) );
	SetPaintBackgroundEnabled( false );

	InitColors();

	cl_updaterate = cvar->FindVar( "cl_updaterate" );
	cl_cmdrate = cvar->FindVar( "cl_cmdrate" );
	assert( cl_updaterate && cl_cmdrate );

	memset( sendcolor, 0, 3 );
	memset( holdcolor, 0, 3 );
	memset( extrap_base_color, 255, 3 );

	memset( m_PacketLatency, 0, TIMINGS * sizeof( packet_latency_t ) );
	memset( m_Cmdinfo, 0, TIMINGS * sizeof( cmdinfo_t ) );
	memset( m_Graph, 0, TIMINGS * sizeof( netbandwidthgraph_t ) );

	m_Framerate = 0.0f;
	m_AvgLatency = 0.0f;
	m_AvgPacketLoss = 0.0f;
	m_AvgPacketChoke = 0.0f;
	m_IncomingSequence = 0;
	m_OutgoingSequence = 0;
	m_UpdateWindowSize = 0;
	m_IncomingData = 0;
	m_OutgoingData = 0;
	m_AvgPacketIn = 0.0f;
	m_AvgPacketOut = 0.0f;

	netcolors[0].color[0] = 255;
	netcolors[0].color[1] = 0;
	netcolors[0].color[2] = 0;
	netcolors[0].alpha = 255;
	netcolors[1].color[0] = 0;
	netcolors[1].color[1] = 0;
	netcolors[1].color[2] = 255;
	netcolors[1].alpha = 255;
	netcolors[2].color[0] = 240;
	netcolors[2].color[1] = 127;
	netcolors[2].color[2] = 63;
	netcolors[2].alpha = 255;
	netcolors[3].color[0] = 255;
	netcolors[3].color[1] = 255;
	netcolors[3].color[2] = 0;
	netcolors[3].alpha = 255;
	netcolors[4].color[0] = 63;
	netcolors[4].color[1] = 255;
	netcolors[4].color[2] = 63;
	netcolors[4].alpha = 150;

	vgui::ivgui()->AddTickSignal( GetVPanel(), 500 );

	m_WhiteMaterial.Init( "vgui/white", TEXTURE_GROUP_OTHER );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CNetGraphPanel::~CNetGraphPanel( void )
{
}

void CNetGraphPanel::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_hFont = pScheme->GetFont( "DefaultFixedOutline", true );

	// Estimate the width of our panel.
	char str[512];
	wchar_t ustr[512];
	Q_snprintf( str, sizeof( str ), "fps:  435  ping: 533 ms       0/0" );
	vgui::localize()->ConvertANSIToUnicode( str, ustr, sizeof( ustr ) );
	int textTall;
	g_pMatSystemSurface->GetTextSize( m_hFont, ustr, m_EstimatedWidth, textTall );

	assert( m_hFont );

	int w, h;
	vgui::surface()->GetScreenSize( w, h );
	SetSize( w, h );
	SetPos( 0, 0 );
}

//-----------------------------------------------------------------------------
// Purpose: Copies data from netcolor_t array into fields passed in
// Input  : color - 
//			*cv - 
//			*alpha - 
//-----------------------------------------------------------------------------
void CNetGraphPanel::GetColorValues( int color, byte *cv, byte *alpha )
{
	int i;
	netcolor_t *pc = &netcolors[ color ];
	for ( i = 0; i < 3; i++ )
	{
		cv[ i ] = pc->color[ i ];
	}
	*alpha = pc->alpha;
}

//-----------------------------------------------------------------------------
// Purpose: Sets appropriate color values
// Input  : *packet - 
//			*color - 
//			*ping - 
//			*alpha - 
//-----------------------------------------------------------------------------
void CNetGraphPanel::ColorForHeight( packet_latency_t *packet, byte *color, int *ping, byte *alpha )
{
	int h = packet->latency;
	*ping = 0;
	switch ( h )
	{
	case 9999:
		GetColorValues( COLOR_DROPPED, color, alpha );
		break;
	case 9998:
		GetColorValues( COLOR_INVALID, color, alpha );
		break;
	case 9997:
		GetColorValues( COLOR_SKIPPED, color, alpha );
		break;
	default:
		*ping = 1;
		GetColorValues( packet->choked ? COLOR_CHOKED : COLOR_NORMAL, color, alpha );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set up blend colors for comman/client-frame/interpolation graph
//-----------------------------------------------------------------------------
void CNetGraphPanel::InitColors( void )
{
	int i, j;
	byte mincolor[2][3];
	byte maxcolor[2][3];
	float	dc[2][3];
	int		hfrac;	
	float	f;

	mincolor[0][0] = 63;
	mincolor[0][1] = 0;
	mincolor[0][2] = 100;

	maxcolor[0][0] = 0;
	maxcolor[0][1] = 63;
	maxcolor[0][2] = 255;

	mincolor[1][0] = 255;
	mincolor[1][1] = 127;
	mincolor[1][2] = 0;

	maxcolor[1][0] = 250;
	maxcolor[1][1] = 0;
	maxcolor[1][2] = 0;

	for ( i = 0; i < 3; i++ )
	{
		dc[0][i] = (float)(maxcolor[0][i] - mincolor[0][i]);
		dc[1][i] = (float)(maxcolor[1][i] - mincolor[1][i]);
	}

	hfrac = LERP_HEIGHT / 3;

	for ( i = 0; i < LERP_HEIGHT; i++ )
	{
		if ( i < hfrac )
		{
			f = (float)i / (float)hfrac;
			for ( j = 0; j < 3; j++ )
			{
				colors[ i ][j] = mincolor[0][j] + f * dc[0][j];
			}
		}
		else
		{
			f = (float)(i-hfrac) / (float)(LERP_HEIGHT - hfrac );
			for ( j = 0; j < 3; j++ )
			{
				colors[ i ][j] = mincolor[1][j] + f * dc[1][j];
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Draw client framerate / usercommand graph
// Input  : vrect - 
//			*cmdinfo - 
//			x - 
//			w - 
//-----------------------------------------------------------------------------

void CNetGraphPanel::DrawTimes( vrect_t vrect, cmdinfo_t *cmdinfo, int x, int w )
{
	int i;
	int j;
	int	extrap_point;
	int a, h;
	vrect_t  rcFill;
	int ptx, pty;

	// Draw cmd_rate value
	ptx = max( x + w - 1 - 25, 1 );
	pty = max( vrect.y + vrect.height - 4 - LERP_HEIGHT + 1, 1 );
	
	extrap_point = LERP_HEIGHT / 3;

	for (a=0 ; a<w ; a++)
	{
		i = ( m_OutgoingSequence - a ) & ( TIMINGS - 1 );
		h = min( ( cmdinfo[i].cmd_lerp / 3.0 ) * LERP_HEIGHT, LERP_HEIGHT );

		rcFill.x		= x + w -a - 1;
		rcFill.width	= 1;
		rcFill.height	= 1;

		rcFill.y = vrect.y + vrect.height - 4;
		
		if ( h >= extrap_point )
		{
			int start = 0;

			h -= extrap_point;
			rcFill.y -= extrap_point;

			if ( !net_graphsolid.GetInt() )
			{
				rcFill.y -= (h - 1);
				start = (h - 1);
			}

			for ( j = start; j < h; j++ )
			{
				DrawLine(&rcFill, colors[j + extrap_point], 255 );	
				rcFill.y--;
			}
		}
		else
		{
			int oldh;
			oldh = h;
			rcFill.y -= h;
			h = extrap_point - h;

			if ( !net_graphsolid.GetInt() )
			{
				h = 1;
			}

			for ( j = 0; j < h; j++ )
			{
				DrawLine(&rcFill, colors[j + oldh], 255 );	
				rcFill.y--;
			}
		}

		rcFill.y = vrect.y + vrect.height - 4 - extrap_point;

		DrawLine( &rcFill, extrap_base_color, 255 );

		rcFill.y = vrect.y + vrect.height - 3;

		if ( cmdinfo[ i ].sent )
		{
			DrawLine( &rcFill, sendcolor, 255 );
		}
		else
		{
			DrawLine( &rcFill, holdcolor, 200 );
		}
	}

	g_pMatSystemSurface->DrawColoredText( m_hFont, ptx, pty, GRAPH_RED, GRAPH_GREEN, GRAPH_BLUE, 255, "%i/s", cl_cmdrate->GetInt() );

}

//-----------------------------------------------------------------------------
// Purpose: Compute frame database for rendering m_NetChannel computes choked, and lost packets, too.
//  Also computes latency data and sets max packet size
// Input  : *packet_latency - 
//			*graph - 
//			*choke_count - 
//			*loss_count - 
//			*biggest_message - 
//			1 - 
//-----------------------------------------------------------------------------
void CNetGraphPanel::GetFrameData( 	INetChannelInfo *netchannel, int *biggest_message, float *avg_message, float *f95thpercentile )
{
	float	frame_received_time;
	// float	frame_latency;

	*biggest_message	= 0;
	*avg_message		= 0.0f;
	*f95thpercentile	= 0.0f;

	int msg_count = 0;

	m_IncomingSequence = netchannel->GetSequenceNr( FLOW_INCOMING );
	m_OutgoingSequence = netchannel->GetSequenceNr( FLOW_OUTGOING );
	m_UpdateWindowSize = netchannel->GetBufferSize();
	m_AvgPacketLoss	   = netchannel->GetAvgLoss( FLOW_INCOMING );
	m_AvgPacketChoke   = netchannel->GetAvgChoke( FLOW_INCOMING );
	m_AvgLatency	   = netchannel->GetAvgLatency( FLOW_OUTGOING );
	m_IncomingData	   = netchannel->GetAvgData( FLOW_INCOMING ) / 1024.0f;
	m_OutgoingData	   = netchannel->GetAvgData( FLOW_OUTGOING ) / 1024.0f;
	m_AvgPacketIn      = netchannel->GetAvgPackets( FLOW_INCOMING );
	m_AvgPacketOut     = netchannel->GetAvgPackets( FLOW_OUTGOING );

	for ( int i=0; i<MAX_FLOWS; i++ )
		netchannel->GetStreamProgress( i, &m_StreamRecv[i], &m_StreamTotal[i] );

	if ( cl_updaterate->GetFloat() > 0.001f )
	{
		m_AvgLatency -= 0.5f / cl_updaterate->GetFloat();
	}

	// Can't be below zero
	m_AvgLatency = max( 0.0, m_AvgLatency );

	// Fill in frame data
	for ( int seqnr =m_IncomingSequence - m_UpdateWindowSize + 1
		; seqnr <= m_IncomingSequence
		; seqnr++)
	{
		
		
		frame_received_time = netchannel->GetPacketTime( FLOW_INCOMING, seqnr );

		netbandwidthgraph_t *nbwg = &m_Graph[ seqnr & ( TIMINGS - 1 )];

		for ( int i=0; i<=INetChannelInfo::TOTAL; i++ )
		{
			nbwg->msgbytes[i] = netchannel->GetPacketBytes( FLOW_INCOMING, seqnr, i );
		}

		// Assert ( nbwg->msgbytes[INetChannelInfo::TOTAL] > 0 );

		if ( nbwg->msgbytes[INetChannelInfo::TOTAL] > *biggest_message )
		{
			*biggest_message = nbwg->msgbytes[INetChannelInfo::TOTAL];
		}

		*avg_message += (float)( nbwg->msgbytes[INetChannelInfo::TOTAL] );
		msg_count++;
	}

	if ( *biggest_message > 1000 )
	{
		*biggest_message = 1000;
	}

	if ( msg_count >= 1 )
	{
		*avg_message /= msg_count;

		int deviationsquared = 0;

		// Compute std deviation
		// Fill in frame data
		for (int seqnr=m_IncomingSequence - m_UpdateWindowSize + 1
			; seqnr <= m_IncomingSequence
			; seqnr++)
		{
			
			
			int bytes = m_Graph[ seqnr & ( TIMINGS - 1 )].msgbytes[INetChannelInfo::TOTAL];

			deviationsquared += ( bytes * bytes );
		}

		float var = ( float )( deviationsquared ) / (float)( msg_count - 1 );
		float stddev = sqrt( var );

		*f95thpercentile = *avg_message + 2.0f * stddev;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Fills in command interpolation/holdback & message size data
// Input  : *cmdinfo - 
//-----------------------------------------------------------------------------
void CNetGraphPanel::GetCommandInfo( INetChannelInfo *netchannel, cmdinfo_t *cmdinfo )
{
	for ( int seqnr = m_OutgoingSequence - m_UpdateWindowSize + 1
		; seqnr <= m_OutgoingSequence
		; seqnr++)
	{
		// Also set up the lerp point.
		cmdinfo_t *ci = &cmdinfo[ seqnr & ( TIMINGS - 1 ) ];

		ci->cmd_lerp = 0; // m_NetChannel->GetCommandInterpolationAmount( i );
		ci->sent =	netchannel->IsValidPacket( FLOW_OUTGOING, seqnr );
		ci->size =	netchannel->GetPacketBytes( FLOW_OUTGOING, seqnr, INetChannelInfo::TOTAL);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Draws overlay text fields showing framerate, latency, bandwidth breakdowns, 
//  and, optionally, packet loss and choked packet percentages
// Input  : graphvalue - 
//			x - 
//			y - 
//			*graph - 
//			*cmdinfo - 
//			count - 
//			avg - 
//			*framerate - 
//			0.0 - 
//			avg - 
//-----------------------------------------------------------------------------
void CNetGraphPanel::DrawTextFields( int graphvalue, int x, int y, netbandwidthgraph_t *graph, cmdinfo_t *cmdinfo )
{
	static int lastout;

	char sz[ 256 ];
	int out;

	// Move rolling average
	m_Framerate = FRAMERATE_AVG_FRAC * m_Framerate + ( 1.0 - FRAMERATE_AVG_FRAC ) * gpGlobals->absoluteframetime;

	// Print it out
	y -= net_graphheight.GetInt();

	if ( m_Framerate <= 0.0f )
		m_Framerate = 1.0f;

	if ( engine->IsPlayingDemo() )
		m_AvgLatency = 0.0f;

	int textWide, textTall;
	g_pMatSystemSurface->GetTextSize( m_hFont, L"text", textWide, textTall );
	
	Q_snprintf( sz, sizeof( sz ), "fps: %3i   ping: %i ms", (int)(1.0f / m_Framerate), (int)(m_AvgLatency*1000.0f) );
	
	g_pMatSystemSurface->DrawColoredText( m_hFont, x, y, GRAPH_RED, GRAPH_GREEN, GRAPH_BLUE, 255, sz );

	y += textTall;

	out = cmdinfo[ ( ( m_OutgoingSequence - 1 ) & ( TIMINGS - 1 ) ) ].size;
	if ( !out )
	{
		out = lastout;
	}
	else
	{
		lastout = out;
	}

	int totalsize = graph[ ( m_IncomingSequence & ( TIMINGS - 1 ) ) ].msgbytes[INetChannelInfo::TOTAL];
	
	Q_snprintf( sz, sizeof( sz ), "in :%4i   %2.2f k/s  %3.1f/s", totalsize, m_IncomingData, m_AvgPacketIn );

	g_pMatSystemSurface->DrawColoredText( m_hFont, x, y, GRAPH_RED, GRAPH_GREEN, GRAPH_BLUE, 255, sz );
	y += textTall;

	Q_snprintf( sz, sizeof( sz ), "out:%4i   %2.2f k/s  %3.1f/s", out, m_OutgoingData, m_AvgPacketOut );

	g_pMatSystemSurface->DrawColoredText( m_hFont, x, y, GRAPH_RED, GRAPH_GREEN, GRAPH_BLUE, 255, sz );
	y += textTall;

	if ( graphvalue > 2 )
	{
		Q_snprintf( sz, sizeof( sz ), "loss:%3i   choke: %i", (int)(m_AvgPacketLoss*100.0f), (int)(m_AvgPacketChoke*100.0f) );
		g_pMatSystemSurface->DrawColoredText( m_hFont, x, y, GRAPH_RED, GRAPH_GREEN, GRAPH_BLUE, 255, sz );
		y += textTall;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Determine type of graph to show, or if +graph key is held down, use detailed graph
// Output : int
//-----------------------------------------------------------------------------
int CNetGraphPanel::GraphValue( void )
{
	int graphtype;

	graphtype = net_graph.GetInt();
	
	if ( !graphtype && !( in_graph.state & 1 ) )
		return 0;

	// With +graph key, use max area
	if ( !graphtype )
	{
		graphtype = 2;
	}

	return graphtype;
}

//-----------------------------------------------------------------------------
// Purpose: Figure out x and y position for graph based on net_graphpos
//   value.
// Input  : *rect - 
//			width - 
//			*x - 
//			*y - 
//-----------------------------------------------------------------------------
void CNetGraphPanel::GraphGetXY( vrect_t *rect, int width, int *x, int *y )
{
	*x = rect->x + 5;

	switch ( net_graphpos.GetInt() )
	{
	case 0:
		break;
	case 1:
		*x = rect->x + rect->width - 5 - width;
		break;
	case 2:
		*x = rect->x + ( rect->width - 10 - width ) / 2;
		break;
	default:
		*x = rect->x + clamp( XRES( net_graphpos.GetInt() ), 5, rect->width - width - 5 );
	}

	*y = rect->y+rect->height - LERP_HEIGHT - 5;
}

//-----------------------------------------------------------------------------
// Purpose: drawing stream progess (file download etc) as green bars ( under in/out)
// Input  : x - 
//			y - 
//			maxmsgbytes - 
//-----------------------------------------------------------------------------

void CNetGraphPanel::DrawStreamProgress( int x, int y, int width )
{
	vrect_t rcLine;

	rcLine.height	= 1;
	rcLine.x		= x;
	
	byte color[3]; color[0] = 0; color[1] = 200; color[2] = 0;

	if ( m_StreamTotal[FLOW_INCOMING] > 0 )
	{
		rcLine.y = y - net_graphheight.GetInt() + 15 + 14;
		rcLine.width = (m_StreamRecv[FLOW_INCOMING]*width)/m_StreamTotal[FLOW_INCOMING];
		DrawLine( &rcLine, color, 255 );
	}

	if ( m_StreamTotal[FLOW_OUTGOING] > 0 )
	{
		rcLine.y = y - net_graphheight.GetInt() + 2*15 + 14;
		rcLine.width = (m_StreamRecv[FLOW_OUTGOING]*width)/m_StreamTotal[FLOW_OUTGOING];
		DrawLine( &rcLine, color, 255 );
	}
}


//-----------------------------------------------------------------------------
// Purpose: If showing bandwidth data, draw hatches big enough for largest message
// Input  : x - 
//			y - 
//			maxmsgbytes - 
//-----------------------------------------------------------------------------
void CNetGraphPanel::DrawHatches( int x, int y, int maxmsgbytes )
{
	int starty;
	int ystep;
	vrect_t rcHatch;

	byte colorminor[3];
	byte color[3];

	ystep = (int)( 10.0 / net_scale.GetFloat() );
	ystep = max( ystep, 1 );

	rcHatch.y		= y;
	rcHatch.height	= 1;
	rcHatch.x		= x;
	rcHatch.width	= 4;

	color[0] = 0;
	color[1] = 200;
	color[2] = 0;

	colorminor[0] = 63;
	colorminor[1] = 63;
	colorminor[2] = 0;

	for ( starty = rcHatch.y; rcHatch.y > 0 && ((starty - rcHatch.y)*net_scale.GetFloat() < ( maxmsgbytes + 50 ) ); rcHatch.y -= ystep )
	{
		if ( !((int)((starty - rcHatch.y)*net_scale.GetFloat() ) % 50 ) )
		{
			DrawLine( &rcHatch, color, 255 );
		}
		else if ( ystep > 5 )
		{
			DrawLine( &rcHatch, colorminor, 200 );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: State how many updates a second are being requested
// Input  : x - 
//			y - 
//-----------------------------------------------------------------------------
void CNetGraphPanel::DrawUpdateRate( int x, int y )
{
	// Last one
	g_pMatSystemSurface->DrawColoredText( m_hFont, x, y, GRAPH_RED, GRAPH_GREEN, GRAPH_BLUE, 255, "%i/s", cl_updaterate->GetInt() );
}

//-----------------------------------------------------------------------------
// Purpose: Draws bandwidth breakdown data
// Input  : *rcFill - 
//			bytes - 
//			r - 
//			g - 
//			b - 
//			alpha - 
// Output : int
//-----------------------------------------------------------------------------
int CNetGraphPanel::DrawDataSegment( vrect_t *rcFill, int bytes, byte r, byte g, byte b, byte alpha )
{
	int h;
	byte color[3];

	h = bytes / net_scale.GetFloat();
	
	color[0] = r;
	color[1] = g;
	color[2] = b;

	rcFill->height = h;
	rcFill->y -= h;

	if ( rcFill->y < 2 )
		return 0;

	DrawLine( rcFill, color, alpha );

	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNetGraphPanel::OnTick( void )
{
	SetVisible( ShouldDraw() );
}

bool CNetGraphPanel::ShouldDraw( void )
{
	if ( GraphValue() != 0 )
		return true;

	return false;
}

void CNetGraphPanel::DrawLargePacketSizes( int x, int w, int graphtype, float warning_threshold )
{
	byte		color[3];
	byte		alpha;
	vrect_t		rcFill = {0,0,0,0};
	int a, i;
	int			ping;

	if ( graphtype >= 2 )
	{
		for (a=0 ; a<w ; a++)
		{
			i = (m_IncomingSequence-a) & ( TIMINGS - 1 );
			
			ColorForHeight( &m_PacketLatency[i], color, &ping, &alpha );

			rcFill.x			= x + w -a -1;
			rcFill.width		= 1;
			rcFill.y			= m_Graph[i].sampleY;
			rcFill.height		= m_Graph[i].sampleHeight;

			if ( m_Graph[i].msgbytes[INetChannelInfo::TOTAL] > 300 &&
				warning_threshold != 0.0f &&
				m_Graph[i].msgbytes[INetChannelInfo::TOTAL] >= warning_threshold && 
				( rcFill.y >= 12 ) )
			{
				char sz[ 32 ];
				Q_snprintf( sz, sizeof( sz ), "%i", m_Graph[i].msgbytes[INetChannelInfo::TOTAL] );

				int len = g_pMatSystemSurface->DrawTextLen( m_hFont, sz );

				int textx, texty;

				textx = rcFill.x - len / 2;
				texty = rcFill.y - 11;

				g_pMatSystemSurface->DrawColoredText( m_hFont, textx, texty, 255, 255, 255, 255, sz );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNetGraphPanel::Paint() 
{
	VPROF( "CNetGraphPanel::Paint" );

	int			graphtype;

	int			x, y;
	int			w;
	vrect_t		vrect;

	int			maxmsgbytes = 0;

	float		avg_message = 0.0f;
	float		warning_threshold = 0.0f;

	if ( ( graphtype = GraphValue() ) == 0 )
		return;

	// Since we divide by scale, make sure it's sensible
	if ( net_scale.GetFloat() <= 0 )
	{
		net_scale.SetValue( 0.1f );
	}

	int sw, sh;
	vgui::surface()->GetScreenSize( sw, sh );

	// Get screen rectangle
	vrect.x			= 0;
	vrect.y			= 0;
	vrect.width		= sw;
	vrect.height	= sh;


	w = min( (int)TIMINGS, m_EstimatedWidth );
	if ( vrect.width < w + 10 )
	{
		w = vrect.width - 10;
	}

	// get current client netchannel INetChannelInfo interface
	INetChannelInfo *nci = engine->GetNetChannelInfo();

	if ( nci )
	{
		// update incoming data
		GetFrameData( nci, &maxmsgbytes, &avg_message, &warning_threshold );

		// update outgoing data
		GetCommandInfo( nci, m_Cmdinfo );

	}

	GraphGetXY( &vrect, w, &x, &y );

	if ( graphtype < 3 )
	{
		PaintLineArt( x, y, w, graphtype, maxmsgbytes );

		DrawLargePacketSizes( x, w, graphtype, warning_threshold );

		// Draw client frame timing info
		DrawTimes( vrect, m_Cmdinfo, x, w );

		// Draw update rate
		DrawUpdateRate( max( 1, x + w - 25 ), max( 1, y - net_graphheight.GetFloat() - 1 ) );
	}

	DrawTextFields( graphtype, x, y, m_Graph, m_Cmdinfo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNetGraphPanel::PaintLineArt( int x, int y, int w, int graphtype, int maxmsgbytes ) 
{
	VPROF( "CNetGraphPanel::PaintLineArt" );

	ResetLineSegments();

	int lastvalidh = 0;

	byte		color[3];
	int			ping;
	byte		alpha;
	vrect_t		rcFill = {0,0,0,0};

	for (int a=0 ; a<w ; a++)
	{
		int i = (m_IncomingSequence-a) & ( TIMINGS - 1 );
		int h = m_PacketLatency[i].latency;
		
		ColorForHeight( &m_PacketLatency[i], color, &ping, &alpha );

		// Skipped
		if ( !ping ) 
		{
			// Re-use the last latency
			h = lastvalidh;  
		}
		else
		{
			lastvalidh = h;
		}

		if (h > ( net_graphheight.GetFloat() -  LERP_HEIGHT - 2 ) )
		{
			h = net_graphheight.GetFloat() - LERP_HEIGHT - 2;
		}

		rcFill.x		= x + w -a -1;
		rcFill.y		= y - h;
		rcFill.width	= 1;
		rcFill.height	= ping ? 1 : h;

		DrawLine(&rcFill, color, alpha );		

		rcFill.y		= y;
		rcFill.height	= 1;

		color[0] = 0;
		color[1] = 255;
		color[2] = 0;

		DrawLine( &rcFill, color, 160 );

		if ( graphtype < 2 )
			continue;

		// Draw a separator.
		rcFill.y = y - net_graphheight.GetFloat() - 1;
		rcFill.height = 1;

		color[0] = 255;
		color[1] = 255;
		color[2] = 255;

		DrawLine(&rcFill, color, 255 );		

		// Move up for begining of data
		rcFill.y -= 1;

		// Packet didn't have any real data...
		if ( m_PacketLatency[i].latency > 9995 )
			continue;


		if ( !DrawDataSegment( &rcFill, m_Graph[ i ].msgbytes[INetChannelInfo::LOCALPLAYER], 0, 0, 255 ) )
			continue;

		if ( !DrawDataSegment( &rcFill, m_Graph[ i ].msgbytes[INetChannelInfo::OTHERPLAYERS], 0, 255, 0 ) )
			continue;

		if ( !DrawDataSegment( &rcFill, m_Graph[ i ].msgbytes[INetChannelInfo::ENTITIES], 255, 0, 0 ) )
			continue;

		if ( !DrawDataSegment( &rcFill, m_Graph[ i ].msgbytes[INetChannelInfo::SOUNDS], 255, 255, 0) )
			continue;

		if ( !DrawDataSegment( &rcFill, m_Graph[ i ].msgbytes[INetChannelInfo::EVENTS], 0, 255, 255 ) )
			continue;
		
		if ( !DrawDataSegment( &rcFill, m_Graph[ i ].msgbytes[INetChannelInfo::USERMESSAGES], 128, 128, 0 ) )
			continue;

		if ( !DrawDataSegment( &rcFill, m_Graph[ i ].msgbytes[INetChannelInfo::ENTMESSAGES], 0, 128, 128 ) )
			continue;

		if ( !DrawDataSegment( &rcFill, m_Graph[ i ].msgbytes[INetChannelInfo::STRINGCMD], 128, 0, 0) )
			continue;

		if ( !DrawDataSegment( &rcFill, m_Graph[ i ].msgbytes[INetChannelInfo::STRINGTABLE], 0, 128, 0) )
			continue;

		if ( !DrawDataSegment( &rcFill, m_Graph[ i ].msgbytes[INetChannelInfo::VOICE], 0, 0, 128  ) )
			continue;

		// Final data chunk is total size, don't use solid line routine for this
		h = m_Graph[i].msgbytes[INetChannelInfo::TOTAL] / net_scale.GetFloat();

		color[ 0 ] = color[ 1 ] = color[ 2 ] = 240;

		rcFill.height = 1;
		rcFill.y = y - net_graphheight.GetFloat() - 1 - h;

		if ( rcFill.y < 2 )
			continue;

		DrawLine(&rcFill, color, 128 );		

		// Cache off height
		m_Graph[i].sampleY = rcFill.y;
		m_Graph[i].sampleHeight = rcFill.height;
	}

	if ( graphtype >= 2 )
	{
		// Draw hatches for first one:
		// on the far right side
		DrawHatches( x, y - net_graphheight.GetFloat() - 1, maxmsgbytes );
		
		DrawStreamProgress( x, y, w );
	}

	DrawLineSegments();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNetGraphPanel::ResetLineSegments()
{
	m_Rects.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNetGraphPanel::DrawLineSegments()
{
	int c = m_Rects.Count();
	if ( c <= 0 )
		return;

	IMesh* m_pMesh = materials->GetDynamicMesh( true, NULL, NULL, m_WhiteMaterial );
	CMeshBuilder		meshBuilder;
	meshBuilder.Begin( m_pMesh, MATERIAL_LINES, c );

	int i;
	for ( i = 0 ; i < c; i++ )
	{
		CLineSegment *seg = &m_Rects[ i ];

		meshBuilder.Color4ubv( seg->color );
		meshBuilder.TexCoord2f( 0, 0.0f, 0.0f );
		meshBuilder.Position3f( seg->x1, seg->y1, 0 );
		meshBuilder.AdvanceVertex();

		meshBuilder.Color4ubv( seg->color );
		meshBuilder.TexCoord2f( 0, 0.0f, 0.0f );
		meshBuilder.Position3f( seg->x2, seg->y2, 0 );
		meshBuilder.AdvanceVertex();
	}

	meshBuilder.End();

	m_pMesh->Draw();
}

//-----------------------------------------------------------------------------
// Purpose: Draws a colored, filled rectangle
// Input  : *rect - 
//			*color - 
//			alpha - 
//-----------------------------------------------------------------------------
void CNetGraphPanel::DrawLine( vrect_t *rect, unsigned char *color, unsigned char alpha )
{
	VPROF( "CNetGraphPanel::DrawLine" );

	int idx = m_Rects.AddToTail();
	CLineSegment *seg = &m_Rects[ idx ];

	seg->color[0] = color[0];
	seg->color[1] = color[1];
	seg->color[2] = color[2];
	seg->color[3] = alpha;

	if ( rect->width == 1 )
	{
		seg->x1 = rect->x;
		seg->y1 = rect->y;
		seg->x2 = rect->x;
		seg->y2 = rect->y + rect->height;
	}
	else if ( rect->height == 1 )
	{
		seg->x1 = rect->x;
		seg->y1 = rect->y;
		seg->x2 = rect->x + rect->width;
		seg->y2 = rect->y;
	}
	else
	{
		Assert( 0 );
		m_Rects.Remove( idx );
	}
}

class CNetGraphPanelInterface : public INetGraphPanel
{
private:
	CNetGraphPanel *netGraphPanel;
public:
	CNetGraphPanelInterface( void )
	{
		netGraphPanel = NULL;
	}
	void Create( vgui::VPANEL parent )
	{
		netGraphPanel = new CNetGraphPanel( parent );
	}
	void Destroy( void )
	{
		if ( netGraphPanel )
		{
			netGraphPanel->SetParent( (vgui::Panel *)NULL );
			delete netGraphPanel;
		}
	}
};

static CNetGraphPanelInterface g_NetGraphPanel;
INetGraphPanel *netgraphpanel = ( INetGraphPanel * )&g_NetGraphPanel;
