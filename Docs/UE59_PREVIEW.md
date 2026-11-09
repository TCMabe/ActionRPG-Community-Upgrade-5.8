# Unreal Engine 5.9 Preview validation

This branch is an experimental compatibility investigation based on the validated 5.8.4 Community Upgrade. It does not replace `main`.

## Status

Validated November 9, 2026 against UE 5.9.0 Preview, changelist 60351404 (`++UE5+Release-5.9`). Editor/game compilation, all 179 project Blueprints, both GAS automation tests, Windows Development packaging, and the packaged gameplay smoke checks below pass with the 5.9 build defaults. This remains a preview release, not exhaustive gameplay certification.

The workflow defaults to `C:/Program Files/Epic Games/UE_5.9`, checks version `5.9.0`, and records the engine's complete `Build.version` in `Saved/EngineBuildVersion.json`. Use `-ExpectedEngineVersion` only for an intentionally selected 5.9 patch version; this is not a bypass for other engine versions.

Both targets use `BuildSettingsVersion.V8` and `EngineIncludeOrderVersion.Unreal5_9`, following the installed engine's upgrade diagnostics. No runtime C++ API fixes were required for the editor or game build. The workflow now checks that both named GAS tests actually ran successfully, rather than accepting any single passing test in the group. No new GAS, PCG, multiplayer, or animation framework is substituted as part of the compatibility check.

Validated compiler: MSVC 14.50.35739, Windows SDK 10.0.26100.0. The Blueprint audit reports zero errors, zero warnings, and zero failed loads. GAS automation reports two passing tests, one with the inherited empty-world navigation warning. Logs and generated reports remain local under `Saved`.

## Packaged results

- Windows Development build/cook/stage/archive completed with UAT exit code 0; 1,382 packages were discovered in the cook. No cook warnings/errors were reported.
- Main menu, loading/menu-to-arena transition, intro skipping, AI/combat, damage, kills, soul rewards, autoplay, game over, pause/options, and travel back to the menu were exercised.
- A separate copy of the 5.8 QA save loaded with its five souls, axe, and equipped Fireball. Fireball activation, mana use and cooldown presentation worked.
- A separate shop fixture was funded with 100 souls through editor Python solely for testing. Buying one Health Potion deducted 25 souls, displayed a successful purchase, and equipped one potion. Using it played the potion effect and reduced its count from 1 to 0. This fixture and its script are local ignored files; no starting-inventory or gameplay changes are shipped.
- After fully exiting and restarting the packaged game, the profile retained 75 souls, Fireball equipment, the consumed-potion state, and an audio-option change saved through Back.
- `DumpUnbuiltLightInteractions` reports zero lights and zero primitives with unbuilt interactions in both the menu and the loaded arena.
- All 873 editable content packages still match the starting 5.8.4 snapshot byte-for-byte. No asset resave or lighting rebake was required for these checks.

## Known limits and warnings

The packaged preview logs 13 nonfatal ToolsetRegistry warnings about missing baked SequencerScripting toolset manifests, plus a MovieScene warning about the wave-banner sound extending beyond its section. The tested gameplay continued without fatal errors or Blueprint `Accessed None` errors. These warnings are recorded, not suppressed or presented as a warning-free runtime.

Optional stable shader-key generation remains disabled from the 5.8 port. The 5.9 shader-key serialization path has not been separately certified, and first-use PSO compilation can still cause hitches. Full boss/wave completion, every item/ability combination, long-session play, Vulkan runtime, mobile/Mac, multiplayer, and browser packaging remain unverified. The child/duplicate goblin test covers initialization rather than complete AI/combat/death/wave behavior.

## Reproduce

Install MSVC Build Tools v14.50 for x64/x86 alongside your existing toolchains (component `Microsoft.VisualStudio.Component.VC.14.50.18.0.x86.x64`). The included `.vsconfig` lists the build dependencies. This engine explicitly bans all 14.51 compilers and 14.50 compiler versions before 14.50.35723. The toolchain folder can still be named `14.50.35717`; verify the compiler version reported by UnrealBuildTool rather than relying on the folder name. The initial build was blocked by this toolchain check before any project code compiled.

Close the editor, then run from this separate project directory:

```powershell
.\Scripts\Build-Sample.ps1 -Phase Validate -RunAutomation
.\Scripts\Build-Sample.ps1 -Phase Package
```

Run the asset upgrade/resave phase only after inspecting load/compile diagnostics. Rebuild lighting if the new engine invalidates or changes the existing baked data. A package from 5.8 is not evidence that a 5.9 build passed.

## Required checks

- Editor and Windows game targets build.
- All project Blueprints compile and load.
- Both `ActionRPG.Port` GAS tests pass, with warnings recorded.
- Windows Development cook and package complete.
- Fresh package: menu/arena travel, combat, fireball cooldown/mana, potion consumption, shop, inventory, save/reload, lighting, and loading screen.
- Child/duplicate enemy combat, death and wave advancement remain separate functional QA beyond the initialization test.

## Official references

- [5.9 MegaJam Preview announcement](https://forums.unrealengine.com/t/unreal-engine-5-9-megajam-preview/2852840)
- [5.9 release notes](https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-9-release-notes)

Epic identifies this as a preview and recommends testing on a project copy. Optional MCP/Python, IK retargeting, PCG and browser features do not automatically become dependencies of this sample.
