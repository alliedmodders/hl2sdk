#include "entitysystem.h"

// idk
typedef long __int64;

// In IDA a __int64 can even be a class
// still need to figure out wha this returns
__int64 CEntityClass::Unserialize(CEntityClass *const this, CEntityInstance *pEntity, const CEntityKeyValues *pKeyValues) {
  __int64 entity_index; // v8, e.g. ent_attachments entity_index
  const char* entity_className; // v7, e.g. weapon_glock

  // this gets turned into a string if it's trying to get printed out
  __int64 entity_classInstance; // v9, e.g. CWeaponGlock
  

}
