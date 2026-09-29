// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
  oCNpc* playerTrfCopy = nullptr;

  HOOK Hook_oCNpc_CopyTransformSpellInvariantValuesTo PATCH( &oCNpc::CopyTransformSpellInvariantValuesTo, &oCNpc::CopyTransformSpellInvariantValuesTo_Union );
  void oCNpc::CopyTransformSpellInvariantValuesTo_Union( oCNpc* npc ) {
    if ( this == player )
      playerTrfCopy = this;

    THISCALL( Hook_oCNpc_CopyTransformSpellInvariantValuesTo )( npc );
  }

  // Inspiration @G2-Ucieczka - https://github.com/TheKetrab/G2-Ucieczka
  HOOK Hook_oCNpc_OnTouch PATCH( &oCNpc::OnTouch, &oCNpc::OnChrzonszcz );
  void oCNpc::OnChrzonszcz( zCVob* vob ) {
    oCNpc* npc = vob->CastTo<oCNpc>();
    zCVisual* visual = vob->GetVisual();

    if ( !npc || !visual || !this->IsSelfPlayer() || !this->IsHuman() || vob == playerTrfCopy || npc->guild != NPC_GIL_MEATBUG || npc->HasFlag( NPC_FLAG_IMMORTAL ) || !Options::TrampleMeatbugs ) {
      THISCALL( Hook_oCNpc_OnTouch )( vob );
      return;
    }

    bool largeMdlScale = false;
    if ( npc->model_scale[0] > 1.0f || npc->model_scale[1] > 1.0f || npc->model_scale[2] > 1.0f )
      largeMdlScale = true;

    if ( visual->GetVisualName() != "MEATBUG.MDS" || largeMdlScale ) {
      THISCALL( Hook_oCNpc_OnTouch )( vob );
      return;
    }

    // Kill and Exp
    npc->DoDie( this );
    if ( npc->attribute[NPC_ATR_HITPOINTS] > 0 )
      npc->attribute[NPC_ATR_HITPOINTS] = 0;

    // Sound
    if ( zCSoundFX* snd = zsound->LoadSoundFX( "CS_IAM_UD_FL_0" + Z randomizer.Random( 1, 5 ) + ".WAV" ) ) {
      zCSoundSystem::zTSound3DParams params;
      zsound->GetSound3DProps( 0, params );
      zsound->PlaySound3D( snd, vob, 0, &params );
      snd->Release();
    }

    //// Blood
    //oCVisualFX::CreateAndPlay( "BFX_PRESET2", npc, nullptr, 0, 0, 0, 0 );

    THISCALL( Hook_oCNpc_OnTouch )( vob );
  }
}