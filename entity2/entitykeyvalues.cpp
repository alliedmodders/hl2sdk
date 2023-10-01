#include "entity2/entitysystem.h"

void CEntityKeyValues::ReleaseAllComplexKeys() {
  CEntityKeyValues::EntityComplexKeyListElem_t *i;
  IEntityKeyComplex *m_pKey;
  int m_nRef;

  for ( i = this->m_pComplexKeys; i; i = i->m_pNext ) {
      while(1) { // ehhh...???
        m_pKey = i->m_pKey;
        m_nRef = i->m_pKey->m_nRef;
        i->m_pKey->m_nRef = m_nRef - 1;
        if ( m_nRef == 1 )
          break;
        i = i->m_pNext;
        if ( !i )
          this->m_pComplexKeys = 0LL;
      }
      (*m_pKey->_vptr_IEntityKeyComplex)(m_pKey);
    }
  }
}


void CEntityKeyValues::RemoveAllKeys() {
  CUtlLeanVectorFixedGrowableBase<unsigned int,9u,short int>::IndexType_t m_nAllocated;
  CUtlLeanVectorBase<CEntityKeyValues::KeyValueInfo_t,short int>::ElemType_t *m_pElements;
  uint16 m_flags;
}

// CEntityKeyValues::Serialize


__int64 CEntityKeyValues::Unserialize(CUtlBuffer *const buf) {
  
}


// CEntityKeyValues::UnserializeKeys from world renderer

// more
