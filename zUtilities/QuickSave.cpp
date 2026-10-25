// Supported with union (c) 2020 Union team
// Union SOURCE file

#include <climits>

namespace GOTHIC_ENGINE {
  static bool NormalizeQuickSaveSlots( int slotCount, int& minSlot, int& maxSlot ) {
    if ( slotCount <= 0 )
      return false;

    // Derive the bounds from the manager so mods with additional slots work.
    minSlot = minSlot < 0 ? 0 : minSlot >= slotCount ? slotCount - 1 : minSlot;
    maxSlot = maxSlot < 0 ? 0 : maxSlot >= slotCount ? slotCount - 1 : maxSlot;
    if ( minSlot > maxSlot ) {
      const int previousMin = minSlot;
      minSlot = maxSlot;
      maxSlot = previousMin;
    }
    return true;
  }

  static oCSavegameInfo* GetQuickSaveInfo( int slot ) {
    auto manager = gameMan->savegameManager;
    // Negative indices select the engine's temporary current-save info;
    // positive indices are used by GetSavegame without a bounds check.
    if ( slot < 0 || slot >= manager->infoList.GetNum() )
      return nullptr;
    return manager->GetSavegame( slot );
  }

  static void AdvanceQuickSaveSlot( int mode, int minSlot, int maxSlot, int& lastSlot, int& lastNumber ) {
    // Restart the label sequence before signed overflow or an invalid counter.
    const int nextNumber = lastNumber < 0 || lastNumber == INT_MAX ? 1 : lastNumber + 1;
    if ( mode == QuickSave::QuickSaveMode::Alternative ) {
      // The lowest set bit selects increasingly old backup slots. Wrap over
      // the inclusive range, which also supports a single configured slot.
      unsigned int bits = static_cast<unsigned int>( nextNumber );
      int changeIndex = 0;
      while ( ( bits & 1u ) == 0u ) {
        ++changeIndex;
        bits >>= 1;
      }
      const int slotCount = maxSlot - minSlot + 1;
      lastSlot = minSlot + changeIndex % slotCount;
    }
    else {
      lastSlot = lastSlot < minSlot || lastSlot >= maxSlot ? minSlot : lastSlot + 1;
    }
    lastNumber = nextNumber;
  }

  bool QuickSave::KeepClosingMenus = false;

  HOOK Hook_zCMenu_HandleFrame PATCH( &zCMenu::HandleFrame, &zCMenu::HandleFrame_Union );
  void zCMenu::HandleFrame_Union( int frame ) {
    if ( QuickSave::KeepClosingMenus ) {
      for ( int i = 0; i < this->m_listItems.GetNum(); i++ )
        this->m_listItems[i]->m_bLeaveItem = true;

      this->m_exitState = zCMenu::BACK;
      return;
    }

    THISCALL( Hook_zCMenu_HandleFrame )( frame );
  }

  HOOK Hook_zCMenu_HandleRun PATCH( &zCMenu::Run, &zCMenu::Run_Union );
  int zCMenu::Run_Union()
  {
    int result = THISCALL( Hook_zCMenu_HandleRun )( );

    if ( zCMenu::activeList.GetNum() == 0 )
      QuickSave::KeepClosingMenus = false;

    return result;
  }

  void QuickSave::SetSaveSlotAndNr() {
    zCArray<oCSavegameInfo*> saveList = gameMan->savegameManager->infoList;
    NormalizeQuickSaveSlots( saveList.GetNum(), Options::MinSaveSlot, Options::MaxSaveSlot );

    int latestNr = 0;
    int latestSaveSlot = Invalid;

    for ( int i = 0; i < saveList.GetNum(); i++ ) {
      if ( !saveList[i] )
        continue;

      if ( saveList[i]->m_SlotNr < Options::MinSaveSlot || saveList[i]->m_SlotNr > Options::MaxSaveSlot )
        continue;

      if ( !saveList[i]->GetName().HasWord( Options::SaveName ) )
        continue;

      int nr = saveList[i]->GetName().Cut( 0, Options::SaveName.Length() ).ToInt32();
      if ( nr > latestNr ) {
        latestNr = nr;
        latestSaveSlot = saveList[i]->m_SlotNr;
      }
    }

    iLastSaveNumber = latestNr;
    iLastSaveSlot = latestSaveSlot;
  }

  int QuickSave::InInteraction() {
    if ( IsBusy() ) return true;
    if ( playerHelper.IsBusy() ) return true;
    if ( player->bodyState == BS_TAKEITEM ) return true;
    if ( player->bodyState == BS_MOBINTERACT ) return true;
    if ( player->bodyState & BS_FLAG_INTERRUPTABLE && !( player->bodyState & BS_FLAG_FREEHANDS ) ) return true;

    return false;
  }

  bool QuickSave::IsBusy() {
    return isSaving || isLoading;
  }

  int QuickSave::CanSave() {
    if ( playerHelper.IsDead() ) return false;
    if ( InInteraction() ) return false;
    if ( player->GetAnictrl()->state != zCAIPlayer::zMV_STATE_STAND ) return false;
    if ( player->HasBodyStateModifier( BS_MOD_TRANSFORMED ) ) return false;
    //if ( ogame->game_testmode ) return false;
    //if ( !player->IsInFightMode_S(0) ) return false;

    zSTRING saveMenu = gameMan->menu_save_savegame ? gameMan->menu_save_savegame->name : "MENU_SAVEGAME_SAVE";

    for ( int i = 0; i < zCMenuItem::itemList.GetNum(); i++ ) {
      zCMenuItem* item = zCMenuItem::itemList[i];
      if ( item->m_parOnSelAction_S[0] == saveMenu ) {
        bool cantSave = item->m_parItemFlags & IT_DISABLED || item->m_parItemFlags & IT_ONLY_OUT_GAME;
        return !cantSave;
      }
    }

    return true;
  }

