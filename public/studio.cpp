//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include "studio.h"
#include "datacache/idatacache.h"
#include "datacache/imdlcache.h"
#include "convar.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

mstudioanimdesc_t &studiohdr_t::pAnimdesc( int i ) const
{ 
	if (numincludemodels == 0)
	{
		return *pLocalAnimdesc( i );
	}

	virtualmodel_t *pVModel = (virtualmodel_t *)GetVirtualModel();
	Assert( pVModel );

	virtualgroup_t *pGroup = &pVModel->m_group[ pVModel->m_anim[i].group ];
	const studiohdr_t *pStudioHdr = pGroup->GetStudioHdr();
	Assert( pStudioHdr );

	return *pStudioHdr->pLocalAnimdesc( pVModel->m_anim[i].index );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

mstudioanim_t *mstudioanimdesc_t::pAnim( void ) const
{
	if (animblock == 0)
	{
		return  (mstudioanim_t *)(((byte *)this) + animindex);
	}
	else
	{
		byte *pAnimBlocks = pStudiohdr()->GetAnimBlock( animblock );
		
		if ( pAnimBlocks )
		{
			return (mstudioanim_t *)(pAnimBlocks + animindex);
		}
	}

	return NULL;
}

mstudioikrule_t *mstudioanimdesc_t::pIKRule( int i ) const
{
	if (ikruleindex)
	{
		return (mstudioikrule_t *)(((byte *)this) + ikruleindex) + i;
	}
	else if (animblockikruleindex)
	{
		if (animblock == 0)
		{
			return  (mstudioikrule_t *)(((byte *)this) + animblockikruleindex) + i;
		}
		else
		{
			byte *pAnimBlocks = pStudiohdr()->GetAnimBlock( animblock );
			
			if ( pAnimBlocks )
			{
				return (mstudioikrule_t *)(pAnimBlocks + animblockikruleindex) + i;
			}
		}
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

bool studiohdr_t::SequencesAvailable() const
{
	if (numincludemodels == 0)
	{
		return true;
	}

	return ( GetVirtualModel() != NULL );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

int studiohdr_t::GetNumSeq( void ) const
{
	if (numincludemodels == 0)
	{
		return numlocalseq;
	}

	virtualmodel_t *pVModel = (virtualmodel_t *)GetVirtualModel();
	Assert( pVModel );
	return pVModel->m_seq.Count();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

mstudioseqdesc_t &studiohdr_t::pSeqdesc( int i ) const
{
	if (numincludemodels == 0)
	{
		return *pLocalSeqdesc( i );
	}

	virtualmodel_t *pVModel = (virtualmodel_t *)GetVirtualModel();
	Assert( pVModel );

	if ( !pVModel )
	{
		return *pLocalSeqdesc( i );
	}

	virtualgroup_t *pGroup = &pVModel->m_group[ pVModel->m_seq[i].group ];
	const studiohdr_t *pStudioHdr = pGroup->GetStudioHdr();
	Assert( pStudioHdr );

	return *pStudioHdr->pLocalSeqdesc( pVModel->m_seq[i].index );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

int studiohdr_t::iRelativeAnim( int baseseq, int relanim ) const
{
	if (numincludemodels == 0)
	{
		return relanim;
	}

	virtualmodel_t *pVModel = (virtualmodel_t *)GetVirtualModel();
	Assert( pVModel );

	virtualgroup_t *pGroup = &pVModel->m_group[ pVModel->m_seq[baseseq].group ];

	return pGroup->masterAnim[ relanim ];
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

int studiohdr_t::iRelativeSeq( int baseseq, int relseq ) const
{
	if (numincludemodels == 0)
	{
		return relseq;
	}

	virtualmodel_t *pVModel = (virtualmodel_t *)GetVirtualModel();
	Assert( pVModel );

	virtualgroup_t *pGroup = &pVModel->m_group[ pVModel->m_seq[baseseq].group ];

	return pGroup->masterSeq[ relseq ];
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

int	studiohdr_t::GetNumPoseParameters( void ) const
{
	if (numincludemodels == 0)
	{
		return numlocalposeparameters;
	}

	virtualmodel_t *pVModel = (virtualmodel_t *)GetVirtualModel();
	Assert( pVModel );

	return pVModel->m_pose.Count();
}



//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

const mstudioposeparamdesc_t &studiohdr_t::pPoseParameter( int i ) const
{
	if (numincludemodels == 0)
	{
		return *pLocalPoseParameter( i );
	}

	virtualmodel_t *pVModel = (virtualmodel_t *)GetVirtualModel();
	Assert( pVModel );

	if ( pVModel->m_pose[i].group == 0)
		return *pLocalPoseParameter( pVModel->m_pose[i].index );

	virtualgroup_t *pGroup = &pVModel->m_group[ pVModel->m_pose[i].group ];

	const studiohdr_t *pStudioHdr = pGroup->GetStudioHdr();
	Assert( pStudioHdr );

	return *pStudioHdr->pLocalPoseParameter( pVModel->m_pose[i].index );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

int studiohdr_t::GetSharedPoseParameter( int iSequence, int iLocalPose ) const
{
	if (numincludemodels == 0)
	{
		return iLocalPose;
	}

	if (iLocalPose == -1)
		return iLocalPose;

	virtualmodel_t *pVModel = (virtualmodel_t *)GetVirtualModel();
	Assert( pVModel );

	virtualgroup_t *pGroup = &pVModel->m_group[ pVModel->m_seq[iSequence].group ];

	return pGroup->masterPose[iLocalPose];
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

int studiohdr_t::EntryNode( int iSequence ) const
{
	mstudioseqdesc_t &seqdesc = pSeqdesc( iSequence );

	if (numincludemodels == 0 || seqdesc.localentrynode == 0)
	{
		return seqdesc.localentrynode;
	}

	virtualmodel_t *pVModel = (virtualmodel_t *)GetVirtualModel();
	Assert( pVModel );

	virtualgroup_t *pGroup = &pVModel->m_group[ pVModel->m_seq[iSequence].group ];

	return pGroup->masterNode[seqdesc.localentrynode-1]+1;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------


int studiohdr_t::ExitNode( int iSequence ) const
{
	mstudioseqdesc_t &seqdesc = pSeqdesc( iSequence );

	if (numincludemodels == 0 || seqdesc.localexitnode == 0)
	{
		return seqdesc.localexitnode;
	}

	virtualmodel_t *pVModel = (virtualmodel_t *)GetVirtualModel();
	Assert( pVModel );

	virtualgroup_t *pGroup = &pVModel->m_group[ pVModel->m_seq[iSequence].group ];

	return pGroup->masterNode[seqdesc.localexitnode-1]+1;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

int	studiohdr_t::GetNumAttachments( void ) const
{
	if (numincludemodels == 0)
	{
		return numlocalattachments;
	}

	virtualmodel_t *pVModel = (virtualmodel_t *)GetVirtualModel();
	Assert( pVModel );

	return pVModel->m_attachment.Count();
}



//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

const mstudioattachment_t &studiohdr_t::pAttachment( int i ) const
{
	if (numincludemodels == 0)
	{
		return *pLocalAttachment( i );
	}

	virtualmodel_t *pVModel = (virtualmodel_t *)GetVirtualModel();
	Assert( pVModel );

	virtualgroup_t *pGroup = &pVModel->m_group[ pVModel->m_attachment[i].group ];
	const studiohdr_t *pStudioHdr = pGroup->GetStudioHdr();
	Assert( pStudioHdr );

	return *pStudioHdr->pLocalAttachment( pVModel->m_attachment[i].index );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

int	studiohdr_t::GetAttachmentBone( int i ) const
{
	const mstudioattachment_t &attachment = pAttachment( i );

	// remap bone
	virtualmodel_t *pVModel = GetVirtualModel();
	if (pVModel)
	{
		virtualgroup_t *pGroup = &pVModel->m_group[ pVModel->m_attachment[i].group ];
		int iBone = pGroup->masterBone[attachment.localbone];
		if (iBone == -1)
			return 0;
		return iBone;
	}
	return attachment.localbone;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

void studiohdr_t::SetAttachmentBone( int iAttachment, int iBone )
{
	mstudioattachment_t &attachment = (mstudioattachment_t &)pAttachment( iAttachment );

	// remap bone
	virtualmodel_t *pVModel = GetVirtualModel();
	if (pVModel)
	{
		virtualgroup_t *pGroup = &pVModel->m_group[ pVModel->m_attachment[iAttachment].group ];
		iBone = pGroup->boneMap[iBone];
	}
	attachment.localbone = iBone;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

char *studiohdr_t::pszNodeName( int iNode ) const
{
	if (numincludemodels == 0)
	{
		return pszLocalNodeName( iNode );
	}

	virtualmodel_t *pVModel = (virtualmodel_t *)GetVirtualModel();
	Assert( pVModel );

	if ( pVModel->m_node.Count() <= iNode-1 )
		return "Invalid node";

	return pVModel->m_group[ pVModel->m_node[iNode-1].group ].GetStudioHdr()->pszLocalNodeName( pVModel->m_node[iNode-1].index );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

int studiohdr_t::GetTransition( int iFrom, int iTo ) const
{
	if (numincludemodels == 0)
	{
		return *pLocalTransition( (iFrom-1)*numlocalnodes + (iTo - 1) );
	}

	return iTo;
	/*
	FIXME: not connected
	virtualmodel_t *pVModel = (virtualmodel_t *)GetVirtualModel();
	Assert( pVModel );

	return pVModel->m_transition.Element( iFrom ).Element( iTo );
	*/
}


int	studiohdr_t::GetActivityListVersion( void ) const
{
	if (numincludemodels == 0)
	{
		return activitylistversion;
	}

	virtualmodel_t *pVModel = (virtualmodel_t *)GetVirtualModel();
	Assert( pVModel );

	int version = activitylistversion;

	int i;
	for (i = 1; i < pVModel->m_group.Count(); i++)
	{
		virtualgroup_t *pGroup = &pVModel->m_group[ i ];
		const studiohdr_t *pStudioHdr = pGroup->GetStudioHdr();

		Assert( pStudioHdr );

		version = min( version, pStudioHdr->activitylistversion );
	}

	return version;
}

void studiohdr_t::SetActivityListVersion( int version ) const
{
	activitylistversion = version;

	if (numincludemodels == 0)
	{
		return;
	}

	virtualmodel_t *pVModel = (virtualmodel_t *)GetVirtualModel();
	Assert( pVModel );

	int i;
	for (i = 1; i < pVModel->m_group.Count(); i++)
	{
		virtualgroup_t *pGroup = &pVModel->m_group[ i ];
		const studiohdr_t *pStudioHdr = pGroup->GetStudioHdr();

		Assert( pStudioHdr );

		pStudioHdr->SetActivityListVersion( version );
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------


int studiohdr_t::GetNumIKAutoplayLocks( void ) const
{
	if (numincludemodels == 0)
	{
		return numlocalikautoplaylocks;
	}

	virtualmodel_t *pVModel = (virtualmodel_t *)GetVirtualModel();
	Assert( pVModel );

	return pVModel->m_iklock.Count();
}

const mstudioiklock_t &studiohdr_t::pIKAutoplayLock( int i ) const
{
	if (numincludemodels == 0)
	{
		return *pLocalIKAutoplayLock( i );
	}

	virtualmodel_t *pVModel = (virtualmodel_t *)GetVirtualModel();
	Assert( pVModel );

	virtualgroup_t *pGroup = &pVModel->m_group[ pVModel->m_iklock[i].group ];
	const studiohdr_t *pStudioHdr = pGroup->GetStudioHdr();
	Assert( pStudioHdr );

	return *pStudioHdr->pLocalIKAutoplayLock( pVModel->m_iklock[i].index );
}

int	studiohdr_t::CountAutoplaySequences() const
{
	int count = 0;
	for (int i = 0; i < GetNumSeq(); i++)
	{
		mstudioseqdesc_t &seqdesc = pSeqdesc( i );
		if (seqdesc.flags & STUDIO_AUTOPLAY)
		{
			count++;
		}
	}
	return count;
}

int	studiohdr_t::CopyAutoplaySequences( unsigned short *pOut, int outCount ) const
{
	int outIndex = 0;
	for (int i = 0; i < GetNumSeq() && outIndex < outCount; i++)
	{
		mstudioseqdesc_t &seqdesc = pSeqdesc( i );
		if (seqdesc.flags & STUDIO_AUTOPLAY)
		{
			pOut[outIndex] = i;
			outIndex++;
		}
	}
	return outIndex;
}

//-----------------------------------------------------------------------------
// Purpose:	maps local sequence bone to global bone
//-----------------------------------------------------------------------------

int	studiohdr_t::RemapSeqBone( int iSequence, int iLocalBone ) const	
{
	// remap bone
	virtualmodel_t *pVModel = GetVirtualModel();
	if (pVModel)
	{
		const virtualgroup_t *pSeqGroup = pVModel->pSeqGroup( iSequence );
		return pSeqGroup->masterBone[iLocalBone];
	}
	return iLocalBone;
}

int	studiohdr_t::RemapAnimBone( int iAnim, int iLocalBone ) const
{
	// remap bone
	virtualmodel_t *pVModel = GetVirtualModel();
	if (pVModel)
	{
		const virtualgroup_t *pAnimGroup = pVModel->pAnimGroup( iAnim );
		return pAnimGroup->masterBone[iLocalBone];
	}
	return iLocalBone;
}





















//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

CStudioHdr::CStudioHdr( void )
{
	// set pointer to bogus value
	m_nFrameUnlockCounter = 0;
	m_pFrameUnlockCounter = &m_nFrameUnlockCounter;
	Init( NULL );
}

CStudioHdr::CStudioHdr( IMDLCache *modelcache )
{
	SetModelCache( modelcache );
	Init( NULL );
}

CStudioHdr::CStudioHdr( const studiohdr_t *pStudioHdr, IMDLCache *modelcache )
{
	SetModelCache( modelcache );
	Init( pStudioHdr );
}


void CStudioHdr::SetModelCache( IMDLCache *modelcache )
{
	m_pFrameUnlockCounter = modelcache->GetFrameUnlockCounterPtr( MDLCACHE_STUDIOHDR );
}


// extern IDataCache *g_pDataCache;

void CStudioHdr::Init( const studiohdr_t *pStudioHdr )
{
	m_pStudioHdr = pStudioHdr;

	m_pVModel = NULL;
	m_pStudioHdrCache.RemoveAll();

	if (m_pStudioHdr == NULL)
	{
		// make sure the tag says it's not ready
		m_nFrameUnlockCounter = *m_pFrameUnlockCounter - 1;
		return;
	}

	m_nFrameUnlockCounter = *m_pFrameUnlockCounter;

	if (m_pStudioHdr->numincludemodels == 0)
	{
		return;
	}

	ResetVModel( m_pStudioHdr->GetVirtualModel() );

	return;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

bool CStudioHdr::SequencesAvailable() const
{
	if (m_pStudioHdr->numincludemodels == 0)
	{
		return true;
	}

	if (m_pVModel == NULL)
	{
		// repoll m_pVModel
		ResetVModel( m_pStudioHdr->GetVirtualModel() );
	}

	return ( m_pVModel != NULL );
}


void CStudioHdr::ResetVModel( const virtualmodel_t *pVModel ) const
{
	m_pVModel = (virtualmodel_t *)pVModel;

	if (m_pVModel != NULL)
	{
		m_pStudioHdrCache.SetCount( m_pVModel->m_group.Count() );

		int i;
		for (i = 0; i < m_pStudioHdrCache.Count(); i++)
		{
			m_pStudioHdrCache[ i ] = NULL;
		}
	}
}

const studiohdr_t *CStudioHdr::GroupStudioHdr( int i ) const
{
	const studiohdr_t *pStudioHdr = m_pStudioHdrCache[ i ];

	if (pStudioHdr == NULL)
	{
		virtualgroup_t *pGroup = &m_pVModel->m_group[ i ];
		pStudioHdr = pGroup->GetStudioHdr();
		m_pStudioHdrCache[ i ] = pStudioHdr;
	}

	Assert( pStudioHdr );
	return pStudioHdr;
}


const studiohdr_t *CStudioHdr::pSeqStudioHdr( int sequence ) const
{
	if (m_pVModel == NULL)
	{
		return m_pStudioHdr;
	}

	const studiohdr_t *pStudioHdr = GroupStudioHdr( m_pVModel->m_seq[sequence].group );

	return pStudioHdr;
}


const studiohdr_t *CStudioHdr::pAnimStudioHdr( int animation ) const
{
	if (m_pVModel == NULL)
	{
		return m_pStudioHdr;
	}

	const studiohdr_t *pStudioHdr = GroupStudioHdr( m_pVModel->m_anim[animation].group );

	return pStudioHdr;
}



mstudioanimdesc_t &CStudioHdr::pAnimdesc( int i ) const
{ 
	if (m_pVModel == NULL)
	{
		return *m_pStudioHdr->pLocalAnimdesc( i );
	}

	const studiohdr_t *pStudioHdr = GroupStudioHdr( m_pVModel->m_anim[i].group );

	return *pStudioHdr->pLocalAnimdesc( m_pVModel->m_anim[i].index );
}

#if 0

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

mstudioanim_t *mstudioanimdesc_t::pAnim( void ) const
{
	if (animblock == 0)
	{
		return  (mstudioanim_t *)(((byte *)this) + animindex);
	}
	else
	{
		byte *pAnimBlocks = pStudiohdr()->GetAnimBlock( animblock );
		
		if ( !pAnimBlocks )
		{
			Error( "Error loading .ani file info block for '%s'\n",
				pStudiohdr()->name );
		}
		
		return (mstudioanim_t *)(pAnimBlocks + animindex);
	}

	return NULL;
}

#endif

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

int CStudioHdr::GetNumSeq( void ) const
{
	if (m_pVModel == NULL)
	{
		return m_pStudioHdr->numlocalseq;
	}

	return m_pVModel->m_seq.Count();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

mstudioseqdesc_t &CStudioHdr::pSeqdesc( int i ) const
{
	Assert( i >= 0 && i < GetNumSeq() );
	
	if (m_pVModel == NULL)
	{
		return *m_pStudioHdr->pLocalSeqdesc( i );
	}

	const studiohdr_t *pStudioHdr = GroupStudioHdr( m_pVModel->m_seq[i].group );

	return *pStudioHdr->pLocalSeqdesc( m_pVModel->m_seq[i].index );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

int CStudioHdr::iRelativeAnim( int baseseq, int relanim ) const
{
	if (m_pVModel == NULL)
	{
		return relanim;
	}

	virtualgroup_t *pGroup = &m_pVModel->m_group[ m_pVModel->m_seq[baseseq].group ];

	return pGroup->masterAnim[ relanim ];
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

int CStudioHdr::iRelativeSeq( int baseseq, int relseq ) const
{
	if (m_pVModel == NULL)
	{
		return relseq;
	}

	Assert( m_pVModel );

	virtualgroup_t *pGroup = &m_pVModel->m_group[ m_pVModel->m_seq[baseseq].group ];

	return pGroup->masterSeq[ relseq ];
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

int	CStudioHdr::GetNumPoseParameters( void ) const
{
	if (m_pVModel == NULL)
	{
		return m_pStudioHdr->numlocalposeparameters;
	}

	Assert( m_pVModel );

	return m_pVModel->m_pose.Count();
}



//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

const mstudioposeparamdesc_t &CStudioHdr::pPoseParameter( int i ) const
{
	if (m_pVModel == NULL)
	{
		return *m_pStudioHdr->pLocalPoseParameter( i );
	}

	if ( m_pVModel->m_pose[i].group == 0)
		return *m_pStudioHdr->pLocalPoseParameter( m_pVModel->m_pose[i].index );

	const studiohdr_t *pStudioHdr = GroupStudioHdr( m_pVModel->m_pose[i].group );

	return *pStudioHdr->pLocalPoseParameter( m_pVModel->m_pose[i].index );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

int CStudioHdr::GetSharedPoseParameter( int iSequence, int iLocalPose ) const
{
	if (m_pVModel == NULL)
	{
		return iLocalPose;
	}

	if (iLocalPose == -1)
		return iLocalPose;

	Assert( m_pVModel );

	virtualgroup_t *pGroup = &m_pVModel->m_group[ m_pVModel->m_seq[iSequence].group ];

	return pGroup->masterPose[iLocalPose];
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

int CStudioHdr::EntryNode( int iSequence ) const
{
	mstudioseqdesc_t &seqdesc = pSeqdesc( iSequence );

	if (m_pVModel == NULL || seqdesc.localentrynode == 0)
	{
		return seqdesc.localentrynode;
	}

	Assert( m_pVModel );

	virtualgroup_t *pGroup = &m_pVModel->m_group[ m_pVModel->m_seq[iSequence].group ];

	return pGroup->masterNode[seqdesc.localentrynode-1]+1;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------


int CStudioHdr::ExitNode( int iSequence ) const
{
	mstudioseqdesc_t &seqdesc = pSeqdesc( iSequence );

	if (m_pVModel == NULL || seqdesc.localexitnode == 0)
	{
		return seqdesc.localexitnode;
	}

	Assert( m_pVModel );

	virtualgroup_t *pGroup = &m_pVModel->m_group[ m_pVModel->m_seq[iSequence].group ];

	return pGroup->masterNode[seqdesc.localexitnode-1]+1;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

int	CStudioHdr::GetNumAttachments( void ) const
{
	if (m_pVModel == NULL)
	{
		return m_pStudioHdr->numlocalattachments;
	}

	Assert( m_pVModel );

	return m_pVModel->m_attachment.Count();
}



//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

const mstudioattachment_t &CStudioHdr::pAttachment( int i ) const
{
	if (m_pVModel == NULL)
	{
		return *m_pStudioHdr->pLocalAttachment( i );
	}

	Assert( m_pVModel );

	const studiohdr_t *pStudioHdr = GroupStudioHdr( m_pVModel->m_attachment[i].group );

	return *pStudioHdr->pLocalAttachment( m_pVModel->m_attachment[i].index );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

int	CStudioHdr::GetAttachmentBone( int i ) const
{
	if (m_pVModel == 0)
	{
		return m_pStudioHdr->pLocalAttachment( i )->localbone;
	}

	virtualgroup_t *pGroup = &m_pVModel->m_group[ m_pVModel->m_attachment[i].group ];
	const mstudioattachment_t &attachment = pAttachment( i );
	int iBone = pGroup->masterBone[attachment.localbone];
	if (iBone == -1)
		return 0;
	return iBone;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

void CStudioHdr::SetAttachmentBone( int iAttachment, int iBone )
{
	mstudioattachment_t &attachment = (mstudioattachment_t &)m_pStudioHdr->pAttachment( iAttachment );

	// remap bone
	if (m_pVModel)
	{
		virtualgroup_t *pGroup = &m_pVModel->m_group[ m_pVModel->m_attachment[iAttachment].group ];
		iBone = pGroup->boneMap[iBone];
	}
	attachment.localbone = iBone;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

char *CStudioHdr::pszNodeName( int iNode ) const
{
	if (m_pVModel == NULL)
	{
		return m_pStudioHdr->pszLocalNodeName( iNode );
	}

	if ( m_pVModel->m_node.Count() <= iNode-1 )
		return "Invalid node";

	const studiohdr_t *pStudioHdr = GroupStudioHdr( m_pVModel->m_node[iNode-1].group );
	
	return pStudioHdr->pszLocalNodeName( m_pVModel->m_node[iNode-1].index );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

int CStudioHdr::GetTransition( int iFrom, int iTo ) const
{
	if (m_pVModel == NULL)
	{
		return *m_pStudioHdr->pLocalTransition( (iFrom-1)*m_pStudioHdr->numlocalnodes + (iTo - 1) );
	}

	return iTo;
	/*
	FIXME: not connected
	virtualmodel_t *pVModel = (virtualmodel_t *)GetVirtualModel();
	Assert( pVModel );

	return pVModel->m_transition.Element( iFrom ).Element( iTo );
	*/
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

int	CStudioHdr::GetActivityListVersion( void ) const
{
	if (m_pVModel == NULL)
	{
		return m_pStudioHdr->activitylistversion;
	}

	int version = m_pStudioHdr->activitylistversion;

	int i;
	for (i = 1; i < m_pVModel->m_group.Count(); i++)
	{
		const studiohdr_t *pStudioHdr = GroupStudioHdr( i );
		Assert( pStudioHdr );
		version = min( version, pStudioHdr->activitylistversion );
	}

	return version;
}

void CStudioHdr::SetActivityListVersion( int version )
{
	m_pStudioHdr->activitylistversion = version;

	if (m_pVModel == NULL)
	{
		return;
	}

	int i;
	for (i = 1; i < m_pVModel->m_group.Count(); i++)
	{
		const studiohdr_t *pStudioHdr = GroupStudioHdr( i );
		Assert( pStudioHdr );
		pStudioHdr->SetActivityListVersion( version );
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

int	CStudioHdr::GetEventListVersion( void ) const
{
	if (m_pVModel == NULL)
	{
		return m_pStudioHdr->eventsindexed;
	}

	int version = m_pStudioHdr->eventsindexed;

	int i;
	for (i = 1; i < m_pVModel->m_group.Count(); i++)
	{
		const studiohdr_t *pStudioHdr = GroupStudioHdr( i );
		Assert( pStudioHdr );
		version = min( version, pStudioHdr->eventsindexed );
	}

	return version;
}

void CStudioHdr::SetEventListVersion( int version )
{
	m_pStudioHdr->eventsindexed = version;

	if (m_pVModel == NULL)
	{
		return;
	}

	int i;
	for (i = 1; i < m_pVModel->m_group.Count(); i++)
	{
		const studiohdr_t *pStudioHdr = GroupStudioHdr( i );
		Assert( pStudioHdr );
		pStudioHdr->eventsindexed = version;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------


int CStudioHdr::GetNumIKAutoplayLocks( void ) const
{
	if (m_pVModel == NULL)
	{
		return m_pStudioHdr->numlocalikautoplaylocks;
	}

	return m_pVModel->m_iklock.Count();
}

const mstudioiklock_t &CStudioHdr::pIKAutoplayLock( int i ) const
{
	if (m_pVModel == NULL)
	{
		return *m_pStudioHdr->pLocalIKAutoplayLock( i );
	}

	const studiohdr_t *pStudioHdr = GroupStudioHdr( m_pVModel->m_iklock[i].group );
	Assert( pStudioHdr );
	return *pStudioHdr->pLocalIKAutoplayLock( m_pVModel->m_iklock[i].index );
}

#if 0
int	CStudioHdr::CountAutoplaySequences() const
{
	int count = 0;
	for (int i = 0; i < GetNumSeq(); i++)
	{
		mstudioseqdesc_t &seqdesc = pSeqdesc( i );
		if (seqdesc.flags & STUDIO_AUTOPLAY)
		{
			count++;
		}
	}
	return count;
}

int	CStudioHdr::CopyAutoplaySequences( unsigned short *pOut, int outCount ) const
{
	int outIndex = 0;
	for (int i = 0; i < GetNumSeq() && outIndex < outCount; i++)
	{
		mstudioseqdesc_t &seqdesc = pSeqdesc( i );
		if (seqdesc.flags & STUDIO_AUTOPLAY)
		{
			pOut[outIndex] = i;
			outIndex++;
		}
	}
	return outIndex;
}

#endif

//-----------------------------------------------------------------------------
// Purpose:	maps local sequence bone to global bone
//-----------------------------------------------------------------------------

int	CStudioHdr::RemapSeqBone( int iSequence, int iLocalBone ) const	
{
	// remap bone
	if (m_pVModel)
	{
		const virtualgroup_t *pSeqGroup = m_pVModel->pSeqGroup( iSequence );
		return pSeqGroup->masterBone[iLocalBone];
	}
	return iLocalBone;
}

int	CStudioHdr::RemapAnimBone( int iAnim, int iLocalBone ) const
{
	// remap bone
	if (m_pVModel)
	{
		const virtualgroup_t *pAnimGroup = m_pVModel->pAnimGroup( iAnim );
		return pAnimGroup->masterBone[iLocalBone];
	}
	return iLocalBone;
}

// JasonM hack
//ConVar	flex_maxrule( "flex_maxrule", "0" );

//-----------------------------------------------------------------------------
// Purpose: run the interpreted FAC's expressions, converting flex_controller 
//			values into FAC weights
//-----------------------------------------------------------------------------
void CStudioHdr::RunFlexRules( const float *src, float *dest )
{
	int i, j;

	// FIXME: this shouldn't be needed, flex without rules should be stripped in studiomdl
	for (i = 0; i < numflexdesc(); i++)
	{
		dest[i] = 0;
	}

	for (i = 0; i < numflexrules(); i++)
	{
		float stack[32];
		int k = 0;
		mstudioflexrule_t *prule = pFlexRule( i );

		mstudioflexop_t *pops = prule->iFlexOp( 0 );
/*
		// JasonM hack for flex perf testing...
		int nFlexRulesToRun = 0;								// 0 means run them all
		const char *pszExpression = flex_maxrule.GetString();
		if ( pszExpression )
		{
			nFlexRulesToRun = atoi(pszExpression);				// 0 will be returned if not a numeric string
		}
		// end JasonM hack
//*/
		// debugoverlay->AddTextOverlay( GetAbsOrigin() + Vector( 0, 0, 64 ), i + 1, 0, "%2d:%d\n", i, prule->flex );

		for (j = 0; j < prule->numops; j++)
		{
			switch (pops->op)
			{
			case STUDIO_ADD: stack[k-2] = stack[k-2] + stack[k-1]; k--; break;
			case STUDIO_SUB: stack[k-2] = stack[k-2] - stack[k-1]; k--; break;
			case STUDIO_MUL: stack[k-2] = stack[k-2] * stack[k-1]; k--; break;
			case STUDIO_DIV:
				if (stack[k-1] > 0.0001)
				{
					stack[k-2] = stack[k-2] / stack[k-1];
				}
				else
				{
					stack[k-2] = 0;
				}
				k--; 
				break;
			case STUDIO_NEG: stack[k-1] = -stack[k-1]; break;
			case STUDIO_MAX: stack[k-2] = max( stack[k-2], stack[k-1] ); k--; break;
			case STUDIO_MIN: stack[k-2] = min( stack[k-2], stack[k-1] ); k--; break;
			case STUDIO_CONST: stack[k] = pops->d.value; k++; break;
			case STUDIO_FETCH1: 
				{ 
				int m = pFlexcontroller(pops->d.index)->link;
				stack[k] = src[m];
				k++; 
				break;
				}
			case STUDIO_FETCH2: stack[k] = dest[pops->d.index]; k++; break;
			}

			pops++;
		}

		dest[prule->flex] = stack[0];
/*
		// JasonM hack
		if ( nFlexRulesToRun == 0)					// 0 means run all rules correctly
		{
			dest[prule->flex] = stack[0];
		}
		else // run only up to nFlexRulesToRun correctly...zero out the rest
		{
			if ( j < nFlexRulesToRun )
				dest[prule->flex] = stack[0];
			else
				dest[prule->flex] = 0.0f;
		}

		dest[prule->flex] = 1.0f;
//*/
		// end JasonM hack

	}
}
