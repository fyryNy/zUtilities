// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
  void IconInfo::Draw() {
    if ( !ogame || !player )
      return;

    if ( quickSave->IsBusy() || !ogame->GetShowPlayerStatus() || !ogame->game_drawall || ogame->IsOnPause() || playerHelper.IsInInfo() || player->attribute[NPC_ATR_HITPOINTS] <= 0 )
      return;

    RestoreViewport(); //because some plugins don't restore viewport after Blitting, so it messes with prints in other plugins
    DrawIcon();
    PrintText();
  }

  void IconInfo::DrawIcon() {
    px1 = screen->nax( vx );
    py1 = screen->nay( vy );
    psize = screen->nax( vsize );
    px2 = px1 + psize;
    py2 = py1 + psize;

    float z = ZenDef( 0.9f, 0.9f, 1.1f, 1.1f );
    zCTexture* texture = texture->Load( texName, true );
    const zVEC2 topLeft( static_cast<float>( px1 ), static_cast<float>( py1 ) );
    const zVEC2 bottomRight( static_cast<float>( px2 ), static_cast<float>( py2 ) );
    zrenderer->DrawTile( texture, topLeft, bottomRight, z, zVEC2( 0.0f, 0.0f ), zVEC2( 1.0f, 1.0f ), color );
    texture->Release();
  }

  void IconInfo::PrintText() {
    if ( !text.Length() )
      return;

    int x = vx + vsize + screen->FontY() / 10;
    int y = vy + ( screen->any( py2 ) - vy ) / 2 - screen->FontY() / 2;

    zCView* view = new zCView( 0, 0, 8192, 8192 );
    view->SetFontColor( zCOLOR( 255, 255, 255, color.alpha ) );
    screen->InsertItem( view );
    view->Print( x, y, text );
    view->BlitText();
    delete view;
  }

  int IconInfo::GetSize() {
    if ( !text.Length() ) {
      return vsize;
    }

    return vsize + screen->FontY() / 10 + screen->FontSize( text );
  }

  void IconInfo::RestoreViewport() {
    int ScreenX, ScreenY, ScreenSX, ScreenSY;
    screen->GetViewport( ScreenX, ScreenY, ScreenSX, ScreenSY );
    zrenderer->SetViewport( ScreenX, ScreenY, ScreenSX, ScreenSY );
  }

  IconInfo::IconInfo( int x, int y, int size, zCOLOR color, zSTRING texName, zSTRING text )
    : vx( x ), vy( y ), vsize( size ), color( color ), texName( texName ), text( text ) {
    Draw();
  };
}