  void QuickSave::CheckSave() {
    if ( !zinput->KeyToggled( Options::KeyQuickSave ) )
      return;

    if ( !NormalizeQuickSaveSlots( gameMan->savegameManager->infoList.GetNum(), Options::MinSaveSlot, Options::MaxSaveSlot ) || !CanSave() ) {
      ogame->GetTextView()->Printwin( Options::CantSave );
      return;
    }

    AdvanceQuickSaveSlot( Options::QuickSaveMode, Options::MinSaveSlot, Options::MaxSaveSlot, iLastSaveSlot, iLastSaveNumber );

    // Thumbnail
    zCTextureConvert* thumb = zrenderer->CreateTextureConvert();
    zrenderer->Vid_GetFrontBufferCopy( *thumb );

    // SaveGame
    isSaving = true;
    StartSaveLoad();
#if ENGINE == Engine_G1A
    ogame->WriteSavegame( iLastSaveSlot, true );
#else
    gameMan->Write_Savegame( iLastSaveSlot );
#endif

    // SaveInfo
    oCSavegameInfo* info = GetQuickSaveInfo( iLastSaveSlot );

    info->m_Name = Z Options::SaveName + Z iLastSaveNumber;
    info->m_WorldName = ogame->GetGameWorld()->GetWorldName();
    int day, hour, min;
    ogame->GetTime( day, hour, min );
    info->m_TimeDay = day + 1;
    info->m_TimeHour = hour;
    info->m_TimeMin = min;
    info->m_PlayTimeSeconds = gameMan->GetPlaytimeSeconds();
    info->UpdateThumbPic( thumb );
    delete thumb;

    gameMan->savegameManager->SetAndWriteSavegame( info->m_SlotNr, info );

    if ( Options::PrintQuickSaveLoadInfo )
      ogame->GetTextView()->Printwin( Z Options::SaveCompleted + " '" + Z info->m_Name + Z "' #" + Z info->m_SlotNr );
  }

  void QuickSave::CheckLoad() {
    if ( !zinput->KeyToggled( Options::KeyQuickLoad ) )
      return;

    if ( InInteraction() ) {
      ogame->GetTextView()->Printwin( Options::CantLoad );
      return;
    }

    oCSavegameInfo* info = GetQuickSaveInfo( iLastSaveSlot );

    if ( !info || !info->DoesSavegameExist() ) {
      ogame->GetTextView()->Printwin( Z Options::NoSave + " (" + Z iLastSaveSlot + ")" );
      return;
    }

    isLoading = true;
    StartSaveLoad();
#if ENGINE == Engine_G1A
    ogame->LoadSavegame( info->m_SlotNr, true );
#else
    gameMan->Read_Savegame( info->m_SlotNr );
#endif

    if ( Options::PrintQuickSaveLoadInfo )
      ogame->GetTextView()->Printwin( Z Options::LoadCompleted + " '" + Z info->m_Name + Z "' #" + Z info->m_SlotNr );
  }

  void QuickSave::LoadFromMainMenu() const {
    if ( gameMan->menu_load_savegame )
      gameMan->menu_load_savegame->m_selSlot = iLastSaveSlot;

    if ( zCMenu::activeList.GetNum() == 0 ) {
#if ENGINE == Engine_G1A
      ogame->LoadSavegame( iLastSaveSlot, true );
#else
      gameMan->Read_Savegame( iLastSaveSlot );
#endif
      return;
    }

    QuickSave::KeepClosingMenus = true;
    zoptions->WriteString( "internal", "menuAction", "SAVEGAME_LOAD", false );
  }

  void QuickSave::StartSaveLoad() {
    if ( ogame->GetShowPlayerStatus() ) {
      disabledStatus = true;
      ogame->SetShowPlayerStatus( false );
    }
  }

  void QuickSave::EndSaveLoad() {
    isSaving = false;
    isLoading = false;
    saveEnd = false;

    if ( disabledStatus ) {
      disabledStatus = false;
      ogame->SetShowPlayerStatus( true );
    }
  }

  void QuickSave::Loop() {
    if ( ( isLoading && !ogame->IsOnPause() ) || ( isSaving && saveEnd ) )
      EndSaveLoad();

    if ( !Options::QuickSaveMode ) return;

    if ( ogame->IsOnPause() ) return;

    CheckLoad();
    CheckSave();
  }

  void QuickSave::MenuLoop() {
    if ( !Options::QuickSaveMode || zCMenu::inGameMenu || isLoading ) return;

    if ( !( Options::QuickLoadAfterGameStart || zinput->KeyToggled( Options::KeyQuickLoad ) ) ) return;

    oCSavegameInfo* info = GetQuickSaveInfo( iLastSaveSlot );
    if ( !info || !info->DoesSavegameExist() ) return;

    LoadFromMainMenu();
  }

  QuickSave::QuickSave() {
    SetSaveSlotAndNr();
  }
}
