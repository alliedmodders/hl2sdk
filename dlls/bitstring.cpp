//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:		Arbitrary length bit string
//				** NOTE: This class does NOT override the bitwise operators
//						 as doing so would require overriding the operators
//						 to allocate memory for the returned bitstring.  This method
//						 would be prone to memory leaks as the calling party
//						 would have to remember to delete the memory.  Funtions
//						 are used instead to require the calling party to allocate
//						 and destroy their own memory
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include <limits.h>

#include "bitstring.h"
#include "utlbuffer.h"
#include "tier0/dbg.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Init static vars
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Purpose: Calculate a mask for the last int in the array
// Input  : numBits - 
// Output : static int
//-----------------------------------------------------------------------------

unsigned g_BitStringEndMasks[] = 
{
	0x00000000,
	0xfffffffe,
	0xfffffffc,
	0xfffffff8,
	0xfffffff0,
	0xffffffe0,
	0xffffffc0,
	0xffffff80,
	0xffffff00,
	0xfffffe00,
	0xfffffc00,
	0xfffff800,
	0xfffff000,
	0xffffe000,
	0xffffc000,
	0xffff8000,
	0xffff0000,
	0xfffe0000,
	0xfffc0000,
	0xfff80000,
	0xfff00000,
	0xffe00000,
	0xffc00000,
	0xff800000,
	0xff000000,
	0xfe000000,
	0xfc000000,
	0xf8000000,
	0xf0000000,
	0xe0000000,
	0xc0000000,
	0x80000000,
};

//-----------------------------------------------------------------------------
// Purpose: Print bits for debugging purposes
// Input  :
// Output :
//-----------------------------------------------------------------------------

void DebugPrintBitStringBits( const int *pInts, int nInts )
{
	for (int i=0;i<nInts;i++) 
	{
		for (int j =0; j<BITS_PER_INT;j++) 
		{
			Msg( "%d", (pInts[i] & (1<<j)) ? 1:0);
		}
	}
	Msg( "\n");
}

//-----------------------------------------------------------------------------
// Purpose: Saves a bit string to the given file
// Input  :
// Output :
//-----------------------------------------------------------------------------
void SaveBitString(const int *pInts, int nInts, CUtlBuffer& buf)
{
	for (int i=0;i<nInts;i++) 
	{
		buf.Printf("%d ", pInts[i]);
	}
	buf.Printf("\n");
}

//-----------------------------------------------------------------------------
// Purpose: Loads a bit string from the given file
// Input  :
// Output :
//-----------------------------------------------------------------------------

void LoadBitString(int *pInts, int nInts, CUtlBuffer& buf)
{
	for (int i=0; i<nInts; i++) 
	{
		buf.Scanf("%d", &pInts[i]);
	}
}


//-----------------------------------------------------------------------------

void CVariableBitStringBase::ValidateOperand( const CVariableBitStringBase &operand ) const
{
	Assert(Size() == operand.Size());
}

//-----------------------------------------------------------------------------
// Purpose: Resizes the bit string to a new number of bits
// Input  : resizeNumBits - 
//-----------------------------------------------------------------------------
void CVariableBitStringBase::Resize( int resizeNumBits )
{
	Assert( resizeNumBits >= 0 && resizeNumBits <= USHRT_MAX );

	int newIntCount = CalcNumIntsForBits( resizeNumBits );
	if ( newIntCount != GetNumInts() )
	{
		if ( GetInts() )
		{
			ReallocInts( newIntCount );
			if ( resizeNumBits >= Size() )
			{
				GetInts()[GetNumInts() - 1] &= ~GetEndMask();
				memset( GetInts() + GetNumInts(), 0, (newIntCount - GetNumInts()) * sizeof(int) );
			}
		}
		else
		{
			// Figure out how many ints are needed
			AllocInts( newIntCount );

			// Initialize bitstring by clearing all bits
			memset( GetInts(), 0, newIntCount * sizeof(int) );
		}
		m_numInts = newIntCount;
	} 
	else if ( resizeNumBits >= Size() && GetInts() )
		GetInts()[GetNumInts() - 1] &= ~GetEndMask();

	// store the new size and end mask
	m_numBits = resizeNumBits;
}

