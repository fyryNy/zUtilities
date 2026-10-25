// Supported with union (c) 2020 Union team
// Union HEADER file

namespace GOTHIC_ENGINE {
  using DamageMask = std::bitset<oEDamageIndex::oEDamageIndex_MAX>;

  // Mapping between oEDamageType bit flags and oEIndexDamage enum values.
  // This is needed because:
  //   - oEDamageType uses bit flags (1, 2, 4, 8, ...)
  //   - oEIndexDamage uses sequential indices (0..7)
  struct DamageMap {
    oEDamageType type;
    oEIndexDamage index;
  };

  static constexpr DamageMap DAMAGE_MAP[] = {
    { oEDamageType_Barrier, oEDamageIndex_Barrier },
    { oEDamageType_Blunt,   oEDamageIndex_Blunt   },
    { oEDamageType_Edge,    oEDamageIndex_Edge    },
    { oEDamageType_Fire,    oEDamageIndex_Fire    },
    { oEDamageType_Fly,     oEDamageIndex_Fly     },
    { oEDamageType_Magic,   oEDamageIndex_Magic   },
    { oEDamageType_Point,   oEDamageIndex_Point   },
    { oEDamageType_Fall,    oEDamageIndex_Fall    }
  };

  class DamageMaskHelper {
    static DamageMask learnedRangedDamage;
    static int learnedWeaponInstance;
    static int learnedMunitionInstance;

    static oCItem* GetRangedMunition( oCNpc* npc, oCItem* weapon );

  public:
    static void MarkIntDamageType( const int& damageTypeMask, DamageMask& mask );
    static void MarkWeaponDamage( const oCItem* weapon, DamageMask& mask );
    static void MarkNaturalDamage( const oCNpc* npc, DamageMask& mask );
    static void MarkSpellDamage( const oCSpell* spell, DamageMask& mask );
    static void MarkRangedWeaponDamage( oCNpc* npc, oCItem* weapon, const DamageMask& fallback, DamageMask& mask );
    static void TrackRangedProjectile( oCNpc* npc, oCItem* projectile );
    static void ForgetRangedProjectile( const oCItem* projectile );
    static bool GetRangedDamageSource( const oCItem* source, int& weaponInstance, int& munitionInstance );
    static void RememberRangedDamage( int weaponInstance, int munitionInstance, int damageTypes );
    static void ClearRangedDamage();
    static bool ItemHasDistanceOrMunitionCategoryFlag( const oCItem* item );
  };
}
