// Supported with union (c) 2020 Union team
// Union HEADER file

namespace GOTHIC_ENGINE {
  struct NpcProtectionStatus {
    bool immune;
    int value;
    oEIndexDamage damageIndex;
  };

  enum FocusStatusProtectionPlacement {
    NONE = 0,
    TOP = 1,
    RIGHT = 2,
    CLOSE = 3
  };

  enum TargetProtectionMode {
    Disabled = 0,
    CurrentWeapon = 1,
    All = 2
  };

  enum class ProtectionRenderMode {
    Disabled,
    Immortal,
    Normal
  };

  struct ProtectionModel {
    ProtectionRenderMode renderMode = ProtectionRenderMode::Disabled;
    bool isInFightMode = false;
    TargetProtectionMode targetProtectionMode = TargetProtectionMode::Disabled;
  };

  struct ProtectionLayout {
    FocusStatusProtectionPlacement placement = FocusStatusProtectionPlacement::NONE;
    int startX = 0;
    int startY = 0;
    int iconSize = 0;
    int margin = 0;
    int fontY = 0;
    int rowHeight = 0;
    int iconOffsetY = 0;
    int contentWidth = 0;
    int contentHeight = 0;
    int itemX[oEDamageIndex_MAX] = {};
    int itemY[oEDamageIndex_MAX] = {};
  };

  struct FocusNameLayout {
    int textWidth = 0;
    int fontY = 0;
    int iconsWidth = 0;
    int iconsHeight = 0;
  };

  struct ProtectionIconRenderData {
    zSTRING text;
    zSTRING texture;
    zCOLOR color;
  };

  class FocusStatusBar : public StatusBar {
  public:
    FocusStatusBar();
    bool NeedAdjustPosition( int x, int y, oCNpc* npc, const FocusNameLayout& nameLayout );
    void GetFocusNamePosition( int& x, int& y ) const;

    virtual bool Init() override;
    virtual void Clear() override;
    virtual void Loop() override;

  private:
    static constexpr int ProtectionCount = 7;
    static constexpr int ProtectionPlacementRightMargin = 250;

    static const oEIndexDamage ProtectionDamageOrderedIndexes[ProtectionCount];
    const zSTRING CrackedShieldTexture = zSTRING( "ICON_PROTECTIONS" ); // https://game-icons.net/1x1/lorc/cracked-shield.html

    oEIndexDamage activeDamageIndexes[oEDamageIndex_MAX]; // not `PROTECTION_COUNT` because of bound safety
    int activeDamageIndexesCount = 0;

    NpcProtectionStatus activeStatuses[oEDamageIndex_MAX]; // not `PROTECTION_COUNT` because of bound safety
    int activeStatusesCount = 0;

    ProtectionModel protectionModel = {};

    int valueOffsetY = 0;
    FocusNameLayout focusNameLayout;
    int focusNameX = 0;
    int focusNameY = 0;

    zSTRING ImmuneAbbreviation = "IMM";
    zSTRING iconCache[oEDamageIndex_MAX + 1];

    bool BuildProtectionModel( oCNpc* npc );
    void FillDamageIndexesBuffer();
    void BuildFightModeDamage( DamageMask& mask );
    void BuildNoFightModeDamage( DamageMask& mask );

    void BuildProtectionLayout( ProtectionLayout& layout );
    void AvoidFocusNameOverlap( const ProtectionLayout& layout );
    int GetFocusNameHeight() const;
    void GetOutsideValuePosition( int& x, int& y, int fontY, int textWidth ) const;
    FocusStatusProtectionPlacement GetProtectionPlacement() const;
    int GetHorizontalProtMargin( int fontY ) const;
    int GetVerticalProtMargin( int fontY ) const;

    void RenderProtectionWithLayout( const ProtectionLayout& layout );
    bool CanRenderProtectionStatus( oCNpc* npc, oEIndexDamage damageIndex ) const;

    ProtectionIconRenderData BuildIconRenderData( const NpcProtectionStatus& status, unsigned char alpha );
    void MoveFocusBar( int x, int y, oCNpc* npc );
    virtual void PrintValueOutside( zSTRING& str ) override;

    const zSTRING& GetIconNameByDamageIndex( const oEIndexDamage& index );
  };
}
