// Supported with union (c) 2020 Union team
// Union HEADER file

namespace GOTHIC_ENGINE {
  int GetTopDmgIndex( int damageArr[], int damagetype ) {
    int topDmgAmount = 0, topDmgIndex = 0;

    for ( int i = 0; i < (int)oEDamageIndex::oEDamageIndex_MAX; i++ )
      if ( (oEDamageType)(1 << (oEDamageIndex)i) & damagetype )
        if ( damageArr[i] >= topDmgAmount ) {
          topDmgAmount = damageArr[i];
          topDmgIndex = i;
        }

    return topDmgIndex;
  }

  int oCNpc::GetFistDamageIndex() {
    return GetTopDmgIndex( this->damage, this->damagetype );
  }

  int oCNpc::GetWeaponDamageIndex() {
    oCItem* weapon = this->GetWeapon();

    if ( !weapon )
      return Invalid;

    int index = GetTopDmgIndex( weapon->damage, weapon->damageTypes );

    if ( !weapon->HasFlag( ITM_CAT_FF ) || weapon->munition <= 0 )
      return index;

    oCItem* arrow = this->GetRightHand()->CastTo<oCItem>();

    if ( !arrow )
      return index;

    if ( arrow->instanz != weapon->munition )
      return index;

    return GetTopDmgIndex( arrow->damage, arrow->damageTypes );
  }

  int oCNpc::GetSpellDamageIndex() {
    oCMag_Book* book = this->mag_book;

    if ( !book )
      return Invalid;

    oCSpell* spell = book->GetSelectedSpell();

    if ( !spell )
      return Invalid;

    for ( int i = 0; i < oEIndexDamage::oEDamageIndex_MAX; i++ )
      if ( (oETypeDamage)(1 << (oEIndexDamage)i) & spell->damageType )
        return i;

    return Invalid;
  }

  int oCNpc::GetActiveDamageIndex() {
    if ( this->IsInFightMode_S( NPC_WEAPON_FIST ) )
      return this->GetFistDamageIndex();

    if ( this->IsInFightMode_S( NPC_WEAPON_MAG ) )
      return this->GetSpellDamageIndex();

    return this->GetWeaponDamageIndex();
  }
}