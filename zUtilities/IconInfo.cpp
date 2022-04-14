// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
  void IconInfo::DrawIcon( zSTRING texName ) {
    px1 = screen->nax( vx );
    py1 = screen->nay( vy );
    psize = screen->nax( vsize );
    px2 = px1 + psize;
    py2 = py1 + psize;

    float z = ZenDef( 0.9f, 0.9f, 1.1f, 1.1f );
    zCTexture* texture = texture->Load( texName, true );
    zrenderer->DrawTile( texture, zVEC2( px1, py1 ), zVEC2( px2, py2 ), z, zVEC2( 0, 0 ), zVEC2( 1.0f, 1.0f ), color );
    texture->Release();
  }

  void IconInfo::PrintText( zSTRING text ) {
    int x = vx + vsize + screen->FontY() / 10;
    int y = vy + (screen->any( py2 ) - vy) / 2 - screen->FontY() / 2;

    zCView* view = new zCView( 0, 0, 8192, 8192 );
    view->SetFontColor( zCOLOR( 255, 255, 255, color.alpha ) );
    screen->InsertItem( view );
    view->Print( x, y, text );
    view->BlitText();
    delete view;
  }

  IconInfo::IconInfo( int x, int y, int size, zCOLOR col, zSTRING texName, zSTRING text = "" ) {
    if ( !ogame || !player )
      return;

    if ( quickSave->IsBusy() || !ogame->GetShowPlayerStatus() || ogame->IsOnPause() || playerHelper.IsInInfo() || player->attribute[NPC_ATR_HITPOINTS] <= 0 )
      return;

    vx = x;
    vy = y;
    vsize = size;
    color = col;

    DrawIcon( texName );

    if ( text.Length() )
      PrintText( text );
  }
}