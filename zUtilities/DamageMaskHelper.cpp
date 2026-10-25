// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
  DamageMask DamageMaskHelper::learnedRangedDamage;
  int DamageMaskHelper::learnedWeaponInstance = -1;
  int DamageMaskHelper::learnedMunitionInstance = -1;

  struct RangedProjectileSource {
    int weaponInstance;
    int munitionInstance;
    unsigned long long sequence;
  };
  static std::unordered_map<const oCItem*, RangedProjectileSource> rangedProjectileSources;
  static unsigned long long nextProjectileSequence = 0;
  static constexpr size_t MaxTrackedRangedProjectiles = 128;

  void DamageMaskHelper::MarkIntDamageType( const int& damageTypeMask, DamageMask& mask ) {
    for ( const auto& mapItem : DAMAGE_MAP ) {
      if ( damageTypeMask & mapItem.type ) {
        mask.set( mapItem.index );
      }
    }
  }

  void DamageMaskHelper::MarkWeaponDamage( const oCItem* weapon, DamageMask& mask ) {
    if ( !weapon ) {
      return;
    }

    // The engine applies protections only for declared damage types. A damage
    // array can contain inactive entries, and active physical types can receive
    // attribute bonuses even when their item damage is zero.
    DamageMask weaponMask;
    MarkIntDamageType( weapon->damageTypes, weaponMask );
    if ( weaponMask.any() ) {
      mask |= weaponMask;
      return;
    }

    // Preserve support for mod items with missing type metadata.
    for ( int i = 0; i < oEDamageIndex::oEDamageIndex_MAX; ++i ) {
      if ( weapon->damage[i] > 0 )
        mask.set( i );
    }
  }

  bool DamageMaskHelper::ItemHasDistanceOrMunitionCategoryFlag( const oCItem* item ) {
    return item && ( item->mainflag & ( ITM_CAT_FF | ITM_CAT_MUN ) );
  }

  void DamageMaskHelper::MarkNaturalDamage( const oCNpc* npc, DamageMask& mask ) {
    DamageMask naturalMask;
    if ( npc ) {
      MarkIntDamageType( npc->damagetype, naturalMask );
      if ( naturalMask.none() ) {
        for ( int i = 0; i < oEDamageIndex_MAX; ++i ) {
          if ( npc->damage[i] > 0 )
            naturalMask.set( i );
        }
      }
    }
    if ( naturalMask.none() )
      naturalMask.set( oEDamageIndex_Blunt );
    mask |= naturalMask;
  }

  void DamageMaskHelper::MarkSpellDamage( const oCSpell* spell, DamageMask& mask ) {
    if ( !spell )
      return;

    DamageMask spellMask;
    MarkIntDamageType( spell->damageType, spellMask );
#if ENGINE <= Engine_G1A
    // G1 utility spells default to Blunt, but explicitly damaging spells may use it too.
    if ( spell->damagePerLevel <= 0 )
      spellMask.reset( oEDamageIndex_Blunt );
#endif
    mask |= spellMask;
  }

  oCItem* DamageMaskHelper::GetRangedMunition( oCNpc* npc, oCItem* weapon ) {
    if ( !npc || !weapon || weapon->munition <= 0 )
      return nullptr;

    oCVob* hands[] = { npc->GetLeftHand(), npc->GetRightHand() };
    for ( auto hand : hands ) {
      if ( auto item = hand ? hand->CastTo<oCItem>() : nullptr ) {
        if ( item->instanz == weapon->munition )
          return item;
      }
    }

    return npc->IsInInv( weapon->munition, 1 );
  }

  void DamageMaskHelper::MarkRangedWeaponDamage( oCNpc* npc, oCItem* weapon, const DamageMask& fallback, DamageMask& mask ) {
    if ( !weapon )
      return;

    if ( learnedRangedDamage.any() && learnedWeaponInstance == weapon->instanz && learnedMunitionInstance == weapon->munition ) {
      mask |= learnedRangedDamage;
      return;
    }

    DamageMask rangedMask;
    MarkWeaponDamage( GetRangedMunition( npc, weapon ), rangedMask );
    if ( rangedMask.none() )
      MarkWeaponDamage( weapon, rangedMask );

    mask |= rangedMask.any() ? rangedMask : fallback;
  }

  void DamageMaskHelper::RememberRangedDamage( int weaponInstance, int munitionInstance, int damageTypes ) {
    if ( weaponInstance < 0 )
      return;

    DamageMask observedMask;
    MarkIntDamageType( damageTypes, observedMask );
    if ( observedMask.none() )
      return;

    learnedWeaponInstance = weaponInstance;
    learnedMunitionInstance = munitionInstance;
    learnedRangedDamage = observedMask;
  }

  void DamageMaskHelper::TrackRangedProjectile( oCNpc* npc, oCItem* projectile ) {
    ForgetRangedProjectile( projectile );
    if ( !npc || !projectile )
      return;

    if ( auto weapon = npc->GetEquippedRangedWeapon() ) {
      if ( weapon->munition == projectile->instanz ) {
        // Scripted removal can bypass DoAI. Bound entries left by those arrows,
        // evicting the oldest observation while preserving recent shots.
        if ( rangedProjectileSources.size() >= MaxTrackedRangedProjectiles ) {
          auto oldest = rangedProjectileSources.begin();
          for ( auto it = rangedProjectileSources.begin(); it != rangedProjectileSources.end(); ++it ) {
            if ( it->second.sequence < oldest->second.sequence )
              oldest = it;
          }
          rangedProjectileSources.erase( oldest );
        }
        rangedProjectileSources[projectile] = { weapon->instanz, projectile->instanz, nextProjectileSequence++ };
      }
    }
  }

  void DamageMaskHelper::ForgetRangedProjectile( const oCItem* projectile ) {
    rangedProjectileSources.erase( projectile );
  }

  bool DamageMaskHelper::GetRangedDamageSource( const oCItem* source, int& weaponInstance, int& munitionInstance ) {
    weaponInstance = -1;
    munitionInstance = -1;
    if ( !ItemHasDistanceOrMunitionCategoryFlag( source ) )
      return false;

    if ( source->mainflag & ITM_CAT_FF ) {
      weaponInstance = source->instanz;
      munitionInstance = source->munition;
      return true;
    }

    auto shot = rangedProjectileSources.find( source );
    if ( shot == rangedProjectileSources.end() )
      return false;

    weaponInstance = shot->second.weaponInstance;
    munitionInstance = shot->second.munitionInstance;
    rangedProjectileSources.erase( shot );
    return true;
  }

  void DamageMaskHelper::ClearRangedDamage() {
    learnedRangedDamage.reset();
    learnedWeaponInstance = -1;
    learnedMunitionInstance = -1;
    rangedProjectileSources.clear();
    nextProjectileSequence = 0;
  }
}
