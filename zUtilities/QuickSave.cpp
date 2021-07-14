// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {

  void QuickSave::SetStringsByLanguage() {

    switch ( Union.GetSystemLanguage() )
    {
    case Lang_Rus:
      sCantSave = "���� �� ����� ���� ��������� ������!";
      sCantLoad = "���� �� ����� ���� ���������!";
      sNoSave = "������ ���������� �� ����������!";
      break;
    case Lang_Eng:
      sCantSave = "The game cannot be saved now!";
      sCantLoad = "The game cannot be loaded now!";
      sNoSave = "Such a save does not exist!";
      break;
    case Lang_Ger:
      sCantSave = "Das Spiel kann jetzt nicht gespeichert werden!";
      sCantLoad = "Das Spiel kann jetzt nicht geladen werden!";
      sNoSave = "Ein solches Speichern gibt es nicht!";
      break;
    case Lang_Pol:
      sCantSave = "Nie mo�na teraz zapisa� rozgrywki!";
      sCantLoad = "Nie mo�na teraz wczyta� rozgrywki!";
      sNoSave = "Taki zapis nie istnieje!";
      break;
    default:
      sCantSave = "The game cannot be saved now!";
      sCantLoad = "The game cannot be loaded now!";
      sNoSave = "Such a save does not exist!";
      break;
    }

    sCantSave = zoptions->ReadString( pluginName, "sCantSave", sCantSave );
    sCantLoad = zoptions->ReadString( pluginName, "sCantLoad", sCantLoad );
    sNoSave = zoptions->ReadString( pluginName, "sNoSave", sNoSave );
    sSaveName = zoptions->ReadString( pluginName, "sSaveName", "QuickSave" );

  }

  void QuickSave::ReadOptions() {
#if ENGINE >= Engine_G2
    iMinSaveSlot = zoptions->ReadInt( pluginName, "iMinSaveSlot", 15 );
    iMaxSaveSlot = zoptions->ReadInt( pluginName, "iMaxSaveSlot", 20 );
#else
    iMinSaveSlot = zoptions->ReadInt( pluginName, "iMinSaveSlot", 10 );
    iMaxSaveSlot = zoptions->ReadInt( pluginName, "iMaxSaveSlot", 15 );
#endif
    iLastSaveSlot = zoptions->ReadInt( pluginName, "iLastSaveSlot", iMinSaveSlot );
    iLastSaveNumber = zoptions->ReadInt( pluginName, "iLastSaveNumber", 0 );
  }

  int QuickSave::InInteraction( PlayerHelper* playerHelper ) {
    if ( playerHelper->IsSaving() ) return TRUE;
    if ( playerHelper->IsBusy() ) return TRUE;
    if ( player->bodyState == BS_TAKEITEM ) return TRUE;
    if ( player->bodyState == BS_MOBINTERACT ) return TRUE;
    if ( player->bodyState & BS_FLAG_INTERRUPTABLE && !(player->bodyState & BS_FLAG_FREEHANDS) ) return TRUE;

    return FALSE;
  }

  int QuickSave::CanSave( PlayerHelper* playerHelper ) {
    if ( playerIsDead ) return FALSE;
    if ( InInteraction( playerHelper ) ) return FALSE;
    if ( player->GetAnictrl()->state != zCAIPlayer::zMV_STATE_STAND ) return FALSE;
    //if ( ogame->game_testmode ) return FALSE;
    //if ( !player->IsInFightMode_S(0) ) return FALSE;

    return TRUE;
  }

  void QuickSave::Save( PlayerHelper* playerHelper ) {

    if ( !zinput->KeyToggled( KEY_F10 ) )
      return;

    if ( !CanSave( playerHelper ) ) {
      ogame->GetTextView()->Printwin( sCantSave );
      return;
    }

    ReadOptions();
    iLastSaveSlot++;
    iLastSaveNumber++;
    if ( iLastSaveSlot > iMaxSaveSlot )
      iLastSaveSlot = iMinSaveSlot;

    zoptions->WriteInt( pluginName, "iLastSaveSlot", iLastSaveSlot, FALSE );
    zoptions->WriteInt( pluginName, "iLastSaveNumber", iLastSaveNumber, FALSE );
    zoptions->Save( "Gothic.ini" );

    // Thumbnail
    zCTextureConvert* thumb = zrenderer->CreateTextureConvert();
    zrenderer->Vid_GetFrontBufferCopy( *thumb );

    // SaveGame
    ogame->WriteSavegame( iLastSaveSlot, true );

    // SaveInfo
    auto info = ogame->savegameManager->GetSavegame( iLastSaveSlot );

    info->m_Name = sSaveName + Z iLastSaveNumber;
    info->m_WorldName = ogame->GetGameWorld()->GetWorldName();
    int day, hour, min;
    ogame->GetTime( day, hour, min );
    info->m_TimeDay = day + 1;
    info->m_TimeHour = hour;
    info->m_TimeMin = min;
    info->m_PlayTimeSeconds = gameMan->GetPlaytimeSeconds();
    info->UpdateThumbPic( thumb );
    delete thumb;

    ogame->savegameManager->SetAndWriteSavegame( iLastSaveSlot, info );
  }

  void QuickSave::Load( PlayerHelper* playerHelper ) {

    if ( !zinput->KeyToggled( KEY_F12 ) )
      return;

    if ( InInteraction( playerHelper ) ) {
      ogame->GetTextView()->Printwin( sCantLoad );
      return;
    }

    ReadOptions();

    if ( !ogame->savegameManager->GetSavegame( iLastSaveSlot )->DoesSavegameExist() ) {
      ogame->GetTextView()->Printwin( sNoSave + " (" + Z iLastSaveSlot + ")" );
      return;
    }

    ogame->LoadSavegame( iLastSaveSlot, TRUE );
  }

  void QuickSave::QuickSaveLoop( PlayerHelper* playerHelper ) {

    Load( playerHelper );
    Save( playerHelper );

  }
}
