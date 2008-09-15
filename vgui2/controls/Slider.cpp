//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include <stdio.h>
#define PROTECTED_THINGS_DISABLE

#include <vgui/MouseCode.h>
#include <KeyValues.h>
#include <vgui/IBorder.h>
#include <vgui/IInput.h>
#include <vgui/ISystem.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>

#include <vgui_controls/Slider.h>
#include <vgui_controls/Controls.h>
#include <vgui_controls/TextImage.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

static const float NOB_SIZE = 8.0f;

//-----------------------------------------------------------------------------
// Purpose: Create a slider bar with ticks underneath it
//-----------------------------------------------------------------------------
Slider::Slider(Panel *parent, const char *panelName ) : Panel(parent, panelName)
{
	m_bIsDragOnRepositionNob = false;
	_dragging = false;
	_value = 0;
	_range[0] = 0;
	_range[1] = 0;
	_buttonOffset = 0;
	_sliderBorder = NULL;
	_insetBorder = NULL;
	m_nNumTicks = 10;
	_leftCaption = NULL;
	_rightCaption = NULL;

	SetThumbWidth( 8 );
	RecomputeNobPosFromValue();
	AddActionSignalTarget(this);
	SetBlockDragChaining( true );
}

//-----------------------------------------------------------------------------
// Purpose: Set the size of the slider bar.
//			Warning less than 30 pixels tall and everything probably won't fit.
//-----------------------------------------------------------------------------
void Slider::OnSizeChanged(int wide,int tall)
{
	BaseClass::OnSizeChanged(wide,tall);

	RecomputeNobPosFromValue();
}

