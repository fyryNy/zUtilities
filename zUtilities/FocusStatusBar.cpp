// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
  const oEIndexDamage FocusStatusBar::ProtectionDamageOrderedIndexes[ProtectionCount] = {
    oEDamageIndex_Edge,
    oEDamageIndex_Blunt,
    oEDamageIndex_Point,
    oEDamageIndex_Fire,
    oEDamageIndex_Magic,
    oEDamageIndex_Fly,
    oEDamageIndex_Fall
  };

  HOOK Ivk_OnDamage_Hit_DistanceWeapon PATCH( &oCNpc::OnDamage_Hit, &oCNpc::OnDamage_Hit_DistanceWeapon );
  void oCNpc::OnDamage_Hit_DistanceWeapon( oSDamageDescriptor& desc ) {
    // Capture item identities before the hit can consume or release the projectile.
    int weaponInstance = -1;
    int munitionInstance = -1;
    if ( player && desc.pNpcAttacker == player ) {
      DamageMaskHelper::GetRangedDamageSource( desc.pItemWeapon, weaponInstance, munitionInstance );
    }

    THISCALL( Ivk_OnDamage_Hit_DistanceWeapon )( desc );
    DamageMaskHelper::RememberRangedDamage( weaponInstance, munitionInstance, desc.enuModeDamage );
  }

  HOOK Ivk_SetupAIVob_Protection PATCH( &oCAIArrow::SetupAIVob, &oCAIArrow::SetupAIVob_Protection );
  void oCAIArrow::SetupAIVob_Protection( zCVob* vob, zCVob* ownerVob, zCVob* target ) {
    DamageMaskHelper::ForgetRangedProjectile( arrow );
    THISCALL( Ivk_SetupAIVob_Protection )( vob, ownerVob, target );
    DamageMaskHelper::TrackRangedProjectile( owner == player ? owner : nullptr, arrow );
  }

  HOOK Ivk_DoAI_Protection PATCH( &oCAIArrow::DoAI, &oCAIArrow::DoAI_Protection );
  void oCAIArrow::DoAI_Protection( zCVob* vob, int& vobDeleted ) {
    // DoAI can detach and destroy this AI. Only use the saved key afterwards.
    const oCItem* projectile = arrow;
    THISCALL( Ivk_DoAI_Protection )( vob, vobDeleted );
    if ( vobDeleted )
      DamageMaskHelper::ForgetRangedProjectile( projectile );
  }

  HOOK Ivk_EquipItem_Union PATCH( &oCNpc::EquipItem, &oCNpc::EquipItem_Union );
  void oCNpc::EquipItem_Union( oCItem* item ) {
    const bool resetRangedDamage = this == player && DamageMaskHelper::ItemHasDistanceOrMunitionCategoryFlag( item );
    THISCALL( Ivk_EquipItem_Union )( item );

    if ( resetRangedDamage )
      DamageMaskHelper::ClearRangedDamage();
  }

  FocusStatusBar::FocusStatusBar() : StatusBar( ogame->focusBar ) {
    iconCache[oEDamageIndex_Barrier] = "DMGICON_UNKNOWN";
    iconCache[oEDamageIndex_Blunt] = "DMGICON_BLUNT";     // https://game-icons.net/1x1/lorc/cross-mark.html
    iconCache[oEDamageIndex_Edge] = "DMGICON_EDGE";       // https://game-icons.net/1x1/lorc/ragged-wound.html
    iconCache[oEDamageIndex_Fire] = "DMGICON_FIRE";       // https://game-icons.net/1x1/lorc/burning-embers.html
    iconCache[oEDamageIndex_Fly] = "DMGICON_FLY";         // https://game-icons.net/1x1/lorc/fluffy-trefoil.html
    iconCache[oEDamageIndex_Magic] = "DMGICON_MAGIC";     // https://game-icons.net/1x1/delapouite/polar-star.html
    iconCache[oEDamageIndex_Point] = "DMGICON_POINT";     // https://game-icons.net/1x1/skoll/bullseye.html
    iconCache[oEDamageIndex_Fall] = "DMGICON_FALL";       // https://game-icons.net/1x1/sbed/falling.html
    iconCache[oEDamageIndex_MAX] = "DMGICON_UNKNOWN";     // https://game-icons.net/1x1/lorc/scar-wound.html
  }

  bool FocusStatusBar::NeedAdjustPosition( int x, int y, oCNpc* npc, const FocusNameLayout& nameLayout ) {
    focusNameLayout = nameLayout;
    focusNameX = x;
    focusNameY = y;
    if ( !bar || !npc || npc->attribute[NPC_ATR_HITPOINTS] <= 0 )
      return false;

    valueOffsetY = 0;
    MoveFocusBar( x, y, npc );

    ProtectionLayout layout;
    const bool showProtection = BuildProtectionModel( npc );
    if ( showProtection ) {
      BuildProtectionLayout( layout );

      if ( Options::ShowEnemyBarAboveHim && layout.placement == FocusStatusProtectionPlacement::TOP ) {
        valueOffsetY = layout.startY - bar->vposy;
      }

    }

    AvoidFocusNameOverlap( layout );
    if ( showProtection )
      RenderProtectionWithLayout( layout );

    PrintValue();
    return Options::ShowEnemyBarAboveHim;
  }

  bool FocusStatusBar::Init() {
    return true;
  }

  void FocusStatusBar::Clear() {
    __super::Clear();
  }

  void FocusStatusBar::Loop() {
    if ( valueView ) {
      valueView->ClrPrintwin();
    }
  }

  bool FocusStatusBar::BuildProtectionModel( oCNpc* npc ) {
    if ( !npc || !player )
      return false;

    activeStatusesCount = 0;
    protectionModel.isInFightMode = player->fmode != 0;
    protectionModel.targetProtectionMode = protectionModel.isInFightMode
      ? ( TargetProtectionMode )Options::ShowTargetProtectionInFight
      : ( TargetProtectionMode )Options::ShowTargetProtectionNoFight;

    if ( protectionModel.targetProtectionMode == TargetProtectionMode::Disabled )
      return false;

    if ( npc->HasFlag( NPC_FLAG_IMMORTAL ) ) {
      protectionModel.renderMode = ProtectionRenderMode::Immortal;
      return true;
    }

    FillDamageIndexesBuffer();

    for ( int i = 0; i < activeDamageIndexesCount; ++i ) {
      oEIndexDamage index = activeDamageIndexes[i];

      if ( CanRenderProtectionStatus( npc, index ) ) {
        NpcProtectionStatus& status = activeStatuses[activeStatusesCount++];
        status.value = npc->GetProtectionByIndex( index );
        status.damageIndex = index;
        status.immune = status.value < 0;
      }
    }
    protectionModel.renderMode = ProtectionRenderMode::Normal;

    return activeStatusesCount > 0;
  }

  void FocusStatusBar::FillDamageIndexesBuffer() {
    activeDamageIndexesCount = 0;

    if ( protectionModel.targetProtectionMode == TargetProtectionMode::All ) {
      for ( int i = 0; i < ProtectionCount; ++i ) {
        activeDamageIndexes[activeDamageIndexesCount++] = ProtectionDamageOrderedIndexes[i];
      }
      return;
    }

    DamageMask mask;
    mask.reset();

    if ( protectionModel.isInFightMode ) {
      BuildFightModeDamage( mask );
    }
    else {
      BuildNoFightModeDamage( mask );
    }

    for ( int i = 0; i < oEDamageIndex_MAX; i++ ) {
      if ( mask.test( i ) ) {
        activeDamageIndexes[activeDamageIndexesCount++] = ( oEIndexDamage )i;
      }
    }
  }

  void FocusStatusBar::BuildFightModeDamage( DamageMask& mask ) {
    if ( player->IsInFightMode_S( NPC_WEAPON_FIST ) ) {
      DamageMaskHelper::MarkNaturalDamage( player, mask );
      return;
    }

    // Check for active spell
    if ( player->IsInFightMode_S( NPC_WEAPON_MAG ) ) {
      if ( player->mag_book ) {
        if ( auto spell = player->mag_book->GetSelectedSpell() ) {
          DamageMaskHelper::MarkSpellDamage( spell, mask );
        }
      }
      return;
    }

    // Check for active melee/distance weapon
    if ( auto weapon = player->GetWeapon() ) {
      if ( DamageMaskHelper::ItemHasDistanceOrMunitionCategoryFlag( weapon ) ) {
        DamageMaskHelper::MarkRangedWeaponDamage( player, weapon, Options::DistanceWeaponDamageType, mask );
      }
      else {
        DamageMaskHelper::MarkWeaponDamage( weapon, mask );
      }
    }
    else {
      DamageMaskHelper::MarkNaturalDamage( player, mask );
    }
  }

  void FocusStatusBar::BuildNoFightModeDamage( DamageMask& mask ) {
    bool hasAnyWeapon = false;

    // Check for melee weapon
    if ( auto weapon = player->GetEquippedMeleeWeapon() ) {
      DamageMaskHelper::MarkWeaponDamage( weapon, mask );
      hasAnyWeapon = true;
    }

    // Check for distance weapon
    if ( auto weapon = player->GetEquippedRangedWeapon() ) {
      DamageMaskHelper::MarkRangedWeaponDamage( player, weapon, Options::DistanceWeaponDamageType, mask );
      hasAnyWeapon = true;
    }

    // Check for all selected spells
    if ( player->mag_book ) {
      auto& spells = player->mag_book->spells;
      for ( int i = 0; i < spells.GetNum(); ++i ) {
        if ( spells[i] ) {
          DamageMaskHelper::MarkSpellDamage( spells[i], mask );
          hasAnyWeapon = true;
        }
      }
    }

    if ( !hasAnyWeapon ) {
      DamageMaskHelper::MarkNaturalDamage( player, mask );
    }
  }

  void FocusStatusBar::BuildProtectionLayout( ProtectionLayout& layout ) {
    layout = {};
    layout.fontY = screen->FontY();
    layout.iconSize = static_cast<int>( layout.fontY * 0.75f );
    // IconInfo draws a square in pixels, so its virtual height depends on the
    // aspect ratio. Allow one pixel for rounding at the final draw position.
    const int iconHeight = screen->any( screen->nax( layout.iconSize ) );
    const int roundingSpace = screen->any( 1 ) + 1;
    const bool immortal = protectionModel.renderMode == ProtectionRenderMode::Immortal;
    const int count = immortal ? 1 : activeStatusesCount;
    layout.placement = immortal ? FocusStatusProtectionPlacement::CLOSE : GetProtectionPlacement();
    if ( layout.placement == FocusStatusProtectionPlacement::NONE )
      return;

    const int horizontalMargin = GetHorizontalProtMargin( layout.fontY );
    const int verticalMargin = GetVerticalProtMargin( layout.fontY );
    layout.rowHeight = ( immortal ? iconHeight : max( iconHeight, layout.fontY ) ) + 2 * roundingSpace;
    layout.iconOffsetY = ( layout.rowHeight - iconHeight ) / 2;

    int widths[oEDamageIndex_MAX] = {};
    for ( int i = 0; i < count; ++i ) {
      widths[i] = layout.iconSize;
      if ( !immortal ) {
        zSTRING text = activeStatuses[i].immune
          ? ImmuneAbbreviation : zSTRING( activeStatuses[i].value );
        widths[i] += layout.fontY / 10 + screen->FontSize( text );
      }
      layout.contentWidth = max( layout.contentWidth, widths[i] );
    }

    const bool outsideValue = Options::StatusBarValueMode != StatusBar::Inside
      && Options::StatusBarValueMode != StatusBar::Disabled;

    if ( layout.placement != FocusStatusProtectionPlacement::TOP ) {
      layout.margin = verticalMargin;
      layout.contentHeight = count * layout.rowHeight + ( count - 1 ) * verticalMargin;
      const int barMargin = layout.placement == FocusStatusProtectionPlacement::RIGHT
        ? ProtectionPlacementRightMargin : horizontalMargin;
      layout.startY = bar->vposy + bar->vsizey / 2 - layout.rowHeight / 2;
      layout.startY = max( 0, min( 8192 - layout.contentHeight, layout.startY ) );

      int left = bar->vposx;
      int right = bar->vposx + bar->vsizex;
      int nameX, nameY;
      GetFocusNamePosition( nameX, nameY );
      const int nameHeight = GetFocusNameHeight();
      if ( nameY < layout.startY + layout.contentHeight && nameY + nameHeight > layout.startY ) {
        left = min( left, nameX );
        right = max( right, nameX + focusNameLayout.textWidth + focusNameLayout.iconsWidth );
      }
      if ( outsideValue ) {
        zSTRING value = GetBarValue();
        const int valueWidth = screen->FontSize( value );
        const int valueX = max( 0, min( 8192 - valueWidth, bar->vposx + bar->vsizex / 2 - valueWidth / 2 ) );
        left = min( left, valueX );
        right = max( right, valueX + valueWidth );
      }

      const int rightX = right + barMargin;
      const int leftX = left - barMargin - layout.contentWidth;
      if ( layout.contentHeight <= 8192 && ( rightX + layout.contentWidth <= 8192 || leftX >= 0 ) ) {
        layout.startX = rightX + layout.contentWidth <= 8192 ? rightX : leftX;
        for ( int i = 0; i < count; ++i )
          layout.itemY[i] = i * ( layout.rowHeight + verticalMargin );
        return;
      }

      // A column cannot fit beside the name or inside the screen. Keep every
      // protection visible by flowing the cells into rows above/below the bar.
      layout.placement = FocusStatusProtectionPlacement::TOP;
    }

    layout.margin = horizontalMargin;
    layout.contentWidth = 0;
    const int edgeSpace = screen->anx( 1 ) + 1;
    const int availableWidth = 8192 - 2 * edgeSpace;
    int rowStart = 0;
    int rowWidth = 0;
    int rowY = 0;
    for ( int i = 0; i <= count; ++i ) {
      const int nextWidth = i < count ? rowWidth + ( i > rowStart ? horizontalMargin : 0 ) + widths[i] : 0;
      if ( i == count || ( i > rowStart && nextWidth > availableWidth ) ) {
        layout.contentWidth = max( layout.contentWidth, rowWidth );
        // Initially center each row on zero; translate to content coordinates
        // once the widest row is known.
        for ( int j = rowStart; j < i; ++j )
          layout.itemX[j] -= rowWidth / 2;
        rowY += layout.rowHeight + verticalMargin;
        rowStart = i;
        rowWidth = 0;
      }
      if ( i < count ) {
        layout.itemX[i] = rowWidth + ( i > rowStart ? horizontalMargin : 0 );
        layout.itemY[i] = rowY;
        rowWidth = layout.itemX[i] + widths[i];
      }
    }
    for ( int i = 0; i < count; ++i )
      layout.itemX[i] += layout.contentWidth / 2;
    layout.contentHeight = rowY - verticalMargin;

    layout.startX = bar->vposx + bar->vsizex / 2 - layout.contentWidth / 2;
    if ( Options::ShowEnemyBarAboveHim ) {
      const int valueSpace = outsideValue ? bar->vsizey / 2 + layout.fontY : 0;
      const int requiredBarY = valueSpace + layout.contentHeight + layout.margin;
      const int nameHeight = GetFocusNameHeight();
      const int belowBar = max( bar->vsizey, bar->vsizey / 2 + focusNameLayout.fontY + nameHeight );
      bar->SetPos( bar->vposx, max( requiredBarY, min( 8192 - belowBar, bar->vposy ) ) );
      layout.startY = bar->vposy - layout.margin - layout.contentHeight;
    }
    else {
      const int valueSpace = outsideValue ? layout.fontY + layout.margin : 0;
      const int belowBar = bar->vsizey + layout.margin + valueSpace + layout.contentHeight;
      const int aboveBar = layout.margin + valueSpace + layout.contentHeight;
      if ( bar->vposy + belowBar <= 8192 ) {
        layout.startY = bar->vposy + bar->vsizey + layout.margin + valueSpace;
      }
      else if ( bar->vposy >= aboveBar ) {
        layout.startY = bar->vposy - aboveBar;
        if ( outsideValue )
          valueOffsetY = -bar->vsizey - 2 * layout.margin - layout.fontY;
      }
      else {
        bar->SetPos( bar->vposx, max( 0, 8192 - belowBar ) );
        layout.startY = bar->vposy + bar->vsizey + layout.margin + valueSpace;
      }
    }

    layout.startX = max( 0, min( 8192 - layout.contentWidth, layout.startX ) );
    layout.startY = max( 0, min( 8192 - layout.contentHeight, layout.startY ) );
  }

  FocusStatusProtectionPlacement FocusStatusBar::GetProtectionPlacement() const {
    if ( activeStatusesCount == 1 ) {
      return FocusStatusProtectionPlacement::CLOSE;
    }

    if ( Options::TargetProtectionIconPosition == 0 ) {
      return FocusStatusProtectionPlacement::TOP;
    }

    if ( activeStatusesCount <= 0 ) {
      return FocusStatusProtectionPlacement::NONE;
    }

    return FocusStatusProtectionPlacement::RIGHT;
  }

  int FocusStatusBar::GetHorizontalProtMargin( int fontY ) const {
    return static_cast<int>( fontY * 0.1f + 30 );
  }

  int FocusStatusBar::GetVerticalProtMargin( int fontY ) const {
    return static_cast<int>( fontY * 0.15f + 15 );
  }

  void FocusStatusBar::RenderProtectionWithLayout( const ProtectionLayout& layout ) {
    if ( protectionModel.renderMode == ProtectionRenderMode::Immortal ) {
      auto color = Colors::Gray;
      if ( bar )
        color.alpha = bar->alpha;

      IconInfo( layout.startX, layout.startY + layout.iconOffsetY, layout.iconSize, color, CrackedShieldTexture );
      return;
    }

    unsigned char alpha = bar ? bar->alpha : 255;
    for ( int i = 0; i < activeStatusesCount; ++i ) {
      auto data = BuildIconRenderData( activeStatuses[i], alpha );
      IconInfo( layout.startX + layout.itemX[i], layout.startY + layout.itemY[i] + layout.iconOffsetY,
        layout.iconSize, data.color, data.texture, data.text );
    }
  }

  bool FocusStatusBar::CanRenderProtectionStatus( oCNpc* npc, oEIndexDamage damageIndex ) const {
    if ( protectionModel.targetProtectionMode != TargetProtectionMode::All )
      return true;

    if ( Options::HideTargetProtectionZeroValues && npc->GetProtectionByIndex( damageIndex ) == 0 ) {
      return false;
    }

    switch ( damageIndex ) {
      case oEDamageIndex_Fall:
        return !Options::HideTargetProtectionFallDamage;
      case oEDamageIndex_Fly:
        return !Options::HideTargetProtectionFlyDamage;
      case oEDamageIndex_Fire:
        return !Options::HideTargetProtectionFireDamage;
      default:
        return true;
    }
  }

  ProtectionIconRenderData FocusStatusBar::BuildIconRenderData( const NpcProtectionStatus& status, unsigned char alpha ) {
    ProtectionIconRenderData data;

    data.text = status.immune ? ImmuneAbbreviation : zSTRING( status.value );

    data.texture = Options::TargetProtectionIconStyle
      ? CrackedShieldTexture
      : GetIconNameByDamageIndex( status.damageIndex );

    data.color = Colors::GetColorByDamageIndex( status.damageIndex );
    data.color.alpha = alpha;

    return data;
  }

  void FocusStatusBar::GetFocusNamePosition( int& x, int& y ) const {
    x = focusNameX;
    y = focusNameY;
    if ( !bar )
      return;

    if ( Options::ShowEnemyBarAboveHim ) {
      x = bar->vposx + bar->vsizex / 2 - focusNameLayout.textWidth / 2;
      y = bar->vposy + bar->vsizey / 2 + focusNameLayout.fontY;
    }
    x = max( 0, min( 8192 - focusNameLayout.textWidth - focusNameLayout.iconsWidth, x ) );
    y = max( 0, min( 8192 - GetFocusNameHeight(), y ) );
  }

  int FocusStatusBar::GetFocusNameHeight() const {
    // FontY excludes the last glyph pixel. Include it and the square reward
    // icons, whose virtual height varies with the screen aspect ratio.
    return max( focusNameLayout.fontY + screen->any( 1 ) + 1, focusNameLayout.iconsHeight );
  }

  void FocusStatusBar::AvoidFocusNameOverlap( const ProtectionLayout& layout ) {
    if ( Options::ShowEnemyBarAboveHim )
      return;

    const int nameWidth = focusNameLayout.textWidth + focusNameLayout.iconsWidth;
    const int nameHeight = GetFocusNameHeight();
    GetFocusNamePosition( focusNameX, focusNameY );
    bool overlap = false;
    int blockedTop = 8192;
    int blockedBottom = 0;
    auto reserve = [&]( int x, int y, int width, int height ) {
      if ( width <= 0 || height <= 0 || focusNameX >= x + width || focusNameX + nameWidth <= x )
        return;
      blockedTop = min( blockedTop, y );
      blockedBottom = max( blockedBottom, y + height );
      if ( focusNameY < y + height && focusNameY + nameHeight > y )
        overlap = true;
    };
    reserve( bar->vposx, bar->vposy, bar->vsizex, bar->vsizey );
    reserve( layout.startX, layout.startY, layout.contentWidth, layout.contentHeight );
    const bool outsideValue = Options::StatusBarValueMode != StatusBar::Inside
      && Options::StatusBarValueMode != StatusBar::Disabled;
    if ( outsideValue ) {
      zSTRING value = GetBarValue();
      const int valueWidth = screen->FontSize( value );
      int valueX, valueY;
      GetOutsideValuePosition( valueX, valueY, screen->FontY(), valueWidth );
      reserve( valueX, valueY, valueWidth, screen->FontY() + screen->any( 1 ) + 1 );
    }
    if ( !overlap )
      return;

    // Fixed bars and projected names have independent anchors. Only move a
    // name when it intersects the bar, value, or protections.
    const int below = blockedBottom + GetVerticalProtMargin( focusNameLayout.fontY );
    const int above = blockedTop - GetVerticalProtMargin( focusNameLayout.fontY ) - nameHeight;
    focusNameY = below + nameHeight <= 8192 ? below : max( 0, above );
    focusNameX = max( 0, min( 8192 - nameWidth, focusNameX ) );
  }

  void FocusStatusBar::MoveFocusBar( int x, int y, oCNpc* npc ) {
    if ( !Options::ShowEnemyBarAboveHim )
      return;

    if ( !IsBarActive() )
      return;

    zCCamera* cam = ogame->GetCamera();
    zVEC3 viewPos = cam->GetTransform( zTCamTrafoType::zCAM_TRAFO_VIEW ) * npc->GetPositionWorld();
    int posx, posy;
    cam->Project( &viewPos, posx, posy );
    if ( viewPos[VZ] <= cam->nearClipZ )
      return;

    x = x + screen->FontSize( npc->name[0] ) / 2 - bar->vsizex / 2;
    if ( x + bar->vsizex > 8192 )
      x = 8192 - bar->vsizex;

    x = max( 0, x );
    const int nameHeight = GetFocusNameHeight();
    const int belowBar = max( bar->vsizey, bar->vsizey / 2 + focusNameLayout.fontY + nameHeight );
    const bool outsideValue = Options::StatusBarValueMode != StatusBar::Inside
      && Options::StatusBarValueMode != StatusBar::Disabled;
    const int aboveBar = outsideValue ? bar->vsizey / 2 + screen->FontY() : 0;
    y = max( aboveBar, min( 8192 - belowBar, y - static_cast<int>( screen->FontY() * 1.75f ) ) );

    bar->SetPos( x, y );
  }

  void FocusStatusBar::GetOutsideValuePosition( int& x, int& y, int fontY, int textWidth ) const {
    x = bar->vposx + bar->vsizex / 2 - textWidth / 2;
    if ( Options::ShowEnemyBarAboveHim ) {
      int offsetY = bar->vsizey / 2 + fontY;
      y = bar->vposy - offsetY + valueOffsetY;
    }
    else {
      int margin = GetHorizontalProtMargin( fontY );
      y = bar->vposy + bar->vsizey + margin + valueOffsetY;
      if ( !valueOffsetY && y + fontY > 8192 )
        y = bar->vposy - margin - fontY;
    }

    y = max( 0, min( 8192 - fontY - screen->any( 1 ) - 1, y ) );
    x = max( 0, min( 8192 - textWidth, x ) );
  }

  void FocusStatusBar::PrintValueOutside( zSTRING& str ) {
    int x, y;
    GetOutsideValuePosition( x, y, valueView->FontY(), valueView->FontSize( str ) );
    valueView->SetFontColor( zCOLOR( valueView->color.r, valueView->color.g, valueView->color.b, bar->alpha ) );
    valueView->Print( x, y, str );
  }

  const zSTRING& FocusStatusBar::GetIconNameByDamageIndex( const oEIndexDamage& index ) {
    if ( index < 0 || index >= oEDamageIndex_MAX )
      return iconCache[oEDamageIndex_MAX]; // fallback

    return iconCache[index];
  }
}
