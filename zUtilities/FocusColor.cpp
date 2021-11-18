// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
  HOOK Ivk_Print_Union PATCH( &zCView::Print, &zCView::Print_Union );
  void zCView::Print_Union( int x, int y, const zSTRING& text ) {
    if ( focusColor.CanPrintFocus( this, x, y, text ) )
      return;

    THISCALL( Ivk_Print_Union )(x, y, text);
  }

  int oCNpc::GetAivar( zSTRING aivar ) {
    auto sym = parser->GetSymbol( aivar );
    if ( !sym )
      return 0;

    return this->aiscriptvars[sym->single_intdata];
  }

  void FocusColor::InitData() {
    zCPar_Symbol* sym = nullptr;

    sym = parser->GetSymbol( "NPCTYPE_FRIEND" );
    TYPE_FRIEND = (sym) ? sym->single_intdata : Invalid;

    sym = parser->GetSymbol( "CRIME_MURDER" );
    CRIME_MURDER = (sym) ? sym->single_intdata : Invalid;
  }

  bool FocusColor::CanStealNow( oCItem* focusItem ) {
    int ZS_Clear = parser->GetIndex( "ZS_ClearRoom" );

#if ENGINE >= Engine_G2
    int ZS_Observe = parser->GetIndex( "ZS_ObservePlayer" );
#else
    int ZS_Observe = parser->GetIndex( "ZS_ObservePerson" );
#endif

    if ( !ZS_Observe || !ZS_Clear )
      return false;

    auto list = ogame->GetGameWorld()->voblist_npcs->GetNextInList();

    while ( list != nullptr ) {
      oCNpc* npc = list->GetData();
      list = list->GetNextInList();

      if ( npc->attribute[NPC_ATR_HITPOINTS] <= 0 || npc == player ) continue;

      if ( !npc->HasPerception( NPC_PERC_ASSESSTHEFT ) )
        continue;

      if ( !npc->IsInPerceptionRange( NPC_PERC_ASSESSTHEFT, (int)npc->GetDistanceToVob( *player ) ) )
        continue;

#if ENGINE < Engine_G2
      if ( npc->GetAivar( "AIV_ITEMSCHWEIN" ) && npc->HasVobDetected( player ) )
        return false;
#endif

      if ( ogame->GetGuilds()->GetAttitude( npc->guild, player->guild ) == NPC_ATT_FRIENDLY ) continue;
      if ( npc->IsFriendly( player ) || npc->npcType == TYPE_FRIEND ) continue;

      if ( !npc->HasVobDetected( player ) ) {
        if ( npc->IsAIState( ZS_Clear ) || npc->IsAIState( ZS_Observe ) )
          return false;

        continue;
      }

#if ENGINE < Engine_G2
      if ( !focusItem->owner && !focusItem->ownerGuild )
        continue;
#endif

      return false;
    }

    return true;
  }

