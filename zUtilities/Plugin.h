// This file added in headers queue
// File: "Headers.h"

namespace GOTHIC_ENGINE {
  // Shared entry points for the source fragments aggregated by Sources.h.
  float CoerceInRange( float start, float length, float minValue, float maxValue );
  bool Intersects( const tagRECT& a, const tagRECT& b );
  string GetArchivePath( string sav );
  zSTRING GetAttributeName( int att );
  void RegisterCommands();
  void HandleMunitionLoop();
  void RenderSelectedItem();
  void Archive();
  void Unarchive();

  extern oCItem* lastActiveMunition;
  extern oCNpc* playerTrfCopy;
#if ENGINE >= Engine_G2
  extern std::vector<oCVisualFX*> vfxs;
#endif
}
