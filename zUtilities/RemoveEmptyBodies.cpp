// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
  static std::unordered_set<oCNpc*> fadingBodies;

  void ClearFadingBodies() {
    fadingBodies.clear();
  }

  HOOK Ivk_CheckRemoveNpc_Union PATCH( &oCSpawnManager::CheckRemoveNpc, &oCSpawnManager::CheckRemoveNpc_Union );
  int oCSpawnManager::CheckRemoveNpc_Union( oCNpc* npc ) {
    if ( !npc || !npc->GetHomeWorld() ) {
      return THISCALL( Ivk_CheckRemoveNpc_Union )( npc );
    }

    // Safety: living NPCs or player must NEVER be touched by body removal!
    if ( npc->attribute[NPC_ATR_HITPOINTS] > 0 || npc->IsAPlayer() || !npc->IsFullyDead() ) {
      return THISCALL( Ivk_CheckRemoveNpc_Union )( npc );
    }

    // 1. If this dead body is already fading away, handle its fading until completely gone
    if ( fadingBodies.find( npc ) != fadingBodies.end() ) {
      this->InitCameraPos();
      auto dist = ( npc->GetPositionWorld() - this->camPos ).LengthApprox();

      if ( dist > this->GetRemoveRange() || Options::RemoveBodies == 2 ) {
        fadingBodies.erase( npc );
        this->DeleteNpc( npc );
        return TRUE;
      }
      else {
        int faded = npc->FadeAway();
        if ( faded ) {
          fadingBodies.erase( npc );
          this->DeleteNpc( npc );
          return TRUE;
        }
        return TRUE;
      }
    }

    // 2. If option is disabled (0), do not touch any new bodies
    if ( !Options::RemoveBodies ) {
      return THISCALL( Ivk_CheckRemoveNpc_Union )( npc );
    }

    if ( npc->GetEM()->GetCutsceneMode() || ( !npc->IsConditionValid() && npc->attribute[NPC_ATR_HITPOINTS] > 0 ) ) {
      return THISCALL( Ivk_CheckRemoveNpc_Union )( npc );
    }

#if ENGINE >= Engine_G2
    if ( npc->GetGuild() == NPC_GIL_DRAGON ) {
      return THISCALL( Ivk_CheckRemoveNpc_Union )( npc );
    }
#endif

    auto InventoryEmpty = false;
#if ENGINE <= Engine_G2
    InventoryEmpty = npc->IsInventoryEmpty( true, false );
#else
    InventoryEmpty = npc->IsInventoryEmpty( true, true );
#endif

    if ( !InventoryEmpty || npc->HasMissionItem() ) {
      return THISCALL( Ivk_CheckRemoveNpc_Union )( npc );
    }

    if ( Options::RemoveBodies == 1 ) {
      npc->SetCollDetDyn( FALSE );
      npc->SetSleeping( FALSE );
      npc->fadeAwayTime = 5000.0f;
      fadingBodies.insert( npc );
    }
    else if ( Options::RemoveBodies == 2 ) {
      this->DeleteNpc( npc );
      return TRUE;
    }

    return THISCALL( Ivk_CheckRemoveNpc_Union )( npc );
  }
}