/*****************************************************************************
 * 
 * Swizzler
 *
 * Purpose: To allow simple manipulations of a swizzled texture, without the 
 * hassle or overhead of unswizzling the whole thing in order to tweak a few 
 * points on the texture. This works with both 2D and 3D textures.
 * 
 * Notes: 
 *   Most of the time when messing with a texture, you will be incrementing
 *   by a constant value in each dimension.  Those deltas can be converted
 *   to an intermediate value via the SwizzleXXX(num) methods which can be
 *   used to quickly increment a dimension.
 *
 *   The type SWIZNUM is used to represent numbers returned by the SwizzleXXX()
 *   methods, also known as "intermediate values" in this documentation.
 * 
 *   Code in comments may be uncommented in order to provide some sort of 
 *   parameter sanity. It assures that any number passed to num will only 
 *   alter the dimension specified by dim.
 * 
 * Elements:
 *   
 *   m_u = texture map (converted) u coordinate
 *   m_v = texture map (converted) v coordinate
 *   m_w = texture map (converted) w coordinate
 * 
 *   m_MaskU = internal mask for u coordinate
 *   m_MaskV = internal mask for v coordinate
 *   m_MaskW = internal mask for w coordinate
 *
 *   m_Width = width of the texture this instance of the class has been initialized for
 *   m_Height = height of the texture this instance of the class has been initialized for
 *   m_Depth = depth of the texture this instance of the class has been initialized for
 * 
 * Methods:
 *   SWIZNUM SwizzleU(DWORD num) -- converts num to an intermediate value that
 *     can be used to modify the u coordinate
 *   SWIZNUM SwizzleV(DWORD num) -- converts num to an intermediate value that
 *     can be used to modify the v coordinate
 *   SWIZNUM SwizzleW(DWORD num) -- converts num to an intermediate value that
 *     can be used to modify the w coordinate
 *
 *   DWORD UnswizzleU(SWIZNUM index) -- takes an index to the swizzled texture, 
 *     and extracts & returns the u coordinate
 *   DWORD UnswizzleV(SWIZNUM index) -- takes an index to the swizzled texture, 
 *     and extracts & returns the v coordinate
 *   DWORD UnswizzleW(SWIZNUM index) -- takes an index to the swizzled texture, 
 *     and extracts & returns the w coordinate
 *
 *   SWIZNUM SetU(SWIZNUM num) -- sets the U coordinate to num, where num is an intermediate 
 *     value returned by Convert; returns num.
 *   SWIZNUM SetV(SWIZNUM num) -- sets the V coordinate to num, where num is an intermediate 
 *     value returned by Convert; returns num.
 *   SWIZNUM SetW(SWIZNUM num) -- sets the W coordinate to num, where num is an intermediate 
 *     value returned by Convert; returns num.
 *
 *   SWIZNUM AddU(SWIZNUM num) -- adds num to the U coordinate, where num is an intermediate value 
 *     returned by Convert; returns the new (swizzled) U coordinate
 *   SWIZNUM AddV(SWIZNUM num) -- adds num to the V coordinate, where num is an intermediate value 
 *     returned by Convert; returns the new (swizzled) V coordinate
 *   SWIZNUM AddW(SWIZNUM num) -- adds num to the W coordinate, where num is an intermediate value 
 *     returned by Convert; returns the new (swizzled) W coordinate
 *
 *   SWIZNUM SubU(SWIZNUM num) -- subtracts num from the U coordinate, where num is an intermediate value 
 *     returned by Convert; returns the new (swizzled) U coordinate
 *   SWIZNUM SubV(SWIZNUM num) -- subtracts num from the V coordinate, where num is an intermediate value 
 *     returned by Convert; returns the new (swizzled) V coordinate
 *   SWIZNUM SubW(SWIZNUM num) -- subtracts num from the W coordinate, where num is an intermediate value 
 *     returned by Convert; returns the new (swizzled) W coordinate
 *
 *   SWIZNUM IncU() -- increments the U coordinate by 1, returns the new (swizzled) U coordinate.
 *   SWIZNUM IncV() -- increments the V coordinate by 1, returns the new (swizzled) V coordinate.
 *   SWIZNUM IncW() -- increments the W coordinate by 1, returns the new (swizzled) W coordinate.
 *
 *   SWIZNUM DecU() -- decrements the U coordinate by 1, returns the new (swizzled) U coordinate.
 *   SWIZNUM DecV() -- decrements the V coordinate by 1, returns the new (swizzled) V coordinate.
 *   SWIZNUM DecW() -- decrements the W coordinate by 1, returns the new (swizzled) W coordinate.
 *
 *   SWIZNUM Get2() -- returns the index to the swizzled volume texture, based on 
 *     the U, and V coordinates, as modified by the previous methods.
 *
 *   SWIZNUM Get3() -- returns the index to the swizzled volume texture, based on 
 *     the U, V, and W coordinates, as modified by the previous methods.
 *
 * Performance:
 *   The algorithm used in most methods of this class require only Subtraction and a binary And
 *   operation to complete the operation. In the AddXXX methods, a negation, a subtraction, and two
 *   binary And operations are required. For this reason, the SubXXX methods are actually faster than
 *   AddXXX. Inc and Dec are roughly the same speed however.
 *
 ****************************************************************************/
