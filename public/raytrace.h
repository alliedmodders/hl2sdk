// $Id:$

#ifndef RAYTRACE_H
#define RAYTRACE_H

#include <tier0/platform.h>
#include <vector.h>
#include <mathlib/ssemath.h>
#include <mathlib/lightdesc.h>
#include <assert.h>
#include <tier1/utlvector.h>
#include <mathlib.h>
#include <bspfile.h>

// fast SSE-ONLY ray tracing module. Based upon various "real time ray tracing" research.

class FourRays
{
public:
	FourVectors origin;
	FourVectors direction;

	inline void Check(void) const
	{
		// in order to be valid to trace as a group, all four rays must have the same signs in all
		// of their direction components
#ifndef NDEBUG
		for(int c=1;c<4;c++)
		{
			Assert(direction.X(0)*direction.X(c)>=0);
			Assert(direction.Y(0)*direction.Y(c)>=0);
			Assert(direction.Z(0)*direction.Z(c)>=0);
		}
#endif
	}
	// returns direction sign mask for 4 rays. returns -1 if the rays can not be traced as a
	// bundle.
	int CalculateDirectionSignMask(void) const;

};

/// The format a triangle is stored in for intersections. size of this structure is important.
/// This structure can be in one of two forms. Before the ray tracing environment is set up, the
/// ProjectedEdgeEquations hold the coordinates of the 3 vertices, for facilitating bounding box
/// checks needed while building the tree. afterwards, they are changed into the projected ege
/// equations for intersection purposes.
struct CacheOptimizedTriangle
{
	// this structure is 16longs=64 bytes for cache line packing.
	Vector N;												// plane equation
	float D;
	int32 triangleid;										// id of the triangle.

	Vector ProjectedEdgeEquations[3];						// A,B,C for each edge equation.  a
															// point is inside the triangle if
															// a*c1+b*c2+c is negative for all 3
															// edges.

	int32 CoordSelect0,CoordSelect1;						// the triangle is projected onto a 2d
	                                                        // plane for edge testing. These are
	                                                        // the indices (0..2) of the
	                                                        // coordinates preserved in the
	                                                        // projection

	void ChangeIntoIntersectionFormat(void);				// change information storage format for
	                                                        // computing intersections.

	int ClassifyAgainstAxisSplit(int split_plane, float split_value); // PLANECHECK_xxx below
	
};

#define PLANECHECK_POSITIVE 1
#define PLANECHECK_NEGATIVE -1
#define PLANECHECK_STRADDLING 0

#define KDNODE_STATE_XSPLIT 0								// this node is an x split
#define KDNODE_STATE_YSPLIT 1								// this node is a ysplit
#define KDNODE_STATE_ZSPLIT 2								// this node is a zsplit
#define KDNODE_STATE_LEAF 3									// this node is a leaf

struct CacheOptimizedKDNode
{
	// this is the cache intensive data structure. "Tricks" are used to fit it into 8 bytes:
	//
	// A) the right child is always stored after the left child, which means we only need one
	// pointer
	// B) The type of node (KDNODE_xx) is stored in the lower 2 bits of the pointer.

	int32 Children;											// child idx, or'ed with flags above

	float SplittingPlaneValue;								// for non-leaf nodes, the nodes on the
	                                                        // "high" side of the splitting plane
	                                                        // are on the right

	inline int NodeType(void) const

	{
		return Children & 3;
	}

	inline int32 TriangleIndexStart(void) const
	{
		assert(NodeType()==KDNODE_STATE_LEAF);
		return Children>>2;
	}

	inline int LeftChild(void) const
	{
		assert(NodeType()!=KDNODE_STATE_LEAF);
		return Children>>2;
	}

	inline int RightChild(void) const
	{
		return LeftChild()+1;
	}

	inline int NumberOfTrianglesInLeaf(void) const
	{
		assert(NodeType()==KDNODE_STATE_LEAF);
		return *((int32 *) &SplittingPlaneValue);
	}

	inline void SetNumberOfTrianglesInLeafNode(int n)
	{
		*((int32 *) &SplittingPlaneValue)=n;
	}

protected:


};


struct RayTracingSingleResult
{
	Vector surface_normal;									// surface normal at intersection
	int32 HitID;											// -1=no hit. otherwise, triangle index
	float HitDistance;										// distance to intersection
	float ray_length;										// leng of initial ray
};

struct RayTracingResult
{
	FourVectors surface_normal;								// surface normal at intersection
	__declspec(align(16)) int32 HitIds[4];					// -1=no hit. otherwise, triangle index
	__m128 HitDistance;										// distance to intersection
};


class RayTraceLight
{
public:
	FourVectors Position;
	FourVectors Intensity;
};


#define RTE_FLAGS_FAST_TREE_GENERATION 1

