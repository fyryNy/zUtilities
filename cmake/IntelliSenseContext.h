#pragma once

// Forced only for standalone .cpp fragments in compile_commands.json.
// The real DLL build still includes those fragments through Sources.h.
#include "IntelliSenseActivePreset.h"
#include "UnionAfx.h"

#define Engine_G1  1
#define Engine_G1A 2
#define Engine_G2  3
#define Engine_G2A 4

// The selected CMake preset supplies these definitions. For multi-platform
// presets, use G2A as the representative standalone-fragment context, matching
// the old editor behavior while still following single-engine presets exactly.
#if defined(__G2A)
#define GOTHIC_ENGINE Gothic_II_Addon
#define ENGINE Engine_G2A
#elif defined(__G2)
#define GOTHIC_ENGINE Gothic_II_Classic
#define ENGINE Engine_G2
#elif defined(__G1A)
#define GOTHIC_ENGINE Gothic_I_Addon
#define ENGINE Engine_G1A
#elif defined(__G1)
#define GOTHIC_ENGINE Gothic_I_Classic
#define ENGINE Engine_G1
#else
#error "No Union game definition supplied by the active CMake preset"
#endif

#define CHECK_THIS_ENGINE (Union.GetEngineVersion() == ENGINE)

#include "Headers.h"
#include "Sources.h"