#pragma once

#ifndef DWORD
typedef unsigned long DWORD;
#endif

typedef DWORD SWIZNUM;

class Swizzler 
{
public:

    // Dimensions of the texture
    DWORD m_Width;
    DWORD m_Height;
    DWORD m_Depth; 

    // Internal mask for each coordinate
    DWORD m_MaskU;
    DWORD m_MaskV;
    DWORD m_MaskW; 

    // Swizzled texture coordinates
    DWORD m_u;
    DWORD m_v;
    DWORD m_w;     

    Swizzler(): m_Width(0), m_Height(0), m_Depth(0),
        m_MaskU(0), m_MaskV(0), m_MaskW(0),
        m_u(0), m_v(0), m_w(0)
        { }

    // Initializes the swizzler
    Swizzler(
        DWORD width, 
        DWORD height, 
        DWORD depth
        )
    { 
        Init(width, height, depth);
    }

    void Init(
        DWORD width,
        DWORD height,
        DWORD depth
        )
    {
        m_Width = width; 
        m_Height = height; 
        m_Depth = depth;
        m_MaskU = 0;
        m_MaskV = 0;
        m_MaskW = 0;
        m_u = 0;
        m_v = 0;
        m_w = 0;

        DWORD i = 1;
        DWORD j = 1;
        DWORD k;

        do 
        {
            k = 0;
            if (i < width)   
            { 
                m_MaskU |= j;   
                k = (j<<=1);  
            }

            if (i < height)  
            { 
                m_MaskV |= j;   
                k = (j<<=1);  
            }

            if (i < depth)   
            {
                 m_MaskW |= j;   
                 k = (j<<=1);  
            }

            i <<= 1;
        } 
        while (k);
    }

    // Swizzles a texture coordinate
    SWIZNUM SwizzleU( 
        DWORD num 
        )
    {
        SWIZNUM r = 0;

        for (DWORD i = 1; i <= m_MaskU; i <<= 1) 
        {
            if (m_MaskU & i) 
            {
                r |= (num & i);
            }
            else
            {
                num <<= 1;
            }
        }

        return r;
    }

    SWIZNUM SwizzleV( 
        DWORD num 
        )
    {
        SWIZNUM r = 0;

        for (DWORD i = 1; i <= m_MaskV; i <<= 1) 
        {
            if (m_MaskV & i)
            {
                r |= (num & i);
            }
            else
            {
                num <<= 1;
            }
        }

        return r;
    }

    SWIZNUM SwizzleW( 
        DWORD num 
        )
    {
        SWIZNUM r = 0;

        for (DWORD i = 1; i <= m_MaskW; i <<= 1) 
        {
            if (m_MaskW & i)
            {
                r |= (num & i);
            }
            else
            {
                num <<= 1;
            }
        }

        return r;
    }

