// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
  HOOK Hook_zCVob_Archive PATCH( &zCVob::Archive, &zCVob::Archive_Union );
  void zCVob::Archive_Union( zCArchiver& ar ) {
    // Persist the vob's real visibility without showing every hidden vob before a save.
    const bool hiddenByNoGrass = ar.InSaveGame() && noGrass.IsHidden( this );
    const bool previousShowVisual = showVisual != 0;
    if ( hiddenByNoGrass )
      showVisual = noGrass.GetOriginalVisibility( this );

    THISCALL( Hook_zCVob_Archive )( ar );

    if ( hiddenByNoGrass )
      showVisual = previousShowVisual;
  }

  void NoGrass::Update() {
    auto prevNoGrass = Options::NoGrass;
    auto prevNoGrassRemoveVobsWithDynamicCollisions = Options::NoGrassRemoveVobsWithDynamicCollisions;
    auto prevVisualNames = visualsNames;
    Options::NoGrass = zoptions->ReadBool( PLUGIN_NAME, "NoGrass", false );
    Options::NoGrassDebugShowHidden = zoptions->ReadBool( PLUGIN_NAME, "NoGrassDebugShowHidden", true );
    Options::NoGrassRemoveVobsWithDynamicCollisions = zoptions->ReadBool( PLUGIN_NAME, "NoGrassRemoveVobsWithDynamicCollisions", false );

    if ( !zoptions->EntryExists( PLUGIN_NAME, "NoGrassVisualNames0" ) )
      zoptions->WriteString( PLUGIN_NAME, "NoGrassVisualNames0", "*grass*, *farn*, *bush*, *smallweed*, nw_nature_plant*, nw_nature_sideplant*, *cavewebs*, *mushroom*", 0 );
    if ( !zoptions->EntryExists( PLUGIN_NAME, "NoGrassVisualNames1" ) )
      zoptions->WriteString( PLUGIN_NAME, "NoGrassVisualNames1", "kb_grass*, kb_unter*, kb_green*, kb_pflan*, kb_farn*, kb_strauch*, ody_farn*", 0 );
    if ( !zoptions->EntryExists( PLUGIN_NAME, "NoGrassVisualNames2" ) )
      zoptions->WriteString( PLUGIN_NAME, "NoGrassVisualNames2", "*duckweed*, *waterlili*", 0 );

    Options::NoGrassVisualNames.clear();
    int vi = 0;
    while ( true ) {
      zSTRING entryName = zSTRING{ "NoGrassVisualNames" } + zSTRING{ vi++ };
      if ( zoptions->EntryExists( PLUGIN_NAME, entryName ) )
        Options::NoGrassVisualNames.push_back( A zoptions->ReadString( PLUGIN_NAME, entryName, "" ) );
      else
        break;
    }

    UpdateVisualsList();
    if ( prevNoGrass != Options::NoGrass
      || prevNoGrassRemoveVobsWithDynamicCollisions != Options::NoGrassRemoveVobsWithDynamicCollisions
      || prevVisualNames != visualsNames )
      UpdateVisuals();
  }

  void NoGrass::UpdateVisualsList() {
    visualsNames.clear();
    for ( auto visualsNamesLists : Options::NoGrassVisualNames ) {
      auto visualsNamesList = visualsNamesLists.Split( "," );
      for ( auto& visualName : visualsNamesList ) {
        visualName.Shrink();
        if ( !visualName.IsEmpty() )
          visualsNames.push_back( visualName );
      }
    }
  }

  bool NoGrass::IsValidVob( zCVob* vob ) {
    if ( !vob )
      return false;

    if ( vob->GetVobType() == zVOB_TYPE_ITEM || vob->GetVobType() == zVOB_TYPE_NSC || vob->GetVobType() == zVOB_TYPE_MOB )
      return false;

    if ( !vob->GetVisual() )
      return false;

    return true;
  }

  bool NoGrass::IsHidden( zCVob* vob ) const {
    if ( !vob )
      return false;

    if ( vob->GetVobType() == zVOB_TYPE_ITEM || vob->GetVobType() == zVOB_TYPE_NSC || vob->GetVobType() == zVOB_TYPE_MOB )
      return false;

    return hiddenVobs.find( vob ) != hiddenVobs.end();
  }

  bool NoGrass::GetOriginalVisibility( zCVob* vob ) const {
    auto it = hiddenVobs.find( vob );
    return it != hiddenVobs.end() ? it->second : vob && vob->showVisual != 0;
  }

  void NoGrass::ForgetVob( zCVob* vob ) {
    auto it = hiddenVobs.find( vob );
    if ( it == hiddenVobs.end() )
      return;

    const bool originalShowVisual = it->second;
    // Erase before Release: destruction can reenter the world-removal hook.
    hiddenVobs.erase( it );
    vob->showVisual = originalShowVisual;
    vob->Release();
  }

  void NoGrass::RestoreVisibility() {
    while ( !hiddenVobs.empty() )
      ForgetVob( hiddenVobs.begin()->first );
  }

  void NoGrass::UpdateVisuals() {
    if ( !ogame || !ogame->GetWorld() )
      return;

    RestoreVisibility();

    if ( !Options::NoGrass )
      return;

    zCArray<zCVob*> vobList;
    vobList.AllocAbs( 16384 );
    ogame->GetWorld()->SearchVobListByClass( zCVob::classDef, vobList, nullptr );

    hiddenVobs.reserve( 4096 );
    std::unordered_map<zCVisual*, bool> visualMatchCache;
    visualMatchCache.reserve( 512 );

    for ( int i = 0; i < vobList.GetNumInList(); i++ ) {
      auto vob = vobList[i];

      if ( !IsValidVob( vob ) )
        continue;

      if ( !Options::NoGrassRemoveVobsWithDynamicCollisions && vob->collDetectionDynamic )
        continue;

      if ( !vob->showVisual )
        continue;

      zCVisual* visual = vob->GetVisual();
      auto it = visualMatchCache.find( visual );
      bool isMatch = false;

      if ( it != visualMatchCache.end() ) {
        isMatch = it->second;
      }
      else {
        const zSTRING& vobVisualName = visual->GetVisualName();
        if ( !vobVisualName.IsEmpty() && !vobVisualName.HasWordI( ".PFX" ) ) {
          for ( auto& visualName : visualsNames ) {
            if ( vobVisualName.CompareMaskedI( visualName ) ) {
              isMatch = true;
              break;
            }
          }
        }
        visualMatchCache.insert( { visual, isMatch } );
      }

      if ( isMatch ) {
        auto inserted = hiddenVobs.insert( { vob, vob->showVisual != 0 } );
        if ( inserted.second )
          vob->AddRef();
        vob->showVisual = 0;
      }
    }
  }

  void NoGrass::ShowVisualsNames() {
    zCArray<zCVob*> listVobs;
    player->CreateVobList( listVobs, 1000.0f );

    auto screenColor = screen->fontColor;
    auto textColor = GFX_WHITE;

    for ( int i = 0; i < listVobs.GetNumInList(); i++ ) {
      auto vob = listVobs[i];

      if ( !IsValidVob( vob ) )
        continue;

      auto vobVisualName = vob->GetVisual()->GetVisualName();
      if ( vobVisualName.IsEmpty() || vobVisualName.HasWordI( ".PFX" ) )
        continue;

      // if (!Options::NoGrassRemoveVobsWithDynamicCollisions && vob->collDetectionDynamic)
      //   continue;

      if ( !Options::NoGrassDebugShowHidden && !vob->showVisual )
        continue;

      if ( !vob->showVisual )
        textColor = GFX_ORANGE;
      else
        textColor = GFX_WHITE;

      if ( vob->collDetectionDynamic )
        vobVisualName += zSTRING{ " (dynamic)" };

      zVEC2 viewPos;
      if ( !DamagePopup::WorldToView( vob->GetPositionWorld(), screen, viewPos ) )
        continue;

      screen->SetFontColor( textColor );
      screen->Print( static_cast<int>( viewPos[VX] ), static_cast<int>( viewPos[VY] ), vobVisualName );
    }

    screen->SetFontColor( screenColor );
  }
} // namespace GOTHIC_ENGINE
