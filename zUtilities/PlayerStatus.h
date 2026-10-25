// Supported with union (c) 2020 Union team
// Union HEADER file

namespace GOTHIC_ENGINE {
  namespace Options {
    bool ShowGameTime, ShowMunitionAmount, ShowPickpocketIcon, UseTimeMultiplier, ShowHumanNpcXpRewardIcon;
    int ShowSystemTime, KeyTimeMultiplier, ShowTargetProtectionNoFight, ShowTargetProtectionInFight, TargetProtectionIconStyle;
    Array<float> TimeMultipliers;
    int TargetProtectionIconPosition, SaveReminder;
    bool HideTargetProtectionZeroValues, HideTargetProtectionFallDamage, HideTargetProtectionFlyDamage, HideTargetProtectionFireDamage;
    DamageMask DistanceWeaponDamageType;

    int ReadProtectionOption( const char* name, int defaultValue, int maximum ) {
      int value = zoptions->ReadInt( PLUGIN_NAME, name, defaultValue );
      if ( value < 0 || value > maximum ) {
        value = defaultValue;
        zoptions->WriteInt( PLUGIN_NAME, name, value, 0 );
      }
      return value;
    }

    void MigrateProtectionOptions() {
      bool hasLegacyOptions = zoptions->EntryExists( PLUGIN_NAME, "ShowTargetProtection" )
        || zoptions->EntryExists( PLUGIN_NAME, "ShowProtOnlyInFight" )
        || zoptions->EntryExists( PLUGIN_NAME, "ShowProtAllDamageTypes" );
      if ( !hasLegacyOptions )
        return;

      int oldMode = zoptions->ReadInt( PLUGIN_NAME, "ShowTargetProtection", TargetProtectionMode::CurrentWeapon );
      if ( oldMode < TargetProtectionMode::Disabled || oldMode > TargetProtectionMode::All )
        oldMode = TargetProtectionMode::CurrentWeapon;

      bool onlyInFight = zoptions->ReadBool( PLUGIN_NAME, "ShowProtOnlyInFight", true );
      bool allDamageTypes = zoptions->ReadBool( PLUGIN_NAME, "ShowProtAllDamageTypes", false );

      if ( !zoptions->EntryExists( PLUGIN_NAME, "ShowTargetProtectionInFight" ) )
        zoptions->WriteInt( PLUGIN_NAME, "ShowTargetProtectionInFight", oldMode, 0 );
      if ( !zoptions->EntryExists( PLUGIN_NAME, "ShowTargetProtectionNoFight" ) ) {
        // The old CurrentWeapon mode hid protections whenever weapons were sheathed.
        int noFightMode = oldMode == TargetProtectionMode::All && !onlyInFight
          ? TargetProtectionMode::All : TargetProtectionMode::Disabled;
        zoptions->WriteInt( PLUGIN_NAME, "ShowTargetProtectionNoFight", noFightMode, 0 );
      }
      if ( !zoptions->EntryExists( PLUGIN_NAME, "HideTargetProtectionZeroValues" ) )
        zoptions->WriteBool( PLUGIN_NAME, "HideTargetProtectionZeroValues", !allDamageTypes, 0 );
    }

    void LoadProtectionOptions() {
      MigrateProtectionOptions();
      ShowTargetProtectionNoFight = ReadProtectionOption( "ShowTargetProtectionNoFight", TargetProtectionMode::Disabled, TargetProtectionMode::All );
      ShowTargetProtectionInFight = ReadProtectionOption( "ShowTargetProtectionInFight", TargetProtectionMode::CurrentWeapon, TargetProtectionMode::All );
      TargetProtectionIconStyle = ReadProtectionOption( "TargetProtectionIconStyle", 0, 1 );
      TargetProtectionIconPosition = ReadProtectionOption( "TargetProtectionIconPosition", 0, 1 );
      HideTargetProtectionZeroValues = zoptions->ReadBool( PLUGIN_NAME, "HideTargetProtectionZeroValues", false );
      HideTargetProtectionFallDamage = zoptions->ReadBool( PLUGIN_NAME, "HideTargetProtectionFallDamage", false );
      HideTargetProtectionFlyDamage = zoptions->ReadBool( PLUGIN_NAME, "HideTargetProtectionFlyDamage", false );
      HideTargetProtectionFireDamage = zoptions->ReadBool( PLUGIN_NAME, "HideTargetProtectionFireDamage", false );

      // Configured fallback only; observed ranged damage is tracked separately.
      DistanceWeaponDamageType.reset();
      DistanceWeaponDamageType.set( oEDamageIndex_Point );
      if ( int iniDamage = zoptions->ReadInt( PLUGIN_NAME, "DistanceWeaponDamageType", 0 ) ) {
        DamageMask iniMask;
        DamageMaskHelper::MarkIntDamageType( iniDamage, iniMask );
        if ( iniMask.any() )
          DistanceWeaponDamageType = iniMask;
      }
    }

