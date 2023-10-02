#ifndef ENTITYKEYVALUES_H
#define ENTITYKEYVALUES_H

#include "entity2/entitysystem.h"

struct CEntityKeyValues {
    CUtlLeanVectorFixedGrowable<unsigned int, 9u, short int> m_keyHashes;
    CUtlLeanVector<CEntityKeyValues::KeyValueInfo_t, short int> m_keyValues;
    CUtlScratchMemoryPool m_memoryPool;
    CEntityKeyValues::EntityComplexKeyListElem_t* m_pComplexKeys;
    int16 m_nRefCount;
    int16 m_nQueuedForSpawnCount;
    CUtlVector<EntityIOConnectionDescFat_t, CUtlMemory<EntityIOConnectionDescFat_t, int> > m_connectionDescs;
}


struct CEntityKeyValues::CIterator {
    const CEntityKeyValues* m_pKeyValues;
    int m_nIndex;
};

struct CEntityKeyValues::EntityComplexKeyListElem_t {
    IEntityKeyComplex* m_pKey;
    CEntityKeyValues::EntityComplexKeyListElem_t* m_pNext;
}

struct CEntityKeyValues::KeyValueInfo_t {
    CEntityVariant m_value;
    const char* m_pAttributeName;
};


#endif // ENTITYKEYVALUES_H
