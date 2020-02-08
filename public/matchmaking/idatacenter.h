#ifndef _IDATACENTER_H_
#define _IDATACENTER_H_

class IDatacenter;

#include "imatchsystem.h"

abstract_class IDatacenter
{
public:
	//
	// GetStats
	//	retrieves the last received datacenter stats
	//
	virtual KeyValues * GetStats() = 0;
};


#endif // _IDATACENTER_H_
