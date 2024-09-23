// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
		HOOK Ivk_CheckRemoveNpc_Union PATCH( &oCSpawnManager::CheckRemoveNpc, &oCSpawnManager::CheckRemoveNpc_Union );
		int oCSpawnManager::CheckRemoveNpc_Union( oCNpc* npc ) {
				if ( !Options::RemoveBodies ) {
						return THISCALL( Ivk_CheckRemoveNpc_Union )( npc );
				}

				if ( !npc || !npc->GetHomeWorld() ) {
						return THISCALL( Ivk_CheckRemoveNpc_Union )( npc );
				}

				if ( npc->GetEM()->GetCutsceneMode() || npc->IsAPlayer() || ( !npc->IsConditionValid() && npc->attribute[NPC_ATR_HITPOINTS] > 0 ) ) {
						return THISCALL( Ivk_CheckRemoveNpc_Union )( npc );
				}

				if ( npc->attribute[NPC_ATR_HITPOINTS] > 0 ) {
						return THISCALL( Ivk_CheckRemoveNpc_Union )( npc );
				}

#if ENGINE >= Engine_G2
				if ( npc->GetGuild() == NPC_GIL_DRAGON ) {
						return THISCALL( Ivk_CheckRemoveNpc_Union )( npc );
				}
#endif

				npc->inventory2.UnpackAllItems();
				int InventoryEmpty = FALSE;
#if ENGINE <= Engine_G2
						InventoryEmpty = npc->inventory2.IsEmpty();
#else
						InventoryEmpty = npc->inventory2.IsEmpty( TRUE, TRUE );
#endif

				if ( !InventoryEmpty || npc->HasMissionItem() ) {
						return THISCALL( Ivk_CheckRemoveNpc_Union )( npc );
				}

				if ( !npc->human_ai->IsDead() ) {
						return TRUE;
				}

				if ( npc->IsFadingAway() ) {
						this->InitCameraPos();
						auto dist = ( npc->GetPositionWorld() - this->camPos ).LengthApprox();

						if ( dist > this->GetRemoveRange() ) {
								this->DeleteNpc( npc );
						}
						else {
								npc->FadeAway();
						}

						return TRUE;
				}
				else {
						npc->StartFadeAway();

						return TRUE;
				}

				return FALSE;
		}
}