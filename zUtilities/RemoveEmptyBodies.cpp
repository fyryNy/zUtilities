// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
  HOOK Ivk_CheckRemoveNpc_Union PATCH( &oCSpawnManager::CheckRemoveNpc, &oCSpawnManager::CheckRemoveNpc_Union );
  int oCSpawnManager::CheckRemoveNpc_Union( oCNpc* npc ) {
    if ( !Options::RemoveBodies && npc->fadeAwayTime <= 0.0f ) {
      return THISCALL( Ivk_CheckRemoveNpc_Union )( npc );
    }

    if ( !npc || !npc->GetHomeWorld() ) {
      return THISCALL( Ivk_CheckRemoveNpc_Union )( npc );
    }

    if ( !npc->IsFullyDead() ) {
      return THISCALL( Ivk_CheckRemoveNpc_Union )( npc );
    }

    if ( npc->GetEM()->GetCutsceneMode() || npc->IsAPlayer() || ( !npc->IsConditionValid() && npc->attribute[NPC_ATR_HITPOINTS] > 0 ) ) {
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

    if ( npc->fadeAwayTime > 0.0f ) {
      this->InitCameraPos();
      auto dist = ( npc->GetPositionWorld() - this->camPos ).LengthApprox();

      if ( dist > this->GetRemoveRange() || Options::RemoveBodies == 2 ) {
        this->DeleteNpc( npc );
        return TRUE;
      }
      else {
        return npc->FadeAway();
      }
    }
    else {
      if ( Options::RemoveBodies == 1 ) {
        npc->SetCollDetDyn( FALSE );
        npc->SetSleeping( FALSE );
        npc->fadeAwayTime = 5000.0f;
      }
      else if ( Options::RemoveBodies == 2 ) {
        this->DeleteNpc( npc );
        return TRUE;
      }
    }

    return THISCALL( Ivk_CheckRemoveNpc_Union )( npc );
  }
}