//-----------------------------------------------------------------------------
// Purpose: Set the value of the slider to one of the ticks.
//-----------------------------------------------------------------------------
void Slider::SetValue(int value, bool bTriggerChangeMessage)
{
	int oldValue=_value;

	if(value<_range[0])
	{
		value=_range[0];
	}

	if(value>_range[1])
	{
		value=_range[1];	
	}

	_value = value;
	
	RecomputeNobPosFromValue();

	if (_value != oldValue && bTriggerChangeMessage)
	{
		SendSliderMovedMessage();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Return the value of the slider
//-----------------------------------------------------------------------------
int Slider::GetValue()
{
	return _value;
}

//-----------------------------------------------------------------------------
// Purpose: Layout the slider before drawing it on screen.
//-----------------------------------------------------------------------------
void Slider::PerformLayout()
{
	BaseClass::PerformLayout();
	RecomputeNobPosFromValue();

	if (_leftCaption)
	{
		_leftCaption->ResizeImageToContent();
	}
	if (_rightCaption)
	{
		_rightCaption->ResizeImageToContent();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Move the nob on the slider in response to changing its value.
//-----------------------------------------------------------------------------
void Slider::RecomputeNobPosFromValue()
{
	//int wide,tall;
	//GetPaintSize(wide,tall);
	int x, y, wide, tall;
	GetTrackRect( x, y, wide, tall );

	float fwide=(float)wide;
	float frange=(float)(_range[1] -_range[0]);
	float fvalue=(float)(_value -_range[0]);
	float fper = (frange != 0.0f) ? fvalue / frange : 0.0f;

	float freepixels = fwide - _nobSize;
	float leftpixel = (float)x;
	float firstpixel = leftpixel + freepixels * fper + 0.5f;

	_nobPos[0]=(int)( firstpixel );
	_nobPos[1]=(int)( firstpixel + _nobSize );


	int rightEdge = x + wide;

	if(_nobPos[1]> rightEdge )
	{
		_nobPos[0]=rightEdge-((int)_nobSize);
		_nobPos[1]=rightEdge;
	}
	
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Sync the slider's value up with the nob's position.
//-----------------------------------------------------------------------------
void Slider::RecomputeValueFromNobPos()
{
//	int wide, tall;
//	GetPaintSize(wide, tall);
	int x, y, wide, tall;
	GetTrackRect( x, y, wide, tall );

	float fwide = (float)wide;
	float frange = (float)( _range[1] - _range[0] );
	float fvalue = (float)( _value - _range[0] );
	float fnob = (float)( _nobPos[0] - x );


	float freepixels = fwide - _nobSize;

	// Map into reduced range
	fvalue = freepixels != 0.0f ? fnob / freepixels : 0.0f;

	// Scale by true range
	fvalue *= frange;

	// Take care of rounding issues.
	SetValue( (int)( fvalue + _range[0] + 0.5) );
}

//-----------------------------------------------------------------------------
// Purpose: Send a message to interested parties when the slider moves
//-----------------------------------------------------------------------------
void Slider::SendSliderMovedMessage()
{	
	// send a changed message
	PostActionSignal(new KeyValues("SliderMoved", "position", _value));
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Slider::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	SetFgColor(GetSchemeColor("Slider.NobColor", pScheme));
	// this line is useful for debugging
	//SetBgColor(GetSchemeColor("0 0 0 255"));

	m_TickColor = pScheme->GetColor( "Slider.TextColor", GetFgColor() );
	m_TrackColor = pScheme->GetColor( "Slider.TrackColor", GetFgColor() );

	m_DisabledTextColor1 = pScheme->GetColor( "Slider.DisabledTextColor1", GetFgColor() );
	m_DisabledTextColor2 = pScheme->GetColor( "Slider.DisabledTextColor2", GetFgColor() );

	_sliderBorder = pScheme->GetBorder("ButtonBorder");
	_insetBorder = pScheme->GetBorder("ButtonDepressedBorder");

	if ( _leftCaption )
	{
		_leftCaption->SetFont(pScheme->GetFont("DefaultVerySmall", IsProportional() ));
	}

	if ( _rightCaption )
	{
		_rightCaption->SetFont(pScheme->GetFont("DefaultVerySmall", IsProportional() ));
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Slider::GetSettings(KeyValues *outResourceData)
{
	BaseClass::GetSettings(outResourceData);
	
	char buf[256];
	if (_leftCaption)
	{
		_leftCaption->GetUnlocalizedText(buf, sizeof(buf));
		outResourceData->SetString("leftText", buf);
	}
	
	if (_rightCaption)
	{
		_rightCaption->GetUnlocalizedText(buf, sizeof(buf));
		outResourceData->SetString("rightText", buf);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Slider::ApplySettings(KeyValues *inResourceData)
{
	BaseClass::ApplySettings(inResourceData);

	const char *left = inResourceData->GetString("leftText", NULL);
	const char *right = inResourceData->GetString("rightText", NULL);

	int thumbWidth = inResourceData->GetInt("thumbwidth", 0);
	if (thumbWidth != 0)
	{
		SetThumbWidth(thumbWidth);
	}

	SetTickCaptions(left, right);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *Slider::GetDescription()
{
	static char buf[1024];
	Q_snprintf(buf, sizeof(buf), "%s, string leftText, string rightText", BaseClass::GetDescription());
	return buf;
}

//-----------------------------------------------------------------------------
// Purpose: Get the rectangle to draw the slider track in.
//-----------------------------------------------------------------------------
void Slider::GetTrackRect( int& x, int& y, int& w, int& h )
{
	int wide, tall;
	GetPaintSize( wide, tall );

	x = 0;
	y = 8;
	w = wide - (int)_nobSize;
	h = 4;
}

//-----------------------------------------------------------------------------
// Purpose: Draw everything on screen
//-----------------------------------------------------------------------------
void Slider::Paint()
{
	DrawTicks();

	DrawTickLabels();

	// Draw nob last so it draws over ticks.
	DrawNob();
}

//-----------------------------------------------------------------------------
// Purpose: Draw the ticks below the slider.
//-----------------------------------------------------------------------------
void Slider::DrawTicks()
{
	int x, y;
	int wide,tall;
	GetTrackRect( x, y, wide, tall );

	// Figure out how to draw the ticks
//	GetPaintSize( wide, tall );

	float fwide  = (float)wide;
	float freepixels = fwide - _nobSize;

	float leftpixel = _nobSize / 2.0f;

	float pixelspertick = freepixels / ( m_nNumTicks );

	y += (int)_nobSize;
	int tickHeight = 5;

    if (IsEnabled())
    {
        surface()->DrawSetColor( m_TickColor ); //vgui::Color( 127, 140, 127, 255 ) );
    	for ( int i = 0; i <= m_nNumTicks; i++ )
    	{
    		int xpos = (int)( leftpixel + i * pixelspertick );
    
    		surface()->DrawFilledRect( xpos, y, xpos + 1, y + tickHeight );
    	}
    }
    else
    {
        surface()->DrawSetColor( m_DisabledTextColor1 ); //vgui::Color( 127, 140, 127, 255 ) );
    	for ( int i = 0; i <= m_nNumTicks; i++ )
    	{
    		int xpos = (int)( leftpixel + i * pixelspertick );
    		surface()->DrawFilledRect( xpos+1, y+1, xpos + 2, y + tickHeight + 1 );
    	}
        surface()->DrawSetColor( m_DisabledTextColor2 ); //vgui::Color( 127, 140, 127, 255 ) );
    	for ( i = 0; i <= m_nNumTicks; i++ )
    	{
    		int xpos = (int)( leftpixel + i * pixelspertick );
    		surface()->DrawFilledRect( xpos, y, xpos + 1, y + tickHeight );
    	}
    }
}

//-----------------------------------------------------------------------------
// Purpose: Draw Tick labels under the ticks.
//-----------------------------------------------------------------------------
void Slider::DrawTickLabels()
{
	int x, y;
	int wide,tall;
	GetTrackRect( x, y, wide, tall );

	// Figure out how to draw the ticks
//	GetPaintSize( wide, tall );
	y += (int)NOB_SIZE + 4;

	// Draw Start and end range values
    if (IsEnabled())
	    surface()->DrawSetTextColor( m_TickColor ); //vgui::Color( 127, 140, 127, 255 ) );
    else
	    surface()->DrawSetTextColor( m_DisabledTextColor1 ); //vgui::Color( 127, 140, 127, 255 ) );


	if ( _leftCaption != NULL )
	{
		_leftCaption->SetPos(0, y);
        if (IsEnabled())
		{
		    _leftCaption->SetColor( m_TickColor ); 
		}
        else
		{
		    _leftCaption->SetColor( m_DisabledTextColor1 ); 
		}

		_leftCaption->Paint();
	}

	if ( _rightCaption != NULL)
	{
		int rwide, rtall;
		_rightCaption->GetSize(rwide, rtall);
		_rightCaption->SetPos((int)(wide - rwide) , y);
        if (IsEnabled())
		{
    		_rightCaption->SetColor( m_TickColor );
		}
		else
		{
    		_rightCaption->SetColor( m_DisabledTextColor1 );
		}

		_rightCaption->Paint();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Draw the nob part of the slider.
//-----------------------------------------------------------------------------
void Slider::DrawNob()
{
	// horizontal nob
	int x, y;
	int wide,tall;
	GetTrackRect( x, y, wide, tall );
	Color col = GetFgColor();
	surface()->DrawSetColor(col);

	int nobheight = 16;

	surface()->DrawFilledRect(
		_nobPos[0], 
		y + tall / 2 - nobheight / 2, 
		_nobPos[1], 
		y + tall / 2 + nobheight / 2);
	// border
	if (_sliderBorder)
	{
		_sliderBorder->Paint(
			_nobPos[0], 
			y + tall / 2 - nobheight / 2, 
			_nobPos[1], 
			y + tall / 2 + nobheight / 2);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set the text labels of the Start and end ticks.
//-----------------------------------------------------------------------------
void Slider::SetTickCaptions( const char *left, const char *right )
{
	if (left)
	{
		if (_leftCaption)
		{
			_leftCaption->SetText(left);
		}
		else
		{
			_leftCaption = new TextImage(left);
		}
	}
	if (right)
	{
		if (_rightCaption)
		{
			_rightCaption->SetText(right);
		}
		else
		{
			_rightCaption = new TextImage(right);
		}
	}
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Set the text labels of the Start and end ticks.
//-----------------------------------------------------------------------------
void Slider::SetTickCaptions( const wchar_t *left, const wchar_t *right )
{
	if (left)
	{
		if (_leftCaption)
		{
			_leftCaption->SetText(left);
		}
		else
		{
			_leftCaption = new TextImage(left);
		}
	}
	if (right)
	{
		if (_rightCaption)
		{
			_rightCaption->SetText(right);
		}
		else
		{
			_rightCaption = new TextImage(right);
		}
	}
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Draw the slider track
//-----------------------------------------------------------------------------
void Slider::PaintBackground()
{
	BaseClass::PaintBackground();
	
	int x, y;
	int wide,tall;

	GetTrackRect( x, y, wide, tall );

	surface()->DrawSetColor( m_TrackColor ); 
	surface()->DrawFilledRect( x, y, x + wide, y + tall );
	if (_insetBorder)
	{
		_insetBorder->Paint( x, y, x + wide, y + tall );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set the range of the slider.
//-----------------------------------------------------------------------------
void Slider::SetRange(int min,int max)
{
	if(max<min)
	{
		max=min;
	}

	if(min>max)
	{
		min=max;
	}

	_range[0]=min;
	_range[1]=max;

	if(_value<_range[0])
	{
		SetValue( _range[0] );
	}
	else if( _value>_range[1])
	{
		SetValue( _range[0] );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Get the max and min values of the slider
//-----------------------------------------------------------------------------
void Slider::GetRange(int& min,int& max)
{
	min=_range[0];
	max=_range[1];
}

//-----------------------------------------------------------------------------
// Purpose: Respond when the cursor is moved in our window if we are clicking
// and dragging.
//-----------------------------------------------------------------------------
void Slider::OnCursorMoved(int x,int y)
{
	if(!_dragging)
	{
		return;
	}

//	input()->GetCursorPos(x,y);
	input()->GetCursorPosition( x, y );
	ScreenToLocal(x,y);

//	int wide,tall;
//	GetPaintSize(wide,tall);
	int _x, _y, wide, tall;
	GetTrackRect( _x, _y, wide, tall );

	_nobPos[0]=_nobDragStartPos[0]+(x-_dragStartPos[0]);
	_nobPos[1]=_nobDragStartPos[1]+(x-_dragStartPos[0]);

	int rightEdge = _x +wide;

	if(_nobPos[1]>rightEdge)
	{
		_nobPos[0]=rightEdge-(_nobPos[1]-_nobPos[0]);
		_nobPos[1]=rightEdge;
	}
		
	if(_nobPos[0]<_x)
	{
		int offset = _x - _nobPos[0];
		_nobPos[1]=_nobPos[1]-offset;
		_nobPos[0]=0;
	}

	RecomputeValueFromNobPos();
	Repaint();
	SendSliderMovedMessage();
}

//-----------------------------------------------------------------------------
// Purpose: If you click on the slider outside of the nob, the nob jumps
// to the click position, and if this setting is enabled, the nob
// is then draggable from the new position until the mouse is released
// Input  : state - 
//-----------------------------------------------------------------------------
void Slider::SetDragOnRepositionNob( bool state )
{
	m_bIsDragOnRepositionNob = state;
}

bool Slider::IsDragOnRepositionNob() const
{
	return m_bIsDragOnRepositionNob;
}

//-----------------------------------------------------------------------------
// Purpose: Respond to mouse presses. Trigger Record staring positon.
//-----------------------------------------------------------------------------
void Slider::OnMousePressed(MouseCode code)
{
	int x,y;

    if (!IsEnabled())
        return;

//	input()->GetCursorPos(x,y);
	input()->GetCursorPosition( x, y );

	ScreenToLocal(x,y);
    RequestFocus();

	bool startdragging = false;

	if ((x >= _nobPos[0]) && (x < _nobPos[1]))
	{
		startdragging = true;
	}
	else
	{
		// we clicked elsewhere on the slider; move the nob to that position
		int min, max;
		GetRange(min, max);

//		int wide = GetWide();
		int _x, _y, wide, tall;
		GetTrackRect( _x, _y, wide, tall );
		if ( wide > 0 )
		{
			float frange = ( float )( max - min );
			float clickFrac = clamp( ( float )( x - _x ) / (float)( wide - 1 ), 0.0f, 1.0f );

			float value = (float)min + clickFrac * frange;

			SetValue( ( int )( value + 0.5f ) );

			startdragging = IsDragOnRepositionNob();
		}
	}

	if ( startdragging )
	{
		// drag the nob
		_dragging = true;
		input()->SetMouseCapture(GetVPanel());
		_nobDragStartPos[0] = _nobPos[0];
		_nobDragStartPos[1] = _nobPos[1];
		_dragStartPos[0] = x;
		_dragStartPos[1] = y;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Just handle double presses like mouse presses
//-----------------------------------------------------------------------------
void Slider::OnMouseDoublePressed(MouseCode code)
{
	OnMousePressed(code);
}

//-----------------------------------------------------------------------------
// Purpose: Handle key presses
//-----------------------------------------------------------------------------
void Slider::OnKeyCodeTyped(KeyCode code)
{
	switch (code)
	{
		// for now left and right arrows just open or close submenus if they are there.
        case KEY_LEFT:
        case KEY_DOWN:
            {
                int val = GetValue();
                SetValue(val-1);
                break;
            }
    	case KEY_RIGHT:
        case KEY_UP:
    		{
                int val = GetValue();
                SetValue(val+1);
    			break;
    		}
        case KEY_PAGEDOWN:
            {
                int min, max;
                GetRange(min, max);
                float range = (float) max-min;
                float pertick = range/m_nNumTicks;
                int val = GetValue();
                SetValue(val - (int) pertick);
                break;
            }
        case KEY_PAGEUP:
            {
                int min, max;
                GetRange(min, max);
                float range = (float) max-min;
                float pertick = range/m_nNumTicks;
                int val = GetValue();
                SetValue(val + (int) pertick);
                break;
            }
        case KEY_HOME:
            {
                int min, max;
                GetRange(min, max);
                SetValue(min);
    			break;
            }
        case KEY_END:
            {
                int min, max;
                GetRange(min, max);
                SetValue(max);
    			break;
            }
    	default:
    		BaseClass::OnKeyCodeTyped(code);
    		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Stop dragging when the mouse is released.
//-----------------------------------------------------------------------------
void Slider::OnMouseReleased(MouseCode code)
{
	_dragging=false;
	input()->SetMouseCapture(null);
}

//-----------------------------------------------------------------------------
// Purpose: Get the nob's position (the ends of each side of the nob)
//-----------------------------------------------------------------------------
void Slider::GetNobPos(int& min, int& max)
{
	min=_nobPos[0];
	max=_nobPos[1];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Slider::SetButtonOffset(int buttonOffset)
{
	_buttonOffset=buttonOffset;
}

void Slider::SetThumbWidth( int width )
{
	_nobSize = (float)width;
}


//-----------------------------------------------------------------------------
// Purpose: Set the number of ticks that appear under the slider.
//-----------------------------------------------------------------------------
void Slider::SetNumTicks( int ticks )
{
	m_nNumTicks = ticks;
}
