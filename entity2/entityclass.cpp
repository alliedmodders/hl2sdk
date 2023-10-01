#include "entitysystem.h"

// idk
typedef long __int64;

// In IDA a __int64 can even be a class
// still need to figure out wha this returns
__int64 CEntityClass::Unserialize(CEntityClass *const this, CEntityInstance *pEntity, const CEntityKeyValues *pKeyValues) {
  __int64 entity_index; // v8, e.g. ent_attachments entity_index
  const char *m_pAsString; // v7, e.g. weapon_glock
  const char *m_pszCPPClassname; // v9, e.g. CWeaponGlock
  

}