    SWIZNUM Swizzle(
        DWORD u, 
        DWORD v, 
        DWORD w
        )
    {
        return SwizzleU(u) | SwizzleV(v) | SwizzleW(w);
    }
    
    // Unswizzles a texture coordinate
    DWORD UnswizzleU( 
        SWIZNUM num
        )
    {
        DWORD r = 0;

        for (DWORD i = 1, j = 1; i; i <<= 1) 
        {
            if (m_MaskU & i)  
            {   
                r |= (num & j);   
                j <<= 1; 
            } 
            else               
            {   
                num >>= 1; 
            }
        }

        return r;
    }

    DWORD UnswizzleV( 
        SWIZNUM num 
        )
    {
        DWORD r = 0;

        for (DWORD i = 1, j = 1; i; i <<= 1) 
        {
            if (m_MaskV & i)  
            {   
                r |= (num & j);   
                j <<= 1; 
            } 
            else
            {   
                num >>= 1; 
            }
        }

        return r;
    }

    DWORD UnswizzleW( 
        SWIZNUM num 
        )
    {
        DWORD r = 0;

        for (DWORD i = 1, j = 1; i; i <<= 1) 
        {
            if (m_MaskW & i)  
            {   
                r |= (num & j);   
                j <<= 1; 
            } 
            else               
            {   
                num >>= 1; 
            }
        }

        return r;
    }

    // Sets a texture coordinate
    __forceinline SWIZNUM SetU(SWIZNUM num) { return m_u = num /* & m_MaskU */; }
    __forceinline SWIZNUM SetV(SWIZNUM num) { return m_v = num /* & m_MaskV */; }
    __forceinline SWIZNUM SetW(SWIZNUM num) { return m_w = num /* & m_MaskW */; }
    
    // Adds a value to a texture coordinate
    __forceinline SWIZNUM AddU(SWIZNUM num) { return m_u = ( m_u - ( (0-num) & m_MaskU ) ) & m_MaskU; }
    __forceinline SWIZNUM AddV(SWIZNUM num) { return m_v = ( m_v - ( (0-num) & m_MaskV ) ) & m_MaskV; }
    __forceinline SWIZNUM AddW(SWIZNUM num) { return m_w = ( m_w - ( (0-num) & m_MaskW ) ) & m_MaskW; }

    // Subtracts a value from a texture coordinate
    __forceinline SWIZNUM SubU(SWIZNUM num) { return m_u = ( m_u - num /* & m_MaskU */ ) & m_MaskU; }
    __forceinline SWIZNUM SubV(SWIZNUM num) { return m_v = ( m_v - num /* & m_MaskV */ ) & m_MaskV; }
    __forceinline SWIZNUM SubW(SWIZNUM num) { return m_w = ( m_w - num /* & m_MaskW */ ) & m_MaskW; }

    // Increments a texture coordinate
    __forceinline SWIZNUM IncU()              { return m_u = ( m_u - m_MaskU ) & m_MaskU; }
    __forceinline SWIZNUM IncV()              { return m_v = ( m_v - m_MaskV ) & m_MaskV; }
    __forceinline SWIZNUM IncW()              { return m_w = ( m_w - m_MaskW ) & m_MaskW; }

    // Decrements a texture coordinate
    __forceinline SWIZNUM DecU()              { return m_u = ( m_u - 1 ) & m_MaskU; }
    __forceinline SWIZNUM DecV()              { return m_v = ( m_v - 1 ) & m_MaskV; }
    __forceinline SWIZNUM DecW()              { return m_w = ( m_w - 1 ) & m_MaskW; }

    // Gets the current swizzled address for a 2D or 3D texture
    __forceinline SWIZNUM Get2D()          { return m_u | m_v; }
    __forceinline SWIZNUM Get3D()          { return m_u | m_v | m_w; }
};