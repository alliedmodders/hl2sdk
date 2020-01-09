#ifndef _IDATACENTER_H_
#define _IDATACENTER_H_

class IDatacenter;

#include "imatchsystem.h"

abstract_class IDatacenterCmdBatch
{
public:
	virtual void AddCommand( KeyValues * ) = 0;
	virtual bool IsFinished() = 0;
	virtual int GetNumResults() = 0;
	virtual void * GetResult( int iElem ) = 0;
	virtual void Destroy() = 0;
};

abstract_class IDatacenter
{
public:
	//
	// GetStats
	//	retrieves the last received datacenter stats
	//
	virtual KeyValues * GetStats() = 0;

	virtual IDatacenterCmdBatch * CreateCmdBatch() = 0;
};


#endif // _IDATACENTER_H_