#if ENGINE >= Engine_G2
  bool FocusColor::CanTakeFromRoom( oCItem* focusItem ) {
    oCPortalRoomManager* rooms = ogame->GetPortalRoomManager();

    if ( !rooms ) return true;

    const zSTRING* portalName = focusItem->GetSectorNameVobIsIn();

    if ( !portalName ) return true;

    int portalIndex = rooms->GetPortalRoomIndex( *portalName );

    if ( portalIndex == Invalid ) return true;

    oCPortalRoom* portalRoom = rooms->portals[portalIndex];

    if ( !portalRoom ) return true;

    int roomGuild = portalRoom->GetOwnerGuild();

    if ( !roomGuild ) return true;

    int att = ogame->GetGuilds()->GetAttitude( roomGuild, player->guild );

    if ( att == NPC_ATT_FRIENDLY ) return true;

    return false;
  }

  int FocusColor::GetAbsolutionLevel( oCNpc* slf ) {
    zCPar_Symbol* sym = nullptr;

    sym = parser->GetSymbol( "NPCTYPE_OCAMBIENT" );
    int TYPE_OCAMBIENT = (sym) ? sym->single_intdata : Invalid;

    sym = parser->GetSymbol( "NPCTYPE_OCMAIN" );
    int TYPE_OCMAIN = (sym) ? sym->single_intdata : Invalid;

    sym = parser->GetSymbol( "NPCTYPE_BL_AMBIENT" );
    int TYPE_BL_AMBIENT = (sym) ? sym->single_intdata : Invalid;

    sym = parser->GetSymbol( "NPCTYPE_BL_MAIN" );
    int TYPE_BL_MAIN = (sym) ? sym->single_intdata : Invalid;

    if ( (TYPE_OCAMBIENT && slf->npcType == TYPE_OCAMBIENT) || (TYPE_OCMAIN && slf->npcType == TYPE_OCMAIN) ) {
      if ( slf->guild == NPC_GIL_PALADIN || slf->guild == NPC_GIL_MILIZ || slf->guild == NPC_GIL_VOLK )
        if ( sym = parser->GetSymbol( "ABSOLUTIONLEVEL_OldCamp" ) )
          return sym->single_intdata;
    }
    else if ( slf->guild == NPC_GIL_PALADIN || slf->guild == NPC_GIL_MILIZ || slf->guild == NPC_GIL_VOLK ) {
      if ( sym = parser->GetSymbol( "ABSOLUTIONLEVEL_City" ) )
        return sym->single_intdata;
    }
    else if ( slf->guild == NPC_GIL_FEUERKREIS || slf->guild == NPC_GIL_NOVIZE ) {
      if ( sym = parser->GetSymbol( "ABSOLUTIONLEVEL_Monastery" ) )
        return sym->single_intdata;
    }
    else if ( slf->guild == NPC_GIL_BAUERN ) {
      if ( sym = parser->GetSymbol( "ABSOLUTIONLEVEL_Farm" ) )
        return sym->single_intdata;
    }
    else if ( (TYPE_BL_AMBIENT && slf->npcType == TYPE_BL_AMBIENT) || (TYPE_BL_MAIN && slf->npcType == TYPE_BL_MAIN) ) {
      if ( sym = parser->GetSymbol( "ABSOLUTIONLEVEL_BL" ) )
        return sym->single_intdata;
    }

    return 0;
  }

  bool FocusColor::HasReasonToKill( oCNpc* slf ) {
    if ( slf->GetAivar( "AIV_NpcSawPlayerCommit" ) == CRIME_MURDER ) return true;
    int AttackReason = slf->GetAivar( "AIV_ATTACKREASON" );
    if ( AttackReason == 13 ) return true;
    if ( AttackReason == 14 ) return true;
    if ( AttackReason == 15 ) return true;
    if ( AttackReason == 17 ) return true;
    if ( AttackReason == 18 ) return true;
    if ( AttackReason == 19 ) return true;
    if ( slf->GetAivar( "AIV_DropDeadAndKill" ) ) return true;

    return false;
  }