enum RayTraceLightingMode_t {
	DIRECT_LIGHTING,										// just dot product lighting
	DIRECT_LIGHTING_WITH_SHADOWS,						// with shadows
	GLOBAL_LIGHTING										// global light w/ shadows
};


class RayStream
{
	friend class RayTracingEnvironment;

	RayTracingSingleResult *PendingStreamOutputs[8][4];
	int n_in_stream[8];
	FourRays PendingRays[8];

public:
	RayStream(void)
	{
		memset(n_in_stream,0,sizeof(n_in_stream));
	}
};


class RayTracingEnvironment
{
public:
	uint32 Flags;											// RTE_FLAGS_xxx above
	FourVectors BackgroundColor;							//< color where no intersection
	CUtlVector<CacheOptimizedKDNode> OptimizedKDTree;		//< the packed kdtree. root is 0
	CUtlVector<CacheOptimizedTriangle> OptimizedTriangleList; //< the packed triangles
	CUtlVector<int32> TriangleIndexList;					//< the list of triangle indices.
	CUtlVector<LightDesc_t> LightList;						//< the list of lights
	CUtlVector<Vector> TriangleColors;						//< color of tries

public:
	RayTracingEnvironment(void)
	{
		BackgroundColor.DuplicateVector(Vector(1,0,0));		// red
		Flags=0;
	}
	// call AddTriangle to set up the world
	void AddTriangle(int32 id, const Vector &v1, const Vector &v2, const Vector &v3,
					 const Vector &color);

	void AddQuad(int32 id, const Vector &v1, const Vector &v2, const Vector &v3,
				 const Vector &v4,							// specify vertices in cw or ccw order
				 const Vector &color);

	// for ease of testing. 
	void AddAxisAlignedRectangularSolid(int id,Vector mincoord, Vector Maxcoord, 
										const Vector &color);


	// SetupAccelerationStructure to prepare for tracing
	void SetupAccelerationStructure(void);


	// lowest level intersection routine - fire 4 rays through the scene. all 4 rays must pass the
	// Check() function, and t extents must be initialized. skipid can be set to exclude a
	// particular id (such as the origin surface). This function finds the closest intersection.
	void Trace4Rays(const FourRays &rays, __m128 TMin, __m128 TMax,int DirectionSignMask,
					RayTracingResult *rslt_out,
					int32 skip_id=-1);

	// higher level intersection routine that handles computing the mask and handling rays which do not match in direciton sign
	void Trace4Rays(const FourRays &rays, __m128 TMin, __m128 TMax,
					RayTracingResult *rslt_out,
					int32 skip_id=-1);

	// compute virtual light sources to model inter-reflection
	void ComputeVirtualLightSources(void);


	// high level interface - pass viewing parameters, rendering flags, and a destination frame
	// buffer, and get a ray traced scene in 32-bit rgba format
	void RenderScene(int width, int height,					// width and height of desired rendering
					 int stride,							// actual width in pixels of target buffer
					 uint32 *output_buffer,					// pointer to destination 
					 Vector CameraOrigin,					// eye position
					 Vector ULCorner,						// word space coordinates of upper left
															// monitor corner
					 Vector URCorner,						// top right corner
					 Vector LLCorner,						// lower left
					 Vector LRCorner,						// lower right
					 RayTraceLightingMode_t lightmode=DIRECT_LIGHTING);

					 
	/// raytracing stream - lets you trace an array of rays by feeding them to this function.
	/// results will not be returned until FinishStream is called. This function handles sorting
	/// the rays by direction, tracing them 4 at a time, and de-interleaving the results.

	void AddToRayStream(RayStream &s,
						Vector const &start,Vector const &end,RayTracingSingleResult *rslt_out);

	inline void RayTracingEnvironment::FlushStreamEntry(RayStream &s,int msk);

	/// call this when you are done. handles all cleanup. After this is called, all rslt ptrs
	/// previously passed to AddToRaySteam will have been filled in.
	void FinishRayStream(RayStream &s);


	int MakeLeafNode(int first_tri, int last_tri);


	float CalculateCostsOfSplit(
		int split_plane,int32 const *tri_list,int ntris,
		Vector MinBound,Vector MaxBound, float &split_value,
		int &nleft, int &nright, int &nboth);
		
	void RefineNode(int node_number,int32 const *tri_list,int ntris,
						 Vector MinBound,Vector MaxBound, int depth);
	
	void CalculateTriangleListBounds(int32 const *tris,int ntris,
									 Vector &minout, Vector &maxout);

	void AddInfinitePointLight(Vector position,				// light center
							   Vector intensity);			// rgb amount

	// use the global variables set by LoadBSPFile to populated the RayTracingEnvironment with
	// faces.
	void InitializeFromLoadedBSP(void);

	void AddBSPFace(int id,dface_t const &face);


};



#endif
