//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "vgui_controls/pch_vgui_controls.h"

// memdbgon must be the last include file in a .cpp file
#include "tier0/memdbgon.h"

enum
{
	MAX_BUFFER_SIZE = 999999,	// maximum size of text buffer
	DRAW_OFFSET_X =	3,
	DRAW_OFFSET_Y =	1,
};

using namespace vgui;

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

namespace vgui
{
	
//-----------------------------------------------------------------------------
// Purpose: Panel used for clickable URL's
//-----------------------------------------------------------------------------
class ClickPanel : public Panel
{
	DECLARE_CLASS_SIMPLE( ClickPanel, Panel );

public:
	ClickPanel(Panel *parent)
	{
		_textIndex = 0;
		SetParent(parent);
		AddActionSignalTarget(parent);
		
		SetCursor(dc_hand);
		
		SetPaintBackgroundEnabled(false);
		SetPaintEnabled(false);
		SetPaintBorderEnabled(false);
	}
	
	void SetTextIndex(int index)
	{
		_textIndex = index;
	}
	
	int GetTextIndex()
	{
		return _textIndex;
	}
	
	void OnMousePressed(MouseCode code)
	{
		if (code == MOUSE_LEFT)
		{
			PostActionSignal(new KeyValues("ClickPanel", "index", _textIndex));
		}
	}

private:
	int _textIndex;
};


//-----------------------------------------------------------------------------
// Purpose: Panel used only to draw the interior border region
//-----------------------------------------------------------------------------
class RichTextInterior : public Panel
{
	DECLARE_CLASS_SIMPLE( RichTextInterior, Panel );

public:
	RichTextInterior( RichText *pParent, const char *pchName ) : BaseClass( pParent, pchName )  
	{
		SetKeyBoardInputEnabled( false );
		SetMouseInputEnabled( false );
		SetPaintBackgroundEnabled( false );
		SetPaintEnabled( false );
		m_pRichText = pParent;
	}

	// bugbug johnc: currently disabled until appearance merge
	/*
	virtual IAppearance *GetAppearance()
	{
		if ( m_pRichText->IsScrollbarVisible() )
			return m_pAppearanceScrollbar;

		return BaseClass::GetAppearance();
	}

	virtual void ApplySchemeSettings( IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings( pScheme );
		m_pAppearanceScrollbar = FindSchemeAppearance( pScheme, "scrollbar_visible" );
	}
	*/

private:
	RichText *m_pRichText;
//	IAppearance *m_pAppearanceScrollbar;    
};
	
};	// namespace vgui

