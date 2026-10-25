// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
  struct FadingBodyState {
    float visualAlpha;
    bool visualAlphaEnabled;
    bool collDetectionDynamic;
    zTVobSleepingMode sleepingMode;
    float fadeAwayTime;
    bool fadeAlphaApplied = false;
    float appliedAlpha = 0.0f;
    bool appliedAlphaEnabled = false;

    explicit FadingBodyState( oCNpc* npc )
      : visualAlpha( npc->visualAlpha ), visualAlphaEnabled( npc->visualAlphaEnabled ),
        collDetectionDynamic( npc->collDetectionDynamic ),
        sleepingMode( static_cast<zTVobSleepingMode>( npc->sleepingMode ) ),
        fadeAwayTime( npc->fadeAwayTime ) {
    }

    void RememberAppliedAlpha( oCNpc* npc ) {
      fadeAlphaApplied = true;
      appliedAlpha = npc->visualAlpha;
      appliedAlphaEnabled = npc->visualAlphaEnabled;
    }

    void Restore( oCNpc* npc ) const {
      const bool living = npc->attribute[NPC_ATR_HITPOINTS] > 0;
      // A revival owns the new actor state. Only undo transparency still left
      // by our fade; an explicit opacity change by the reviver takes precedence.
      if ( !living || ( fadeAlphaApplied && npc->visualAlpha == appliedAlpha
        && npc->visualAlphaEnabled == appliedAlphaEnabled ) ) {
        npc->visualAlpha = visualAlpha;
        npc->visualAlphaEnabled = visualAlphaEnabled;
      }
      if ( living )
        return;

      npc->SetCollDetDyn( collDetectionDynamic );
      npc->SetSleepingMode( sleepingMode );
      npc->fadeAwayTime = fadeAwayTime;
    }
  };

  // Each entry owns one reference until cancellation or the world-removal callback.
  static std::unordered_map<oCNpc*, FadingBodyState> fadingBodies;

  struct ScopedBodyReference {
    oCNpc* npc;
    explicit ScopedBodyReference( oCNpc* value ) : npc( value ) {
      if ( npc )
        npc->AddRef();
    }
    ~ScopedBodyReference() {
      if ( npc )
        npc->Release();
    }
  };

  void ForgetFadingBody( oCNpc* npc ) {
    auto found = fadingBodies.find( npc );
    if ( found == fadingBodies.end() )
      return;

    FadingBodyState state = found->second;
    fadingBodies.erase( found );
    state.Restore( npc );
    npc->Release();
  }

  void ClearFadingBodies() {
    while ( !fadingBodies.empty() ) {
      oCNpc* npc = fadingBodies.begin()->first;
      ForgetFadingBody( npc );
    }
  }

  HOOK Ivk_CheckRemoveNpc_Union PATCH( &oCSpawnManager::CheckRemoveNpc, &oCSpawnManager::CheckRemoveNpc_Union );
  int oCSpawnManager::CheckRemoveNpc_Union( oCNpc* npc ) {
    if ( !npc )
      return THISCALL( Ivk_CheckRemoveNpc_Union )( npc );

    const bool tracked = fadingBodies.find( npc ) != fadingBodies.end();
    // A cancellation or FadeAway can synchronously release the tracked reference.
    ScopedBodyReference reference( tracked ? npc : nullptr );

    if ( !npc->GetHomeWorld() ) {
      ForgetFadingBody( npc );
      return THISCALL( Ivk_CheckRemoveNpc_Union )( npc );
    }

    if ( Options::RemoveBodies != 1 && Options::RemoveBodies != 2 ) {
      ForgetFadingBody( npc );
      return THISCALL( Ivk_CheckRemoveNpc_Union )( npc );
    }

    if ( npc->attribute[NPC_ATR_HITPOINTS] > 0 ) {
      ForgetFadingBody( npc );
      return THISCALL( Ivk_CheckRemoveNpc_Union )( npc );
    }

    if ( npc->IsAPlayer() ) {
      ForgetFadingBody( npc );
      return THISCALL( Ivk_CheckRemoveNpc_Union )( npc );
    }

    if ( !npc->IsFullyDead() ) {
      ForgetFadingBody( npc );
      return THISCALL( Ivk_CheckRemoveNpc_Union )( npc );
    }

    if ( npc->GetEM()->GetCutsceneMode() ) {
      ForgetFadingBody( npc );
      return THISCALL( Ivk_CheckRemoveNpc_Union )( npc );
    }

#if ENGINE >= Engine_G2
    if ( npc->GetGuild() == NPC_GIL_DRAGON ) {
      ForgetFadingBody( npc );
      return THISCALL( Ivk_CheckRemoveNpc_Union )( npc );
    }
#endif

#if ENGINE <= Engine_G2
    const bool inventoryEmpty = npc->IsInventoryEmpty( true, false );
#else
    const bool inventoryEmpty = npc->IsInventoryEmpty( true, true );
#endif
    const bool missionItem = npc->HasMissionItem();
    if ( !inventoryEmpty || missionItem ) {
      ForgetFadingBody( npc );
      return THISCALL( Ivk_CheckRemoveNpc_Union )( npc );
    }

    if ( Options::RemoveBodies == 2 ) {
      ForgetFadingBody( npc );
      this->DeleteNpc( npc );
      return TRUE;
    }

    if ( tracked ) {
      this->InitCameraPos();
      auto dist = ( npc->GetPositionWorld() - this->camPos ).LengthApprox();
      if ( dist > this->GetRemoveRange() ) {
        ForgetFadingBody( npc );
        this->DeleteNpc( npc );
        return TRUE;
      }

      // FadeAway performs deletion and can reenter the world-removal hook.
      if ( npc->FadeAway() )
        return TRUE;

      auto state = fadingBodies.find( npc );
      if ( state != fadingBodies.end() )
        state->second.RememberAppliedAlpha( npc );
      return FALSE;
    }

    ScopedBodyReference startReference( npc );
    fadingBodies.emplace( npc, FadingBodyState( npc ) );
    npc->AddRef();
    npc->SetCollDetDyn( FALSE );
    npc->SetSleeping( FALSE );
    npc->fadeAwayTime = 5000.0f;
    return FALSE;
  }
}