    void PlayerStatus() {
      ShowSystemTime = zoptions->ReadInt( PLUGIN_NAME, "ShowSystemTime", 0 );
      ShowGameTime = zoptions->ReadBool( PLUGIN_NAME, "ShowGameTime", false );
      ShowMunitionAmount = zoptions->ReadBool( PLUGIN_NAME, "ShowMunitionAmount", false );
#if ENGINE >= Engine_G2
      ShowPickpocketIcon = zoptions->ReadBool( PLUGIN_NAME, "ShowPickpocketIcon", true );
#endif
      ShowHumanNpcXpRewardIcon = zoptions->ReadBool( PLUGIN_NAME, "ShowHumanNpcXpRewardIcon", false );
      UseTimeMultiplier = zoptions->ReadBool( PLUGIN_NAME, "UseTimeMultiplier", false );
      KeyTimeMultiplier = GetEmulationKeyCode( zoptions->ReadString( PLUGIN_NAME, "KeyTimeMultiplier", "KEY_Z" ) );
      string MulString = A zoptions->ReadString( PLUGIN_NAME, "TimeMultipliers", "1.0|2.5" );

      Array<string> MulStrings = MulString.Split( "|" );
      TimeMultipliers.Clear();
      for ( uint i = 0; i < MulStrings.GetNum(); i++ ) {
        float multiplier = MulStrings[i].Shrink().ToReal32();
        if ( std::isfinite( multiplier ) && multiplier > 0.0f )
          TimeMultipliers.Insert( multiplier );
      }
      if ( !TimeMultipliers.GetNum() )
        TimeMultipliers.Insert( 1.0f );

      SaveReminder = zoptions->ReadInt( PLUGIN_NAME, "SaveReminder", 5 );

      LoadProtectionOptions();
    }
  }

  class PlayerStatus {
  private:
    HealthStatusBar* hpBar;
    ManaStatusBar* manaBar;
    SwimStatusBar* swimBar;
    uint multiplierIndex = 0;
    float lastAppliedMotion = 1.0f;
    bool speedSuppressed = false;
    bool ownsTimeMultiplier = false;
    int infoIcons = 0;
    std::chrono::high_resolution_clock::time_point lastSaveTime;

    void ShowSystemTime();
    void ShowGameTime();
    void ShowMunitionAmount();
    void ShowSaveReminder();
    void StatusBars();
    void FactorMotion();

  public:
    zCArray<oCInfo*> pickpocketInfos;
    zCArray<zSTRING> interStateFuncs;
    zCArray<zSTRING> stateFuncItems;
    oCItem* stateFuncItem;
    oCNpc* traderNpc;
    FocusStatusBar* focusBar;
    bool CanPickpocketNpc( oCNpc* npc );
    void GetPickpocketInfos();
    bool IsHumanNpcEligibleForXpReward( oCNpc* npc );
    bool CanChangeZtimer();
    bool KnowStateFunc( zCVob* vob );
    void TryAddStateFunc( zCVob* vob );
    void ResetTimeMultiplier();
    void ResetSaveReminder();
    void Clear();
    void Loop();
    void Archive( zCArchiver* ar );
    void Unarchive( zCArchiver* ar );
  };

  PlayerStatus playerStatus;
}