DECLARE_BUILD_FACTORY( RichText );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
RichText::RichText(Panel *parent, const char *panelName) : BaseClass(parent, panelName)
{
	_font = INVALID_FONT;

	m_bRecalcLineBreaks = true;
	m_pszInitialText = NULL;
	_cursorPos = 0;
	_mouseSelection = false;
	_mouseDragSelection = false;
	_vertScrollBar = new ScrollBar(this, "ScrollBar", true);
	_vertScrollBar->AddActionSignalTarget(this);
	_recalcSavedRenderState = true;
	_maxCharCount = (64 * 1024);
	AddActionSignalTarget(this);
	m_pInterior = new RichTextInterior( this, NULL );

	//a -1 for _select[0] means that the selection is empty
	_select[0] = -1;
	_select[1] = -1;
	m_pEditMenu = NULL;
	
	SetCursor(dc_ibeam);
	
	//position the cursor so it is at the end of the text
	GotoTextEnd();
	
	// set default foreground color to black
	_defaultTextColor =  Color(0, 0, 0, 0);
	
	// initialize the line break array
	InvalidateLineBreakStream();

	if ( IsProportional() )
	{
		int width, height;
		int sw,sh;
		surface()->GetProportionalBase( width, height );
		surface()->GetScreenSize(sw, sh);
		
		_drawOffsetX = static_cast<int>( static_cast<float>( DRAW_OFFSET_X )*( static_cast<float>( sw )/ static_cast<float>( width )));
		_drawOffsetY = static_cast<int>( static_cast<float>( DRAW_OFFSET_Y )*( static_cast<float>( sw )/ static_cast<float>( width )));
	}
	else
	{
		_drawOffsetX = DRAW_OFFSET_X;
		_drawOffsetY = DRAW_OFFSET_Y;
	}

	// add a basic format string
	TFormatStream stream;
	stream.color = _defaultTextColor;
	stream.pixelsIndent = 0;
	stream.textStreamIndex = 0;
	stream.textClickable = false;
	m_FormatStream.AddToTail(stream);
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
RichText::~RichText()
{
	delete [] m_pszInitialText;
	delete m_pEditMenu;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void RichText::SetDrawOffsets( int ofsx, int ofsy )
{
	_drawOffsetX = ofsx;
	_drawOffsetY = ofsy;
}

//-----------------------------------------------------------------------------
// Purpose: configures colors
//-----------------------------------------------------------------------------
void RichText::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	
	_font = pScheme->GetFont("Default", IsProportional() );
	
	SetFgColor(GetSchemeColor("RichText.TextColor", pScheme));
	SetBgColor(GetSchemeColor("RichText.BgColor", pScheme));
	
	_selectionTextColor = GetSchemeColor("RichText.SelectedTextColor", GetFgColor(), pScheme);
	_selectionColor = GetSchemeColor("RichText.SelectedBgColor", pScheme);
	
	SetBorder(pScheme->GetBorder("ButtonDepressedBorder"));
}

//-----------------------------------------------------------------------------
// Purpose: if the default format color isn't set then set it
//-----------------------------------------------------------------------------
void RichText::SetFgColor( Color color )
{
	// Replace default format color if 
	// the stream is empty and the color is the default ( or the previous FgColor )
	if ( m_FormatStream.Size() == 1 && 
		( m_FormatStream[0].color == _defaultTextColor || m_FormatStream[0].color == GetFgColor() ) )
	{
		m_FormatStream[0].color = color;
	}
	
	BaseClass::SetFgColor( color );
}

//-----------------------------------------------------------------------------
// Purpose: Sends a message if the data has changed
//          Turns off any selected text in the window if we are not using the edit menu
//-----------------------------------------------------------------------------
void RichText::OnKillFocus()
{
	// check if we clicked the right mouse button or if it is down
	bool mouseRightClicked = input()->WasMousePressed(MOUSE_RIGHT);
	bool mouseRightUp = input()->WasMouseReleased(MOUSE_RIGHT);
	bool mouseRightDown = input()->IsMouseDown(MOUSE_RIGHT);
	
	if (mouseRightClicked || mouseRightDown || mouseRightUp )
	{
		// get the start and ends of the selection area
		int start, end;
		if (GetSelectedRange(start, end)) // we have selected text
		{
			// see if we clicked in the selection area
			int startX, startY;
			CursorToPixelSpace(start, startX, startY);
			int endX, endY;
			CursorToPixelSpace(end, endX, endY);
			int cursorX, cursorY;
			input()->GetCursorPos(cursorX, cursorY);
			ScreenToLocal(cursorX, cursorY);
			
			// check the area vertically
			// we need to handle the horizontal edge cases eventually
			int fontTall = surface()->GetFontTall(_font);
			endY = endY + fontTall;
			if ((startY < cursorY) && (endY > cursorY))
			{
				// if we clicked in the selection area, leave the text highlighted
				return;
			}
		}
	}
	
	// clear any selection
	SelectNone();

	// chain
	BaseClass::OnKillFocus();
}


//-----------------------------------------------------------------------------
// Purpose: Wipe line breaks after the size of a panel has been changed
//-----------------------------------------------------------------------------
void RichText::OnSizeChanged( int wide, int tall )
{
	BaseClass::OnSizeChanged( wide, tall );

   	// blow away the line breaks list 
	_invalidateVerticalScrollbarSlider = true;
	InvalidateLineBreakStream();
	InvalidateLayout();

	if ( _vertScrollBar->IsVisible() )
	{
		_vertScrollBar->MakeReadyForUse();
		m_pInterior->SetBounds( 0, 0, wide - _vertScrollBar->GetWide(), tall );
	}
	else
	{
		m_pInterior->SetBounds( 0, 0, wide, tall );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Set the text array
//          Using this function will cause all lineBreaks to be discarded.
//          This is because this fxn replaces the contents of the text buffer.
//          For modifying large buffers use insert functions.
//-----------------------------------------------------------------------------
void RichText::SetText(const char *text)
{
	if (!text)
	{
		text = "";
	}

	if (text[0] == '#')
	{
		// check for localization
		wchar_t *wsz = localize()->Find(text);
		if (wsz)
		{
			SetText(wsz);
			return;
		}
	}

	// convert to unicode
	wchar_t unicode[1024];
	localize()->ConvertANSIToUnicode(text, unicode, sizeof(unicode));
	SetText(unicode);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void RichText::SetText(const wchar_t *text)
{
	// reset the formatting stream
	m_FormatStream.RemoveAll();
	TFormatStream stream;
	stream.color = GetFgColor();
	stream.pixelsIndent = 0;
	stream.textStreamIndex = 0;
	stream.textClickable = false;
	m_FormatStream.AddToTail(stream);

	// set the new text stream
	m_TextStream.RemoveAll();
	if (text)
	{
		int textLen = wcslen(text);
		m_TextStream.EnsureCapacity(textLen);
		for(int i = 0; i < textLen; i++)
		{
			m_TextStream.AddToTail(text[i]);
		}
	}
	GotoTextStart();
	SelectNone();
	
	// blow away the line breaks list 
	InvalidateLineBreakStream();
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Given cursor's position in the text buffer, convert it to
//			the local window's x and y pixel coordinates
// Input:	cursorPos: cursor index
// Output:	cx, cy, the corresponding coords in the local window
//-----------------------------------------------------------------------------
void RichText::CursorToPixelSpace(int cursorPos, int &cx, int &cy)
{
	int yStart = _drawOffsetY;
	int x = _drawOffsetX, y = yStart;
	_pixelsIndent = 0;
	int lineBreakIndexIndex = 0;
	
	for (int i = GetStartDrawIndex(lineBreakIndexIndex); i < m_TextStream.Count(); i++)
	{
		wchar_t ch = m_TextStream[i];
		
		// if we've found the position, break
		if (cursorPos == i)
		{
			// if we've passed a line break go to that
			if (m_LineBreaks[lineBreakIndexIndex] == i)
			{
				// add another line
				AddAnotherLine(x, y);
				lineBreakIndexIndex++;
			}
			break;
		}
		
		// if we've passed a line break go to that
		if (m_LineBreaks[lineBreakIndexIndex] == i)
		{
			// add another line
			AddAnotherLine(x, y);
			lineBreakIndexIndex++;
		}
		
		// add to the current position
		x += surface()->GetCharacterWidth(_font, ch);
	}
	
	cx = x;
	cy = y;
}

//-----------------------------------------------------------------------------
// Purpose: Converts local pixel coordinates to an index in the text buffer
//-----------------------------------------------------------------------------
int RichText::PixelToCursorSpace(int cx, int cy)
{
	int fontTall = surface()->GetFontTall(_font);
	
	// where to Start reading
	int yStart = _drawOffsetY;
	int x = _drawOffsetX, y = yStart;
	_pixelsIndent = 0;
	int lineBreakIndexIndex = 0;
	
	int startIndex = GetStartDrawIndex(lineBreakIndexIndex);
	if (_recalcSavedRenderState)
	{
		RecalculateDefaultState(startIndex);
	}
	
	_pixelsIndent = m_CachedRenderState.pixelsIndent;
	_currentTextClickable = m_CachedRenderState.textClickable;
	
	bool onRightLine = false;
	for (int i = startIndex; i < m_TextStream.Count(); i++)
	{
		wchar_t ch = m_TextStream[i];

		// if we are on the right line but off the end of if put the cursor at the end of the line
		if (m_LineBreaks[lineBreakIndexIndex] == i)
		{
			// add another line
			AddAnotherLine(x, y);
			lineBreakIndexIndex++;
			
			if (onRightLine)
				break;
		}
		
		// check to see if we're on the right line
		if (cy < yStart)
		{
			// cursor is above panel
			onRightLine = true;
		}
		else if (cy >= y && (cy < (y + fontTall + _drawOffsetY)))
		{
			onRightLine = true;
		}
		
		int wide = surface()->GetCharacterWidth(_font, ch);
		
		// if we've found the position, break
		if (onRightLine)
		{
			if (cx > GetWide())	  // off right side of window
			{
			}
			else if (cx < (_drawOffsetX + _pixelsIndent) || cy < yStart)	 // off left side of window
			{
				return i; // move cursor one to left
			}
			
			if (cx >= x && cx < (x + wide))
			{
				// check which side of the letter they're on
				if (cx < (x + (wide * 0.5)))  // left side
				{
					return i;
				}
				else  // right side
				{						 
					return i + 1;
				}
			}
		}
		x += wide;
	}
	
	return i;
}

//-----------------------------------------------------------------------------
// Purpose: Draws a string of characters in the panel
// Input:	iFirst - Index of the first character to draw
//			iLast - Index of the last character to draw
//			renderState - Render state to use
//			font- font to use
// Output:	returns the width of the character drawn
//-----------------------------------------------------------------------------
int RichText::DrawString(int iFirst, int iLast, TRenderState &renderState, HFont font)
{
	// Calculate the render size
	int fontTall = surface()->GetFontTall(font);
	// BUGBUG John: This won't exactly match the rendered size
	int charWide = 0;
	for ( int i = iFirst; i <= iLast; i++ )
	{
		char ch = m_TextStream[i];
		charWide += surface()->GetCharacterWidth(font, ch);
	}

	// draw selection, if any
	int selection0 = -1, selection1 = -1;
	GetSelectedRange(selection0, selection1);
		
	if (iFirst >= selection0 && iFirst < selection1)
	{
		// draw background selection color
		surface()->DrawSetColor(_selectionColor);
		surface()->DrawFilledRect(renderState.x, renderState.y, renderState.x + charWide, renderState.y + 1 + fontTall);
		
		// reset text color
		surface()->DrawSetTextColor(_selectionTextColor);
	}
	else
	{
		surface()->DrawSetTextColor(renderState.textColor);
	}
		
	surface()->DrawSetTextPos(renderState.x, renderState.y);
	surface()->DrawPrintText(&m_TextStream[iFirst], iLast - iFirst + 1);
		
	return charWide;
}

//-----------------------------------------------------------------------------
// Purpose: Finish drawing url
//-----------------------------------------------------------------------------
void RichText::FinishingURL(int x, int y)
{
	// finishing URL
	ClickPanel *clickPanel = _clickableTextPanels[_clickableTextIndex];
	if (clickPanel)
	{
		int px, py;
		clickPanel->GetPos(px, py);
		int fontTall = surface()->GetFontTall(_font);
		clickPanel->SetSize(x - px, y - py + fontTall);
		clickPanel->SetVisible(true);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Draws the text in the panel
//-----------------------------------------------------------------------------
void RichText::Paint()
{
	// draw background
	Color col = GetBgColor();
	surface()->DrawSetColor(col);
	int wide, tall;
	GetSize(wide, tall);
	surface()->DrawFilledRect(0, 0, wide, tall);
	surface()->DrawSetTextFont(_font);
		
	// hide all the clickable panels until we know where they are to reside
	for (int j = 0; j < _clickableTextPanels.Count(); j++)
	{
		_clickableTextPanels[j]->SetVisible(false);
	}

	if (!m_TextStream.Count())
		return;

	// Check line breaks one last time because it's possible that someone added text to the control after OnThink() but before rendering...
	//  e.g., debug code going to developer console can do this
	if ( m_bRecalcLineBreaks )
	{
		CheckRecalcLineBreaks();
	}
	
	int lineBreakIndexIndex = 0;
	int startIndex = GetStartDrawIndex(lineBreakIndexIndex);
	_currentTextClickable = false;
	
	_clickableTextIndex = GetClickableTextIndexStart(startIndex);
	
	// recalculate and cache the render state at the render start
	if (_recalcSavedRenderState)
	{
		RecalculateDefaultState(startIndex);
	}
	// copy off the cached render state
	TRenderState renderState = m_CachedRenderState;
		
	_pixelsIndent = m_CachedRenderState.pixelsIndent;
	_currentTextClickable = m_CachedRenderState.textClickable;

	renderState.textClickable = _currentTextClickable;
	renderState.textColor = m_FormatStream[renderState.formatStreamIndex++].color;
	if ( _currentTextClickable )
	{
		_clickableTextIndex = startIndex;
	}

	// where to start drawing
	renderState.x = _drawOffsetX + _pixelsIndent;
	renderState.y = _drawOffsetY;
	
	// draw the text
	int selection0 = -1, selection1 = -1;
	GetSelectedRange(selection0, selection1);

	for (int i = startIndex; i < m_TextStream.Count() && renderState.y < tall; )
	{
		// 1.
		// Update our current render state based on the formatting and color streams
		if (UpdateRenderState(i, renderState))
		{
			// check for url state change
			if (renderState.textClickable != _currentTextClickable)
			{
				if (renderState.textClickable)
				{
					// entering new URL
					_clickableTextIndex++;
					
					// set up the panel
					ClickPanel *clickPanel = _clickableTextPanels[_clickableTextIndex];
					
					if (clickPanel)
					{
						clickPanel->SetPos(renderState.x, renderState.y);
					}
				}
				else
				{
					FinishingURL(renderState.x, renderState.y);
				}
				_currentTextClickable = renderState.textClickable;
			}
		}
		
		// 2.
		// if we've passed a line break go to that
		if (m_LineBreaks[lineBreakIndexIndex] == i)
		{
			if (_currentTextClickable)
			{
				FinishingURL(renderState.x, renderState.y);
			}
			
			// add another line
			AddAnotherLine(renderState.x, renderState.y);
			lineBreakIndexIndex++;
			
			if (renderState.textClickable)
			{
				// move to the next URL
				_clickableTextIndex++;
				ClickPanel *clickPanel = _clickableTextPanels[_clickableTextIndex];
				if (clickPanel)
				{
					clickPanel->SetPos(renderState.x, renderState.y);
				}
			}
		}

		// 3.
		// Calculate the range of text to draw all at once
		int iLast = m_TextStream.Count() - 1;
		
		// Stop at the next line break
		if ( m_LineBreaks[lineBreakIndexIndex] <= iLast )
			iLast = m_LineBreaks[lineBreakIndexIndex] - 1;

		// Stop at the next format change
		if ( m_FormatStream.IsValidIndex(renderState.formatStreamIndex) && 
			m_FormatStream[renderState.formatStreamIndex].textStreamIndex <= iLast )
		{
			iLast = m_FormatStream[renderState.formatStreamIndex].textStreamIndex - 1;
		}

		// Stop when entering or exiting the selected range
		if ( i < selection0 && iLast >= selection0 )
			iLast = selection0 - 1;
		if ( i >= selection0 && i <= selection1 && iLast > selection1 )
			iLast = selection1;

		// Handle non-drawing characters specially
		for ( int iT = i; iT <= iLast; iT++ )
		{
			if ( iswcntrl(m_TextStream[iT]) )
			{
				iLast = iT - 1;
				break;
			}
		}

		// 4.
		// Draw the current text range
		if ( iLast < i )
		{
			if ( m_TextStream[i] == '\t' )
			{
				int dxTabWidth = 8 * surface()->GetCharacterWidth(_font, ' ');

				renderState.x = ( dxTabWidth * ( 1 + ( renderState.x / dxTabWidth ) ) );
			}
			i++;
		}
		else
		{
			renderState.x += DrawString(i, iLast, renderState, _font);
			i = iLast + 1;
		}
	}

	if (renderState.textClickable)
	{
		FinishingURL(renderState.x, renderState.y);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int RichText::GetClickableTextIndexStart(int startIndex)
{
	// cycle to the right url panel	for what is visible	after the startIndex.
	for (int i = 0; i < _clickableTextPanels.Count(); i++)
	{
		if (_clickableTextPanels[i]->GetTextIndex() > startIndex)
		{
			return i - 1;
		}
	}
	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: Recalcultes the formatting state from the specified index
//-----------------------------------------------------------------------------
void RichText::RecalculateDefaultState(int startIndex)
{
	if (!m_TextStream.Count())
		return;

	Assert(startIndex < m_TextStream.Count());

	m_CachedRenderState.textColor = GetFgColor();
	_pixelsIndent = 0;
	_currentTextClickable = false;
	_clickableTextIndex = GetClickableTextIndexStart(startIndex);
	
	// find where in the formatting stream we need to be
	GenerateRenderStateForTextStreamIndex(startIndex, m_CachedRenderState);
	_recalcSavedRenderState = false;
}

//-----------------------------------------------------------------------------
// Purpose: updates a render state based on the formatting and color streams
// Output:	true if we changed the render state
//-----------------------------------------------------------------------------
bool RichText::UpdateRenderState(int textStreamPos, TRenderState &renderState)
{
	// check the color stream
	if (m_FormatStream.IsValidIndex(renderState.formatStreamIndex) && 
		m_FormatStream[renderState.formatStreamIndex].textStreamIndex == textStreamPos)
	{
		// set the current formatting
		renderState.textColor = m_FormatStream[renderState.formatStreamIndex].color;
		renderState.textClickable = m_FormatStream[renderState.formatStreamIndex].textClickable;

		int indentChange = m_FormatStream[renderState.formatStreamIndex].pixelsIndent - renderState.pixelsIndent;
		renderState.pixelsIndent = m_FormatStream[renderState.formatStreamIndex].pixelsIndent;

		if (indentChange)
		{
			renderState.x = renderState.pixelsIndent + _drawOffsetX;
		}

		//!! for supporting old functionality, store off state in globals
		_pixelsIndent = renderState.pixelsIndent;

		// move to the next position in the color stream
		renderState.formatStreamIndex++;
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the index in the format stream for the specified text stream index
//-----------------------------------------------------------------------------
int RichText::FindFormatStreamIndexForTextStreamPos(int textStreamIndex)
{
	int formatStreamIndex = 0;
	for (; m_FormatStream.IsValidIndex(formatStreamIndex); formatStreamIndex++)
	{
		if (m_FormatStream[formatStreamIndex].textStreamIndex > textStreamIndex)
			break;
	}

	// step back to the color change before the new line
	formatStreamIndex--;
	if (!m_FormatStream.IsValidIndex(formatStreamIndex))
	{
		formatStreamIndex = 0;
	}
	return formatStreamIndex;
}

//-----------------------------------------------------------------------------
// Purpose: Generates a base renderstate given a index into the text stream
//-----------------------------------------------------------------------------
void RichText::GenerateRenderStateForTextStreamIndex(int textStreamIndex, TRenderState &renderState)
{
	// find where in the format stream we need to be given the specified place in the text stream
	renderState.formatStreamIndex = FindFormatStreamIndexForTextStreamPos(textStreamIndex);
	
	// copy the state data
	renderState.textColor = m_FormatStream[renderState.formatStreamIndex].color;
	renderState.pixelsIndent = m_FormatStream[renderState.formatStreamIndex].pixelsIndent;
	renderState.textClickable = m_FormatStream[renderState.formatStreamIndex].textClickable;
}

//-----------------------------------------------------------------------------
// Purpose: Called pre render
//-----------------------------------------------------------------------------
void RichText::OnThink()
{
	CheckRecalcLineBreaks();
}

void RichText::CheckRecalcLineBreaks()
{
	if (m_bRecalcLineBreaks)
	{
		_recalcSavedRenderState = true;
		RecalculateLineBreaks();
		
		// recalculate scrollbar position
		if (_invalidateVerticalScrollbarSlider)
		{
			LayoutVerticalScrollBarSlider();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called when data changes or panel size changes
//-----------------------------------------------------------------------------
void RichText::PerformLayout()
{
	BaseClass::PerformLayout();

	// force a Repaint
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: inserts a color change into the formatting stream
//-----------------------------------------------------------------------------
void RichText::InsertColorChange(Color col)
{
	// see if color already exists in text stream
	TFormatStream &prevItem = m_FormatStream[m_FormatStream.Count() - 1];
	if (prevItem.color == col)
	{
		// inserting same color into stream, just ignore
	}
	else if (prevItem.textStreamIndex == m_TextStream.Count())
	{
		// this item is in the same place; update values
		prevItem.color = col;
	}
	else
	{
		// add to text stream, based off existing item
		TFormatStream streamItem = prevItem;
		streamItem.color = col;
		streamItem.textStreamIndex = m_TextStream.Count();
		m_FormatStream.AddToTail(streamItem);
	}
}

//-----------------------------------------------------------------------------
// Purpose: inserts an indent change into the formatting stream
//-----------------------------------------------------------------------------
void RichText::InsertIndentChange(int pixelsIndent)
{
	if (pixelsIndent < 0)
	{
		pixelsIndent = 0;
	}
	else if (pixelsIndent > 255)
	{
		pixelsIndent = 255;
	}

	// see if indent change already exists in text stream
	TFormatStream &prevItem = m_FormatStream[m_FormatStream.Count() - 1];
	if (prevItem.pixelsIndent == pixelsIndent)
	{
		// inserting same indent into stream, just ignore
	}
	else if (prevItem.textStreamIndex == m_TextStream.Count())
	{
		// this item is in the same place; update
		prevItem.pixelsIndent = pixelsIndent;
	}
	else
	{
		// add to text stream, based off existing item
		TFormatStream streamItem = prevItem;
		streamItem.pixelsIndent = pixelsIndent;
		streamItem.textStreamIndex = m_TextStream.Count();
		m_FormatStream.AddToTail(streamItem);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Inserts character Start for clickable text, eg. URLS
//-----------------------------------------------------------------------------
void RichText::InsertClickableTextStart( const char *pchClickAction )
{
	// see if indent change already exists in text stream
	TFormatStream &prevItem = m_FormatStream[m_FormatStream.Count() - 1];
	if (prevItem.textClickable)
	{
		// inserting same indent into stream, just ignore
	}
	else if (prevItem.textStreamIndex == m_TextStream.Count())
	{
		// this item is in the same place; update
		prevItem.textClickable = true;
	}
	else
	{
		// add to text stream, based off existing item
		TFormatStream streamItem = prevItem;
		streamItem.textClickable = true;
		streamItem.textStreamIndex = m_TextStream.Count();
		streamItem.m_sClickableTextAction = pchClickAction;
		m_FormatStream.AddToTail(streamItem);
	}

	// invalidate the layout to recalculate where the click panels should go
	InvalidateLineBreakStream();
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Inserts character end for clickable text, eg. URLS
//-----------------------------------------------------------------------------
void RichText::InsertClickableTextEnd()
{
	// see if indent change already exists in text stream
	TFormatStream &prevItem = m_FormatStream[m_FormatStream.Count() - 1];
	if (!prevItem.textClickable)
	{
		// inserting same indent into stream, just ignore
	}
	else if (prevItem.textStreamIndex == m_TextStream.Count())
	{
		// this item is in the same place; update
		prevItem.textClickable = false;
	}
	else
	{
		// add to text stream, based off existing item
		TFormatStream streamItem = prevItem;
		streamItem.textClickable = false;
		streamItem.textStreamIndex = m_TextStream.Count();
		m_FormatStream.AddToTail(streamItem);
	}
}

//-----------------------------------------------------------------------------
// Purpose: moves x,y to the Start of the next line of text
//-----------------------------------------------------------------------------
void RichText::AddAnotherLine(int &cx, int &cy)
{
	cx = _drawOffsetX + _pixelsIndent;
	cy += (surface()->GetFontTall(_font) + _drawOffsetY);
}

//-----------------------------------------------------------------------------
// Purpose: Recalculates line breaks
//-----------------------------------------------------------------------------
void RichText::RecalculateLineBreaks()
{
	m_bRecalcLineBreaks = false;
	_recalcSavedRenderState = true;
	if (!m_TextStream.Count())
		return;
	
	HFont font = _font;
	int wide = GetWide();
	
	// subtract the scrollbar width
	if (_vertScrollBar->IsVisible())
	{
		wide -= _vertScrollBar->GetWide();
	}
	
	int charWidth;
	int x = _drawOffsetX, y = _drawOffsetY;
	
	int wordStartIndex = 0;
	int wordLength = 0;
	bool hasWord = false;
	bool justStartedNewLine = true;
	bool wordStartedOnNewLine = true;
	
	int startChar = 0;
	if (_recalculateBreaksIndex <= 0)
	{
		m_LineBreaks.RemoveAll();
	}
	else
	{
		// remove the rest of the linebreaks list since its out of date.
		for (int i = _recalculateBreaksIndex + 1; i < m_LineBreaks.Count(); ++i)
		{
			m_LineBreaks.Remove(i);
			--i; // removing shrinks the list!
		}
		startChar = m_LineBreaks[_recalculateBreaksIndex];
	}
	
	// handle the case where this char is a new line, in that case
	// we have already taken its break index into account above so skip it.
	if (m_TextStream[startChar] == '\r' || m_TextStream[startChar] == '\n') 
	{
		startChar++;
	}
	
	// cycle to the right url panel	for what is visible	after the startIndex.
	int clickableTextNum = GetClickableTextIndexStart(startChar);
	clickableTextNum++;

	// initialize the renderstate with the start
	TRenderState renderState;
	GenerateRenderStateForTextStreamIndex(startChar, renderState);
	_currentTextClickable = false;
	
	// loop through all the characters	
	for (int i = startChar; i < m_TextStream.Count(); ++i)
	{
		wchar_t ch = m_TextStream[i];
		renderState.x = x;
		if (UpdateRenderState(i, renderState))
		{
			x = renderState.x;
			int preI = i;
			
			// check for clickable text
			if (renderState.textClickable != _currentTextClickable)
			{
				if (renderState.textClickable)
				{
					// make a new clickable text panel
					if (clickableTextNum >= _clickableTextPanels.Count())
					{
						_clickableTextPanels.AddToTail(new ClickPanel(this));
					}
					
					ClickPanel *clickPanel = _clickableTextPanels[clickableTextNum++];
					clickPanel->SetTextIndex(preI);
				}
				
				// url state change
				_currentTextClickable = renderState.textClickable;
			}
		}
		
		// line break only on whitespace characters
		if (!iswspace(ch))
		{
			if (hasWord)
			{
				// append to the current word
			}
			else
			{
				// Start a new word
				wordStartIndex = i;
				hasWord = true;
				wordStartedOnNewLine = justStartedNewLine;
				wordLength = 0;
			}
		}
		else
		{
			// whitespace/punctuation character
			// end the word
			hasWord = false;
		}
		
		// get the width
		charWidth = surface()->GetCharacterWidth(font, ch);
		if (!iswcntrl(ch))
		{
			justStartedNewLine = false;
		}
				
		// check to see if the word is past the end of the line [wordStartIndex, i)
		if ((x + charWidth) >= wide || ch == '\r' || ch == '\n')
		{
			// add another line
			AddAnotherLine(x, y);
			
			justStartedNewLine = true;
			hasWord = false;
			
			if (ch == '\r' || ch == '\n')
			{
				// set the break at the current character
				m_LineBreaks.AddToTail(i);
			}
			else if (wordStartedOnNewLine || iswspace(ch) ) // catch the "blah             " case wrapping around a line
			{
				// word is longer than a line, so set the break at the current cursor
				m_LineBreaks.AddToTail(i);
				
				if (renderState.textClickable)
				{
					// need to split the url into two panels
					int oldIndex = _clickableTextPanels[clickableTextNum - 1]->GetTextIndex();
					
					// make a new clickable text panel
					if (clickableTextNum >= _clickableTextPanels.Count())
					{
						_clickableTextPanels.AddToTail(new ClickPanel(this));
					}
					
					ClickPanel *clickPanel = _clickableTextPanels[clickableTextNum++];
					clickPanel->SetTextIndex(oldIndex);
				}
			}
			else
			{
				// set it at the last word Start
				m_LineBreaks.AddToTail(wordStartIndex);
				
				// just back to reparse the next line of text
				i = wordStartIndex;
			}
			
			// reset word length
			wordLength = 0;
		}
		
		// add to the size
		x += charWidth;
		wordLength += charWidth;
	}
	
	// end the list
	m_LineBreaks.AddToTail(MAX_BUFFER_SIZE);
	
	// set up the scrollbar
	_invalidateVerticalScrollbarSlider = true;
}

//-----------------------------------------------------------------------------
// Purpose: Recalculate where the vertical scroll bar slider should be 
//			based on the current cursor line we are on.
//-----------------------------------------------------------------------------
void RichText::LayoutVerticalScrollBarSlider()
{
	_invalidateVerticalScrollbarSlider = false;

	// set up the scrollbar
	if (!_vertScrollBar->IsVisible())
		return;

	// see where the scrollbar currently is
	int previousValue = _vertScrollBar->GetValue();
	bool bCurrentlyAtEnd = false;
	int rmin, rmax;
	_vertScrollBar->GetRange(rmin, rmax);
	if (rmax && (previousValue + rmin + _vertScrollBar->GetRangeWindow() == rmax))
	{
		bCurrentlyAtEnd = true;
	}
	
	// work out position to put scrollbar, factoring in insets
	int wide, tall, ileft, iright, itop, ibottom;
	GetSize(wide, tall);
	GetInset(ileft, iright, itop, ibottom);

	// with a scroll bar we take off the inset
	wide -= iright;

	_vertScrollBar->SetPos(wide - _vertScrollBar->GetWide() - 1, itop - 1);
	// scrollbar is inside the borders.
	_vertScrollBar->SetSize(_vertScrollBar->GetWide(), tall - ibottom - itop);
	
	// calculate how many lines we can fully display
	int displayLines = tall / (surface()->GetFontTall(_font) + _drawOffsetY);
	int numLines = m_LineBreaks.Count();
	
	if (numLines <= displayLines)
	{
		// disable the scrollbar
		_vertScrollBar->SetEnabled(false);
		_vertScrollBar->SetRange(0, numLines);
		_vertScrollBar->SetRangeWindow(numLines);
		_vertScrollBar->SetValue(0);
	}
	else
	{
		// set the scrollbars range
		_vertScrollBar->SetRange(0, numLines);
		_vertScrollBar->SetRangeWindow(displayLines);
		_vertScrollBar->SetEnabled(true);
		
		// this should make it scroll one line at a time
		_vertScrollBar->SetButtonPressedScrollValue(1);
		if (bCurrentlyAtEnd)
		{
			_vertScrollBar->SetValue(numLines - displayLines);
		}
		_vertScrollBar->InvalidateLayout();
		_vertScrollBar->Repaint();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets whether a vertical scrollbar is visible
//-----------------------------------------------------------------------------
void RichText::SetVerticalScrollbar(bool state)
{
	if (_vertScrollBar->IsVisible() != state)
	{
		_vertScrollBar->SetVisible(state);
		InvalidateLayout();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Create cut/copy/paste dropdown menu
//-----------------------------------------------------------------------------
void RichText::CreateEditMenu()
{	
	// create a drop down cut/copy/paste menu appropriate for this object's states
	if (m_pEditMenu)
		delete m_pEditMenu;
	m_pEditMenu = new Menu(this, "EditMenu");
	
	
	// add cut/copy/paste drop down options if its editable, just copy if it is not
	m_pEditMenu->AddMenuItem("C&opy", new KeyValues("DoCopySelected"), this);
	
	m_pEditMenu->SetVisible(false);
	m_pEditMenu->SetParent(this);
	m_pEditMenu->AddActionSignalTarget(this);
}

//-----------------------------------------------------------------------------
// Purpose: We want single line windows to scroll horizontally and select text
//          in response to clicking and holding outside window
//-----------------------------------------------------------------------------
void RichText::OnMouseFocusTicked()
{
	// if a button is down move the scrollbar slider the appropriate direction
	if (_mouseDragSelection) // text is being selected via mouse clicking and dragging
	{
		OnCursorMoved(0,0);	// we want the text to scroll as if we were dragging
	}	
}

//-----------------------------------------------------------------------------
// Purpose: If a cursor enters the window, we are not elegible for 
//          MouseFocusTicked events
//-----------------------------------------------------------------------------
void RichText::OnCursorEntered()
{
	_mouseDragSelection = false; // outside of window dont recieve drag scrolling ticks
}

//-----------------------------------------------------------------------------
// Purpose: When the cursor is outside the window, if we are holding the mouse
//			button down, then we want the window to scroll the text one char at a time using Ticks
//-----------------------------------------------------------------------------
void RichText::OnCursorExited() 
{
	// outside of window recieve drag scrolling ticks
	if (_mouseSelection)
	{
		_mouseDragSelection = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handle selection of text by mouse
//-----------------------------------------------------------------------------
void RichText::OnCursorMoved(int x, int y)
{
	if (_mouseSelection)
	{
		// update the cursor position
		int x, y;
		input()->GetCursorPos(x, y);
		ScreenToLocal(x, y);
		_cursorPos = PixelToCursorSpace(x, y);
		
		if (_cursorPos != _select[1])
		{
			_select[1] = _cursorPos;
			Repaint();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handle mouse button down events.
//-----------------------------------------------------------------------------
void RichText::OnMousePressed(MouseCode code)
{
	if (code == MOUSE_LEFT)
	{
		// clear current selection
		SelectNone();

		// move the cursor to where the mouse was pressed
		int x, y;
		input()->GetCursorPos(x, y);
		ScreenToLocal(x, y);
		
		_cursorPos = PixelToCursorSpace(x, y);
		
		// enter selection mode
		input()->SetMouseCapture(GetVPanel());
		_mouseSelection = true;
		
		if (_select[0] < 0)
		{
			// if no initial selection position, Start selection position at cursor
			_select[0] = _cursorPos;
		}
		_select[1] = _cursorPos;
		
		RequestFocus();
		Repaint();
	}
	else if (code == MOUSE_RIGHT) // check for context menu open
	{	
		CreateEditMenu();
		Assert(m_pEditMenu);
		
		OpenEditMenu();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handle mouse button up events
//-----------------------------------------------------------------------------
void RichText::OnMouseReleased(MouseCode code)
{
	_mouseSelection = false;
	input()->SetMouseCapture(NULL);
	
	// make sure something has been selected
	int cx0, cx1;
	if (GetSelectedRange(cx0, cx1))
	{
		if (cx1 - cx0 == 0)
		{
			// nullify selection
			_select[0] = -1;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handle mouse double clicks
//-----------------------------------------------------------------------------
void RichText::OnMouseDoublePressed(MouseCode code)
{
	// left double clicking on a word selects the word
	if (code == MOUSE_LEFT)
	{
		// move the cursor just as if you single clicked.
		OnMousePressed(code);
		// then find the start and end of the word we are in to highlight it.
		int selectSpot[2];
		GotoWordLeft();
		selectSpot[0] = _cursorPos;
		GotoWordRight();
		selectSpot[1] = _cursorPos;
		
		if ( _cursorPos > 0 && (_cursorPos-1) < m_TextStream.Count() )
		{
			if (iswspace(m_TextStream[_cursorPos-1]))
			{
				selectSpot[1]--;
				_cursorPos--;
			}
		}
		
		_select[0] = selectSpot[0];
		_select[1] = selectSpot[1];
		_mouseSelection = true;
	}
	
}

//-----------------------------------------------------------------------------
// Purpose: Turn off text selection code when mouse button is not down
//-----------------------------------------------------------------------------
void RichText::OnMouseCaptureLost()
{
	_mouseSelection = false;
}

//-----------------------------------------------------------------------------
// Purpose: Masks which keys get chained up
//			Maps keyboard input to text window functions.
//-----------------------------------------------------------------------------
void RichText::OnKeyCodeTyped(KeyCode code)
{
	bool shift = (input()->IsKeyDown(KEY_LSHIFT) || input()->IsKeyDown(KEY_RSHIFT));
	bool ctrl = (input()->IsKeyDown(KEY_LCONTROL) || input()->IsKeyDown(KEY_RCONTROL));
	bool alt = (input()->IsKeyDown(KEY_LALT) || input()->IsKeyDown(KEY_RALT));
	bool fallThrough = false;
		
	if (ctrl)
	{
		switch(code)
		{
		case KEY_INSERT:
		case KEY_C:
		case KEY_X:
			{
				CopySelected();
				break;
			}
		case KEY_PAGEUP:
		case KEY_HOME:
			{
				GotoTextStart();
				break;
			}
		case KEY_PAGEDOWN:
		case KEY_END:
			{
				GotoTextEnd();
				break;
			}
		default:
			{
				fallThrough = true;
				break;
			}
		}
	}
	else if (alt)
	{
		// do nothing with ALT-x keys
		fallThrough = true;
	}
	else
	{
		switch(code)
		{
		case KEY_TAB:
		case KEY_LSHIFT:
		case KEY_RSHIFT:
		case KEY_ESCAPE:
		case KEY_ENTER:
			{
				fallThrough = true;
				break;
			}
		case KEY_DELETE:
			{
				if (shift)
				{
					// shift-delete is cut
					CopySelected();
				}
				break;
			}
		case KEY_HOME:
			{
				GotoTextStart();
				break;
			}
		case KEY_END:
			{
				GotoTextEnd();
				break;
			}
		case KEY_PAGEUP:
			{
				// if there is a scroll bar scroll down one rangewindow
				if (_vertScrollBar->IsVisible())
				{
					int window = _vertScrollBar->GetRangeWindow();
					int newval = _vertScrollBar->GetValue();
					_vertScrollBar->SetValue(newval - window - 1);
				}
				break;
				
			}
		case KEY_PAGEDOWN:
			{
				// if there is a scroll bar scroll down one rangewindow
				if (_vertScrollBar->IsVisible())
				{
					int window = _vertScrollBar->GetRangeWindow();
					int newval = _vertScrollBar->GetValue();
					_vertScrollBar->SetValue(newval + window + 1);
				}
				break;
			}
		default:
			{
				// return if any other char is pressed.
				// as it will be a unicode char.
				// and we don't want select[1] changed unless a char was pressed that this fxn handles
				return;
			}
		}
	}
	
	// select[1] is the location in the line where the blinking cursor started
	_select[1] = _cursorPos;
	
	// chain back on some keys
	if (fallThrough)
	{
		BaseClass::OnKeyCodeTyped(code);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Scrolls the list according to the mouse wheel movement
//-----------------------------------------------------------------------------
void RichText::OnMouseWheeled(int delta)
{
	MoveScrollBar(delta);
}

//-----------------------------------------------------------------------------
// Purpose: Scrolls the list 
// Input  : delta - amount to move scrollbar up
//-----------------------------------------------------------------------------
void RichText::MoveScrollBar(int delta)
{
	if (_vertScrollBar->IsVisible())
	{
		int val = _vertScrollBar->GetValue();
		val -= (delta * 3);
		_vertScrollBar->SetValue(val);
		_recalcSavedRenderState = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: set the maximum number of chars in the text buffer
//-----------------------------------------------------------------------------
void RichText::SetMaximumCharCount(int maxChars)
{
	_maxCharCount = maxChars;
}

//-----------------------------------------------------------------------------
// Purpose: Find out what line the cursor is on
//-----------------------------------------------------------------------------
int RichText::GetCursorLine()
{
	// always returns the last place
	int pos = m_LineBreaks[m_LineBreaks.Count() - 1];
	Assert(pos == MAX_BUFFER_SIZE);
	return pos;
}

//-----------------------------------------------------------------------------
// Purpose: Move the cursor over to the Start of the next word to the right
//-----------------------------------------------------------------------------
void RichText::GotoWordRight()
{
	// search right until we hit a whitespace character or a newline
	while (++_cursorPos < m_TextStream.Count())
	{
		if (iswspace(m_TextStream[_cursorPos]))
			break;
	}
	
	// search right until we hit an nonspace character
	while (++_cursorPos < m_TextStream.Count())
	{
		if (!iswspace(m_TextStream[_cursorPos]))
			break;
	}
	
	if (_cursorPos > m_TextStream.Count())
	{
		_cursorPos = m_TextStream.Count();
	}
	
	// now we are at the start of the next word
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Move the cursor over to the Start of the next word to the left
//-----------------------------------------------------------------------------
void RichText::GotoWordLeft()
{
	if (_cursorPos < 1)
		return;
	
	// search left until we hit an nonspace character
	while (--_cursorPos >= 0)
	{
		if (!iswspace(m_TextStream[_cursorPos]))
			break;
	}
	
	// search left until we hit a whitespace character
	while (--_cursorPos >= 0)
	{
		if (iswspace(m_TextStream[_cursorPos]))
		{
			break;
		}
	}
	
	// we end one character off
	_cursorPos++;

	// now we are at the start of the previous word
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Move cursor to the Start of the text buffer
//-----------------------------------------------------------------------------
void RichText::GotoTextStart()
{
	_cursorPos = 0;	// set cursor to start
	_invalidateVerticalScrollbarSlider = true;
	// force scrollbar to the top
	_vertScrollBar->SetValue(0);
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Move cursor to the end of the text buffer
//-----------------------------------------------------------------------------
void RichText::GotoTextEnd()
{
	_cursorPos = m_TextStream.Count();	// set cursor to end of buffer
	_invalidateVerticalScrollbarSlider = true;

	// force the scrollbar to the bottom
	int min, max;
	_vertScrollBar->GetRange(min, max);
	_vertScrollBar->SetValue(max);

	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Culls the text stream down to a managable size
//-----------------------------------------------------------------------------
void RichText::TruncateTextStream()
{
	if (_maxCharCount < 1)
		return;

	// choose a point to cull at
	int cullPos = _maxCharCount / 2;

	// kill half the buffer
	m_TextStream.RemoveMultiple(0, cullPos);

	// work out where in the format stream we can start
	int formatIndex = FindFormatStreamIndexForTextStreamPos(cullPos);
	if (formatIndex > 0)
	{
		// take a copy, make it first
		m_FormatStream[0] = m_FormatStream[formatIndex];
		m_FormatStream[0].textStreamIndex = 0;
		// kill the others
		m_FormatStream.RemoveMultiple(1, formatIndex);
	}

	// renormalize the remainder of the format stream
	for (int i = 1; i < m_FormatStream.Count(); i++)
	{
		Assert(m_FormatStream[i].textStreamIndex > cullPos);
		m_FormatStream[i].textStreamIndex -= cullPos;
	}

	// mark everything to be recalculated
	InvalidateLineBreakStream();
	InvalidateLayout();
	_invalidateVerticalScrollbarSlider = true;
}

//-----------------------------------------------------------------------------
// Purpose: Insert a character into the text buffer
//-----------------------------------------------------------------------------
void RichText::InsertChar(wchar_t ch)
{
	// throw away redundant linefeed characters
	if ((char)ch == '\r')
		return;

	if (_maxCharCount > 0 && m_TextStream.Count() > _maxCharCount)
	{
		TruncateTextStream();
	}
	
	// insert the new char at the end of the buffer
	m_TextStream.AddToTail(ch);

	// mark the linebreak steam as needing recalculating from that point
	_recalculateBreaksIndex = m_LineBreaks.Count() - 2;
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Insert a string into the text buffer, this is just a series
//			of char inserts because we have to check each char is ok to insert
//-----------------------------------------------------------------------------
void RichText::InsertString(const char *text)
{
	if (text[0] == '#')
	{
		// check for localization
		wchar_t *wsz = localize()->Find(text);
		if (wsz)
		{
			InsertString(wsz);
			return;
		}
	}

	// upgrade the ansi text to unicode to display it
	int len = strlen(text);
	wchar_t *unicode = (wchar_t *)_alloca((len + 1) * sizeof(wchar_t));
	localize()->ConvertANSIToUnicode(text, unicode, ((len + 1) * sizeof(wchar_t)));
	InsertString(unicode);
}

//-----------------------------------------------------------------------------
// Purpose: Insertsa a unicode string into the buffer
//-----------------------------------------------------------------------------
void RichText::InsertString(const wchar_t *wszText)
{
	// insert the whole string
	for (const wchar_t *ch = wszText; *ch != 0; ++ch)
	{
		InsertChar(*ch);
	}
	InvalidateLayout();
	m_bRecalcLineBreaks = true;
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Declare a selection empty
//-----------------------------------------------------------------------------
void RichText::SelectNone()
{
	// tag the selection as empty
	_select[0] = -1;
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Load in the selection range so cx0 is the Start and cx1 is the end
//			from smallest to highest (right to left)
//-----------------------------------------------------------------------------
bool RichText::GetSelectedRange(int &cx0, int &cx1)
{
	// if there is nothing selected return false
	if (_select[0] == -1)
		return false;
	
	// sort the two position so cx0 is the smallest
	cx0 = _select[0];
	cx1 = _select[1];
	if (cx1 < cx0)
	{
		int temp = cx0;
		cx0 = cx1;
		cx1 = temp;
	}
	
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Opens the cut/copy/paste dropdown menu
//-----------------------------------------------------------------------------
void RichText::OpenEditMenu()
{
	// get cursor position, this is local to this text edit window
	// so we need to adjust it relative to the parent
	int cursorX, cursorY;
	input()->GetCursorPos(cursorX, cursorY);
	
	/* !!	disabled since it recursively gets panel pointers, potentially across dll boundaries, 
			and doesn't need to be necessary (it's just for handling windowed mode)

	// find the frame that has no parent (the one on the desktop)
	Panel *panel = this;
	while ( panel->GetParent() != NULL)
	{
		panel = panel->GetParent();
	}
	panel->ScreenToLocal(cursorX, cursorY);
	int x, y;
	// get base panel's postition
	panel->GetPos(x, y);	  
	
	// adjust our cursor position accordingly
	cursorX += x;
	cursorY += y;
	*/
	
	int x0, x1;
	if (GetSelectedRange(x0, x1)) // there is something selected
	{
		m_pEditMenu->SetItemEnabled("&Cut", true);
		m_pEditMenu->SetItemEnabled("C&opy", true);
	}
	else	// there is nothing selected, disable cut/copy options
	{
		m_pEditMenu->SetItemEnabled("&Cut", false);
		m_pEditMenu->SetItemEnabled("C&opy", false);
	}
	m_pEditMenu->SetVisible(true);
	m_pEditMenu->RequestFocus();
	
	// relayout the menu immediately so that we know it's size
	m_pEditMenu->InvalidateLayout(true);
	int menuWide, menuTall;
	m_pEditMenu->GetSize(menuWide, menuTall);
	
	// work out where the cursor is and therefore the best place to put the menu
	int wide, tall;
	surface()->GetScreenSize(wide, tall);
	
	if (wide - menuWide > cursorX)
	{
		// menu hanging right
		if (tall - menuTall > cursorY)
		{
			// menu hanging down
			m_pEditMenu->SetPos(cursorX, cursorY);
		}
		else
		{
			// menu hanging up
			m_pEditMenu->SetPos(cursorX, cursorY - menuTall);
		}
	}
	else
	{
		// menu hanging left
		if (tall - menuTall > cursorY)
		{
			// menu hanging down
			m_pEditMenu->SetPos(cursorX - menuWide, cursorY);
		}
		else
		{
			// menu hanging up
			m_pEditMenu->SetPos(cursorX - menuWide, cursorY - menuTall);
		}
	}
	
	m_pEditMenu->RequestFocus();
}

//-----------------------------------------------------------------------------
// Purpose: Cuts the selected chars from the buffer and 
//          copies them into the clipboard
//-----------------------------------------------------------------------------
void RichText::CutSelected()
{
	CopySelected();
	// have to request focus if we used the menu
	RequestFocus();	
}

//-----------------------------------------------------------------------------
// Purpose: Copies the selected chars into the clipboard
//-----------------------------------------------------------------------------
void RichText::CopySelected()
{
	int x0, x1;
	if (GetSelectedRange(x0, x1))
	{
		CUtlVector<wchar_t> buf;
		for (int i = x0; i < x1; i++)
		{
			if (m_TextStream[i] == '\n') 
			{
				buf.AddToTail( '\r' );
			}
			// remove any rich edit commands
			buf.AddToTail(m_TextStream[i]);
		}
		buf.AddToTail('\0');
		system()->SetClipboardText(buf.Base(), buf.Count() - 1);
	}
	
	// have to request focus if we used the menu
	RequestFocus();	
}

//-----------------------------------------------------------------------------
// Purpose: Returns the index in the text buffer of the
//          character the drawing should Start at
//-----------------------------------------------------------------------------
int RichText::GetStartDrawIndex(int &lineBreakIndexIndex)
{
	int startIndex = 0;
	int startLine = _vertScrollBar->GetValue();
	
	if ( startLine >= m_LineBreaks.Count() ) // incase the line breaks got reset and the scroll bar hasn't
	{
		startLine = m_LineBreaks.Count() - 1;
	}

	lineBreakIndexIndex = startLine;
	if (startLine && startLine < m_LineBreaks.Count())
	{
		startIndex = m_LineBreaks[startLine - 1];
	}
	
	return startIndex;
}

//-----------------------------------------------------------------------------
// Purpose: Get a string from text buffer
// Input:	offset - index to Start reading from 
//			bufLen - length of string
//-----------------------------------------------------------------------------
void RichText::GetText(int offset, wchar_t *buf, int bufLenInBytes)
{
	if (!buf)
		return;
	
	int bufLen = bufLenInBytes / sizeof(wchar_t);
	for (int i = offset; i < (offset + bufLen - 1); i++)
	{
		if (i >= m_TextStream.Count())
			break;
		
		buf[i-offset] = m_TextStream[i];
	}
	buf[(i-offset)] = 0;
	buf[bufLen-1] = 0;
}

//-----------------------------------------------------------------------------
// Purpose: gets text from the buffer
//-----------------------------------------------------------------------------
void RichText::GetText(int offset, char *pch, int bufLenInBytes)
{
	wchar_t rgwchT[4096];
	GetText(offset, rgwchT, sizeof(rgwchT));
	localize()->ConvertUnicodeToANSI(rgwchT, pch, bufLenInBytes);
}

//-----------------------------------------------------------------------------
// Purpose: Set the font of the buffer text 
//-----------------------------------------------------------------------------
void RichText::SetFont(HFont font)
{
	_font = font;
	InvalidateLayout();
	m_bRecalcLineBreaks = true;
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Called when the scrollbar slider is moved
//-----------------------------------------------------------------------------
void RichText::OnSliderMoved()
{
	_recalcSavedRenderState = true;
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool RichText::RequestInfo(KeyValues *outputData)
{
	if (!stricmp(outputData->GetName(), "GetText"))
	{
		wchar_t wbuf[512];
		GetText(0, wbuf, sizeof(wbuf));
		outputData->SetWString("text", wbuf);
		return true;
	}
	
	return BaseClass::RequestInfo(outputData);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void RichText::OnSetText(const wchar_t *text)
{
	SetText(text);
}

//-----------------------------------------------------------------------------
// Purpose: Called when a URL, etc has been clicked on
//-----------------------------------------------------------------------------
void RichText::OnClickPanel(int index)
{
	wchar_t wBuf[512];
	int outIndex = 0;
	
	// parse out the clickable text, and send it to our listeners
	_currentTextClickable = true;
	TRenderState renderState;
	GenerateRenderStateForTextStreamIndex(index, renderState);
	for (int i = index; i < (sizeof(wBuf) - 1) && i < m_TextStream.Count(); i++)
	{
		// stop getting characters when text is no longer clickable
		UpdateRenderState(i, renderState);
		if (!renderState.textClickable)
			break;

		// copy out the character
		wBuf[outIndex++] = m_TextStream[i];
	}
	
	wBuf[outIndex] = 0;

    if ( m_FormatStream[renderState.formatStreamIndex].m_sClickableTextAction )
	{
		localize()->ConvertANSIToUnicode( m_FormatStream[renderState.formatStreamIndex].m_sClickableTextAction.String(), wBuf, sizeof( wBuf ) );
	}

	PostActionSignal(new KeyValues("TextClicked", "text", wBuf));
	OnTextClicked(wBuf);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void RichText::ApplySettings(KeyValues *inResourceData)
{
	BaseClass::ApplySettings(inResourceData);
	SetMaximumCharCount(inResourceData->GetInt("maxchars", -1));

	// get the starting text, if any
	const char *text = inResourceData->GetString("text", "");
	if (*text)
	{
		delete [] m_pszInitialText;
		int len = Q_strlen(text) + 1;
		m_pszInitialText = new char[ len ];
		Q_strncpy( m_pszInitialText, text, len );
		SetText(text);
	}
	else
	{
		const char *textfilename = inResourceData->GetString("textfile", NULL);
		if ( textfilename )
		{
			FileHandle_t f = filesystem()->Open( textfilename, "rt" );
			if (!f)
			{
				Warning( "RichText: textfile parameter '%s' not found.\n", textfilename );
				return;
			}

			int len = filesystem()->Size( f ) + 1;
			delete [] m_pszInitialText;
			m_pszInitialText = new char[ len ];
			filesystem()->Read( m_pszInitialText, len - 1, f );
			SetText( m_pszInitialText );

			filesystem()->Close( f );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void RichText::GetSettings(KeyValues *outResourceData)
{
	BaseClass::GetSettings(outResourceData);
	outResourceData->SetInt("maxchars", _maxCharCount);
	if (m_pszInitialText)
	{
		outResourceData->SetString("text", m_pszInitialText);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *RichText::GetDescription()
{
	static char buf[1024];
	Q_snprintf(buf, sizeof(buf), "%s, string text", BaseClass::GetDescription());
	return buf;
}

//-----------------------------------------------------------------------------
// Purpose: Get the number of lines in the window
//-----------------------------------------------------------------------------
int RichText::GetNumLines()
{
	return m_LineBreaks.Count();
}

//-----------------------------------------------------------------------------
// Purpose: Sets the height of the text entry window so all text will fit inside
//-----------------------------------------------------------------------------
void RichText::SetToFullHeight()
{
	PerformLayout();
	int wide, tall;
	GetSize(wide, tall);
	
	tall = GetNumLines() * (surface()->GetFontTall(_font) + _drawOffsetY) + _drawOffsetY + 2;
	SetSize (wide, tall);
	PerformLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Select all the text.
//-----------------------------------------------------------------------------
void RichText::SelectAllText()
{
	_cursorPos = 0;
	_select[0] = 0;
	_select[1] = m_TextStream.Count();
}

//-----------------------------------------------------------------------------
// Purpose: Select all the text.
//-----------------------------------------------------------------------------
void RichText::SelectNoText()
{
	_select[0] = 0;
	_select[1] = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void RichText::OnSetFocus()
{ 
	BaseClass::OnSetFocus();
}

//-----------------------------------------------------------------------------
// Purpose: Invalidates the current linebreak stream
//-----------------------------------------------------------------------------
void RichText::InvalidateLineBreakStream()
{
	// clear the buffer
	m_LineBreaks.RemoveAll();
	m_LineBreaks.AddToTail(MAX_BUFFER_SIZE);
	_recalculateBreaksIndex = 0;
	m_bRecalcLineBreaks = true;
}

//-----------------------------------------------------------------------------
// Purpose: Inserts a text string while making URLs clickable/different color
// Input  : *text - string that may contain URLs to make clickable/color coded
//			URLTextColor - color for URL text
//          normalTextColor - color for normal text
//-----------------------------------------------------------------------------
void RichText::InsertPossibleURLString(const char* text, Color URLTextColor, Color normalTextColor)
{
	InsertColorChange(normalTextColor);

	// parse out the string for URL's
	int len = Q_strlen(text), pos = 0;
	bool clickable = false;
	char *resultBuf = (char *)stackalloc( len + 1 );
	while (pos < len)
	{
		pos = ParseTextStringForUrls(text, pos, resultBuf, len, clickable);
		
		if (clickable)
		{
			InsertClickableTextStart();
			InsertColorChange(URLTextColor);
		}
		
		InsertString(resultBuf);
		
		if (clickable)
		{
			InsertColorChange(normalTextColor);
			InsertClickableTextEnd();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: looks for URLs in the string and returns information about the URL
//-----------------------------------------------------------------------------
int RichText::ParseTextStringForUrls(const char *text, int startPos, char *resultBuffer, int resultBufferSize, bool &clickable)
{
	// scan for text that looks like a URL
	int i = startPos;
	while (text[i] != 0)
	{
		bool bURLFound = false;
		
		if (!Q_strnicmp(text + i, "www.", 4))
		{
			// scan ahead for another '.'
			bool bPeriodFound = false;
			for (const char *ch = text + i + 5; ch != 0; ch++)
			{
				if (*ch == '.')
				{
					bPeriodFound = true;
					break;
				}
			}
			
			// URL found
			if (bPeriodFound)
			{
				bURLFound = true;
			}
		}
		else if (!Q_strnicmp(text + i, "http://", 7))
		{
			bURLFound = true;
		}
		else if (!Q_strnicmp(text + i, "ftp://", 6))
		{
			bURLFound = true;
		}
		else if (!Q_strnicmp(text + i, "steam://", 8))
		{
			bURLFound = true;
		}
		else if (!Q_strnicmp(text + i, "steambeta://", 12))
		{
			bURLFound = true;
		}
		else if (!Q_strnicmp(text + i, "mailto:", 7))
		{
			bURLFound = true;
		}
		else if (!Q_strnicmp(text + i, "\\\\", 2))
		{
			bURLFound = true;
		}
		
		if (bURLFound)
		{
			if (i == startPos)
			{
				// we're at the Start of a URL, so parse that out
				clickable = true;
				int outIndex = 0;
				while (text[i] != 0 && !iswspace(text[i]))
				{
					resultBuffer[outIndex++] = text[i++];
				}
				resultBuffer[outIndex] = 0;
				return i;
			}
			else
			{
				// no url
				break;
			}
		}
		
		// increment and loop
		i++;
	}
	
	// nothing found;
	// parse out the text before the end
	clickable = false;
	int outIndex = 0;
	int fromIndex = startPos;
	while (fromIndex < i && outIndex < resultBufferSize)
	{
		resultBuffer[outIndex++] = text[fromIndex++];
	}
	resultBuffer[outIndex] = 0;
	return i;
}

//-----------------------------------------------------------------------------
// Purpose: Opens the web browser with the text
//-----------------------------------------------------------------------------
void RichText::OnTextClicked(const wchar_t *wszText)
{
	char ansi[512];
	localize()->ConvertUnicodeToANSI(wszText, ansi, sizeof(ansi));

	system()->ShellExecute("open", ansi); 
}


//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
bool RichText::IsScrollbarVisible()
{
	return _vertScrollBar->IsVisible();
}


#ifdef DBGFLAG_VALIDATE
//-----------------------------------------------------------------------------
// Purpose: Run a global validation pass on all of our data structures and memory
//			allocations.
// Input:	validator -		Our global validator object
//			pchName -		Our name (typically a member var in our container)
//-----------------------------------------------------------------------------
void RichText::Validate( CValidator &validator, char *pchName )
{
	validator.Push( "vgui::RichText", this, pchName );

	m_TextStream.Validate( validator, "m_TextStream" );
	m_FormatStream.Validate( validator, "m_FormatStream" );
	m_LineBreaks.Validate( validator, "m_TextStream" );
	_clickableTextPanels.Validate( validator, "_clickableTextPanels" );
	validator.ClaimMemory( m_pszInitialText );

	BaseClass::Validate( validator, "vgui::RichText" );

	validator.Pop();
}
#endif // DBGFLAG_VALIDATE