//-----------------------------------------------------------------------------
// Purpose: Allocate the storage for the ints
// Input  : numInts - 
//-----------------------------------------------------------------------------
void CVariableBitStringBase::AllocInts( int numInts )
{
	Assert( !m_pInt );

	if ( numInts == 0 )
		return;

	if ( numInts == 1 )
	{
		m_pInt = &m_iBitStringStorage;
		return;
	}

	m_pInt = (int *)malloc( numInts * sizeof(int) );
}


//-----------------------------------------------------------------------------
// Purpose: Reallocate the storage for the ints
// Input  : numInts - 
//-----------------------------------------------------------------------------
void CVariableBitStringBase::ReallocInts( int numInts )
{
	Assert( GetInts() );
	if ( numInts == 0)
	{
		FreeInts();
		return;
	}

	if ( m_pInt == &m_iBitStringStorage )
	{
		if ( numInts != 1 )
		{
			m_pInt = ((int *)malloc( numInts * sizeof(int) ));
			*m_pInt = m_iBitStringStorage;
		}

		return;
	}

	if ( numInts == 1 )
	{
		m_iBitStringStorage = *m_pInt;
		free( m_pInt );
		m_pInt = &m_iBitStringStorage;
		return;
	}

	m_pInt = (int *)realloc( m_pInt,  numInts * sizeof(int) );
}


//-----------------------------------------------------------------------------
// Purpose: Free storage allocated with AllocInts
//-----------------------------------------------------------------------------
void CVariableBitStringBase::FreeInts( void )
{
	if ( m_numInts > 1 )
	{
		free( m_pInt );
	}
	m_pInt = NULL;
}

//-----------------------------------------------------------------------------