#endif

  zCOLOR FocusColor::NpcColor( oCNpc* focusNpc ) {
    if ( focusNpc->attribute[NPC_ATR_HITPOINTS] <= 0 ) {
      if ( !focusNpc->stealcontainer->contList.GetNumInList() )
        return zCOLOR( 175, 175, 175 );
      else
        return colDefault;
    }

    bool inAttack = focusNpc->IsAIState( parser->GetIndex( "ZS_Attack" ) );
    bool inReact = focusNpc->IsAIState( parser->GetIndex( "ZS_ReactToDamage" ) );

#if ENGINE >= Engine_G2
    if ( focusNpc->IsHostile( player ) && focusNpc->GetPermAttitude( player ) == NPC_ATT_HOSTILE
      || (focusNpc->enemy == player && inAttack && HasReasonToKill( focusNpc )) )
      return zCOLOR( 255, 0, 0 );

    if ( focusNpc->IsAngry( player ) || (focusNpc->enemy == player && inAttack) )
      return zCOLOR( 255, 180, 0 );

    int day, hour, min;
    ogame->GetTime( day, hour, min );

    if ( focusNpc->GetAivar( "AIV_NpcSawPlayerCommit" )
      && !(focusNpc->GetAivar( "AIV_NpcSawPlayerCommit" ) < CRIME_MURDER && focusNpc->GetAivar( "AIV_NpcSawPlayerCommitDay" ) < day - 2)
      && !(focusNpc->GetAivar( "AIV_CrimeAbsolutionLevel" ) < GetAbsolutionLevel( focusNpc )) )
      return zCOLOR( 255, 180, 0 );
#else
    if ( (focusNpc->IsHostile( player ) && focusNpc->GetPermAttitude( player ) == NPC_ATT_HOSTILE)
      || (focusNpc->enemy == player && inAttack && focusNpc->GetAivar( "AIV_ATTACKREASON" )) )
      return zCOLOR( 255, 0, 0 );

    if ( (focusNpc->IsAngry( player ) || focusNpc->enemy == player) && (inAttack || inReact) )
      return zCOLOR( 255, 180, 0 );
#endif

    if ( inReact )
      return colDefault;

    if ( focusNpc->GetAivar( "AIV_PARTYMEMBER" ) )
      return zCOLOR( 51, 235, 255 );

    if ( (focusNpc->IsFriendly( player ) || focusNpc->npcType == TYPE_FRIEND) )
      return zCOLOR( 0, 255, 0 );

    if ( ogame->GetGuilds()->GetAttitude( focusNpc->guild, player->guild ) == NPC_ATT_FRIENDLY )
      return zCOLOR( 175, 255, 175 );

    return colDefault;
  }

  zCOLOR FocusColor::LockableColor( oCMobLockable* focusLockable ) {
    if ( focusLockable->locked )
      if ( focusLockable->keyInstance.Length() && focusLockable->pickLockStr.Length() )
        return zCOLOR( 255, 135, 150 );
      else if ( focusLockable->keyInstance.Length() )
        return zCOLOR( 255, 20, 20 );
      else
        return zCOLOR( 255, 175, 0 );

    return colDefault;
  }

  zCOLOR FocusColor::ChestColor( oCMobContainer* focusContainer ) {
    if ( focusContainer->locked )
      return LockableColor( focusContainer );

    if ( focusContainer->containList.GetNumInList() )
      return zCOLOR( 0, 175, 0 );

    return zCOLOR( 175, 175, 175 );
  }

  zCOLOR FocusColor::ItemColor( oCItem* focusItem ) {
#if ENGINE >= Engine_G2
    if ( focusItem->HasFlag( ITM_FLAG_DROPPED ) )
      return colDefault;
#endif

    if ( focusItem->IsOwnedByNpc( player->GetInstance() ) )
      return colDefault;

    if ( focusItem->IsOwnedByGuild( player->guild ) )
      return colDefault;

#if ENGINE >= Engine_G2
    if ( CanTakeFromRoom( focusItem ) )
      return colDefault;
#endif

    if ( CanStealNow( focusItem ) )
      return colDefault;

    return zCOLOR( 255, 200, 100 );
  }

  zCOLOR FocusColor::GetFocusColor( zCVob* focusVob ) {
    if ( !TYPE_FRIEND || !CRIME_MURDER )
      InitData();

    if ( Options::ColorChests )
      if ( oCMobContainer* focusContainer = focusVob->CastTo<oCMobContainer>() )
        return ChestColor( focusContainer );

    if ( Options::ColorDoors )
      if ( oCMobDoor* focusDoor = focusVob->CastTo<oCMobDoor>() )
        return LockableColor( focusDoor );

    if ( Options::ColorNpcs )
      if ( oCNpc* focusNpc = focusVob->CastTo<oCNpc>() )
        return NpcColor( focusNpc );

    if ( Options::ColorItems )
      if ( oCItem* focusItem = focusVob->CastTo<oCItem>() )
        return ItemColor( focusItem );

    return colDefault;
  }

  zSTRING FocusColor::GetName( zCVob* focusVob ) {
    if ( Options::ColorChests )
      if ( oCMobContainer* focusContainer = focusVob->CastTo<oCMobContainer>() )
        return focusContainer->GetName();

    if ( Options::ColorDoors )
      if ( oCMobDoor* focusDoor = focusVob->CastTo<oCMobDoor>() )
        return focusDoor->GetName();

    if ( Options::ColorNpcs )
      if ( oCNpc* focusNpc = focusVob->CastTo<oCNpc>() )
        return focusNpc->name[0];

    if ( Options::ColorItems )
      if ( oCItem* focusItem = focusVob->CastTo<oCItem>() )
        return focusItem->name;

    return "";
  }

  bool FocusColor::TryPrintFocus( int x, int y, zSTRING name, zCVob* vob ) {
    zCOLOR col = GetFocusColor( vob );
    if ( col.GetDescription() == colDefault.GetDescription() )
      col = screen->GetColor();

    if ( focusView == nullptr ) {
      focusView = new zCView( 0, 0, 8192, 8192 );
      screen->InsertItem( focusView );
    }
    else
      focusView->ClrPrintwin();

    if ( playerStatus.focusBar && playerStatus.focusBar->NeedAdjustPosition( x, y, vob->CastTo<oCNpc>() ) ) {
      x = ogame->focusBar->vposx + ogame->focusBar->vsizex / 2 - focusView->FontSize( name ) / 2;
      y = ogame->focusBar->vposy + ogame->focusBar->vsizey / 2 + focusView->FontY();
    }

    focusView->SetFontColor( col );
    focusView->Print( x, y, name );
    vobOnScreen = true;
    return true;
  }

  bool FocusColor::CanPrintFocus( zCView* view, int x, int y, const zSTRING& text ) {
    if ( !player || playerHelper.IsInInfo() || view == focusColor.focusView || focusColor.vobOnScreen )
      return false;

    zCVob* vob = player->GetFocusVob();
    if ( !vob ) return false;

    zSTRING name = GetName( vob );
    if ( text != name + "\n" && text != name )
      return false;

    return focusColor.TryPrintFocus( x, y, name, vob );
  }

  void FocusColor::Clear() {
    if ( focusView ) {
      focusView->ClrPrintwin();
      screen->RemoveItem( focusView );
      focusView = nullptr;
      vobOnScreen = false;
    }
  }

  void FocusColor::Loop() {
    if ( focusView )
      focusView->ClrPrintwin();

    vobOnScreen = false;

    zCVob* vob = player->GetFocusVob();
    if ( !vob || quickSave->isSaving || ogame->IsOnPause() ) {
      Clear();
      return;
    }
  }
}