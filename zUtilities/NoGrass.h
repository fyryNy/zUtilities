// Supported with union (c) 2020 Union team
// Union HEADER file

namespace GOTHIC_ENGINE {
  namespace Options {
    bool NoGrass;
    bool NoGrassDebugShowHidden;
    bool NoGrassRemoveVobsWithDynamicCollisions;
    std::vector<string> NoGrassVisualNames;
  } // namespace Options

  class NoGrass {
  private:
    std::vector<string> visualsNames;
    // Each tracked vob owns one reference until its visibility is restored.
    std::unordered_map<zCVob*, bool> hiddenVobs;
    bool IsValidVob( zCVob* vob );

  public:
    bool IsHidden( zCVob* vob ) const;
    bool GetOriginalVisibility( zCVob* vob ) const;
    void ForgetVob( zCVob* vob );
    void ShowVisualsNames();
    void RestoreVisibility();
    void UpdateVisualsList();
    void UpdateVisuals();
    void Update();
  };

  NoGrass noGrass;
} // namespace GOTHIC_ENGINE