#ifdef DEBUG
CON_COMMAND(test_bitstring, "Tests the bit string class")
{
	// This function is only testing new features (toml 11-10-02)

	int i, j;

	// Variable sized
	CBitString one, two, three;

	for ( i = 0; i < 100; i++ )
	{
		one.Resize( random->RandomInt(1, USHRT_MAX ) );
	}

	one.Resize( 0 );

	one.Resize( 2 );

	for ( i = 0; i < 14; i++ )
	{
		for ( j = 0; j < 64; j++)
		{
			int bit = random->RandomInt(0, one.Size() - 1 );
			Assert(!one.GetBit(bit));
			one.SetBit( bit );
			Assert(one.GetBit(bit));
			one.ClearBit( bit );
			Assert(!one.GetBit(bit));
		}
		one.Resize( one.Size() * 2 );
	}
	
	one.Resize( 100 );
	one.SetBit( 50 );
	one.Resize( 101 );
	Assert(one.GetBit(50));
	one.Resize( 1010 );
	Assert(one.GetBit(50));
	one.Resize( 49 );
	one.Resize( 100 );
	Assert(!one.GetBit(50));

	Assert( one.IsAllClear() );
	two.Resize( one.Size() );
	one.Not( &two );
	Assert( !two.IsAllClear() );
	Assert( two.IsAllSet() );

	// Fixed sized
	CFixedBitString<1> fbs1;	Assert( fbs1.GetEndMask() == GetEndMask( fbs1.Size() ) );
	CFixedBitString<2> fbs2;	Assert( fbs2.GetEndMask() == GetEndMask( fbs2.Size() ) );
	CFixedBitString<3> fbs3;	Assert( fbs3.GetEndMask() == GetEndMask( fbs3.Size() ) );
	CFixedBitString<4> fbs4;	Assert( fbs4.GetEndMask() == GetEndMask( fbs4.Size() ) );
	CFixedBitString<5> fbs5;	Assert( fbs5.GetEndMask() == GetEndMask( fbs5.Size() ) );
	CFixedBitString<6> fbs6;	Assert( fbs6.GetEndMask() == GetEndMask( fbs6.Size() ) );
	CFixedBitString<7> fbs7;	Assert( fbs7.GetEndMask() == GetEndMask( fbs7.Size() ) );
	CFixedBitString<8> fbs8;	Assert( fbs8.GetEndMask() == GetEndMask( fbs8.Size() ) );
	CFixedBitString<9> fbs9;	Assert( fbs9.GetEndMask() == GetEndMask( fbs9.Size() ) );
	CFixedBitString<10> fbs10;	Assert( fbs10.GetEndMask() == GetEndMask( fbs10.Size() ) );
	CFixedBitString<11> fbs11;	Assert( fbs11.GetEndMask() == GetEndMask( fbs11.Size() ) );
	CFixedBitString<12> fbs12;	Assert( fbs12.GetEndMask() == GetEndMask( fbs12.Size() ) );
	CFixedBitString<13> fbs13;	Assert( fbs13.GetEndMask() == GetEndMask( fbs13.Size() ) );
	CFixedBitString<14> fbs14;	Assert( fbs14.GetEndMask() == GetEndMask( fbs14.Size() ) );
	CFixedBitString<15> fbs15;	Assert( fbs15.GetEndMask() == GetEndMask( fbs15.Size() ) );
	CFixedBitString<16> fbs16;	Assert( fbs16.GetEndMask() == GetEndMask( fbs16.Size() ) );
	CFixedBitString<17> fbs17;	Assert( fbs17.GetEndMask() == GetEndMask( fbs17.Size() ) );
	CFixedBitString<18> fbs18;	Assert( fbs18.GetEndMask() == GetEndMask( fbs18.Size() ) );
	CFixedBitString<19> fbs19;	Assert( fbs19.GetEndMask() == GetEndMask( fbs19.Size() ) );
	CFixedBitString<20> fbs20;	Assert( fbs20.GetEndMask() == GetEndMask( fbs20.Size() ) );
	CFixedBitString<21> fbs21;	Assert( fbs21.GetEndMask() == GetEndMask( fbs21.Size() ) );
	CFixedBitString<22> fbs22;	Assert( fbs22.GetEndMask() == GetEndMask( fbs22.Size() ) );
	CFixedBitString<23> fbs23;	Assert( fbs23.GetEndMask() == GetEndMask( fbs23.Size() ) );
	CFixedBitString<24> fbs24;	Assert( fbs24.GetEndMask() == GetEndMask( fbs24.Size() ) );
	CFixedBitString<25> fbs25;	Assert( fbs25.GetEndMask() == GetEndMask( fbs25.Size() ) );
	CFixedBitString<26> fbs26;	Assert( fbs26.GetEndMask() == GetEndMask( fbs26.Size() ) );
	CFixedBitString<27> fbs27;	Assert( fbs27.GetEndMask() == GetEndMask( fbs27.Size() ) );
	CFixedBitString<28> fbs28;	Assert( fbs28.GetEndMask() == GetEndMask( fbs28.Size() ) );
	CFixedBitString<29> fbs29;	Assert( fbs29.GetEndMask() == GetEndMask( fbs29.Size() ) );
	CFixedBitString<30> fbs30;	Assert( fbs30.GetEndMask() == GetEndMask( fbs30.Size() ) );
	CFixedBitString<31> fbs31;	Assert( fbs31.GetEndMask() == GetEndMask( fbs31.Size() ) );
	CFixedBitString<32> fbs32;	Assert( fbs32.GetEndMask() == GetEndMask( fbs32.Size() ) );
	CFixedBitString<33> fbs33;	Assert( fbs33.GetEndMask() == GetEndMask( fbs33.Size() ) );
	CFixedBitString<34> fbs34;	Assert( fbs34.GetEndMask() == GetEndMask( fbs34.Size() ) );
	CFixedBitString<35> fbs35;	Assert( fbs35.GetEndMask() == GetEndMask( fbs35.Size() ) );
}
#endif
