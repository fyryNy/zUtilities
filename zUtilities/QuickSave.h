// Supported with union (c) 2020 Union team
// Union HEADER file

namespace GOTHIC_ENGINE {
  namespace Options {
    int QuickSaveMode, UseBinarySave, KeyQuickSave, KeyQuickLoad, MinSaveSlot, MaxSaveSlot;
    string CantSave, CantLoad, NoSave, SaveName;

    void QuickSave() {
      QuickSaveMode = zoptions->ReadInt( PLUGIN_NAME, "QuickSaveMode", 1 );

      KeyQuickSave = GetEmulationKeyCode( zoptions->ReadString( PLUGIN_NAME, "KeyQuickSave", "KEY_F10" ) );
      KeyQuickLoad = GetEmulationKeyCode( zoptions->ReadString( PLUGIN_NAME, "KeyQuickLoad", "KEY_F12" ) );

      int SaveSlots = 0;
      for ( int i = 0; i < zCMenuItem::itemList.GetNum(); i++ )
        if ( zCMenuItem::itemList[i]->m_parOnSelAction_S[1] == "SAVEGAME_SAVE" )
          SaveSlots++;

#if ENGINE >= Engine_G2
      if ( SaveSlots == 0 ) SaveSlots = 20;
#else
      if ( SaveSlots == 0 ) SaveSlots = 15;
#endif

      MinSaveSlot = zoptions->ReadInt( PLUGIN_NAME, "MinSaveSlot", SaveSlots - 5 );
      MaxSaveSlot = zoptions->ReadInt( PLUGIN_NAME, "MaxSaveSlot", SaveSlots );

      switch ( Union.GetSystemLanguage() )
      {
      case Lang_Rus:
        CantSave = "���� �� ����� ���� ��������� ������!";
        CantLoad = "���� �� ����� ���� ���������!";
        NoSave = "������ ���������� �� ����������!";
        break;
      case Lang_Ger:
        CantSave = "Das Spiel kann jetzt nicht gespeichert werden!";
        CantLoad = "Das Spiel kann jetzt nicht geladen werden!";
        NoSave = "Ein solches Speichern gibt es nicht!";
        break;
      case Lang_Pol:
        CantSave = "Nie mo�na teraz zapisa� rozgrywki!";
        CantLoad = "Nie mo�na teraz wczyta� rozgrywki!";
        NoSave = "Taki zapis nie istnieje!";
        break;
      default:
        CantSave = "The game cannot be saved now!";
        CantLoad = "The game cannot be loaded now!";
        NoSave = "Such a save does not exist!";
        break;
      }

      CantSave = A zoptions->ReadString( PLUGIN_NAME, "CantSave", CantSave );
      CantLoad = A zoptions->ReadString( PLUGIN_NAME, "CantLoad", CantLoad );
      NoSave = A zoptions->ReadString( PLUGIN_NAME, "NoSave", NoSave );
      SaveName = A zoptions->ReadString( PLUGIN_NAME, "SaveName", "QuickSave" );
    }
  }

  class QuickSave {
  private:
    bool disabledStatus = false;

    int iLastSaveSlot;
    int iLastSaveNumber;

    void SetSaveSlotAndNr();
    int CanSave();
    int InInteraction();
    void CheckSave();
    void CheckLoad();
    void StartSaveLoad();

  public:
    enum QuickSaveMode {
      Disabled,
      Standard,
      Alternative
    };

    bool saveEnd = false;
    bool isSaving = false;
    bool isLoading = false;
    void EndSaveLoad();
    bool IsBusy();
    void Loop();
    QuickSave();
  };

  QuickSave* quickSave;
}
