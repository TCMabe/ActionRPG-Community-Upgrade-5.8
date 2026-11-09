# Unreal Engine 5.9 Preview validation

This branch is an experimental compatibility investigation based on the validated 5.8.4 Community Upgrade. It does not replace `main`.

## Status

The isolated project and workflow are prepared. Engine installation, C++ compilation, Blueprint validation, automation, and packaging on 5.9 have not yet completed. No 5.9 compatibility certification is implied by changing the project association.

The workflow defaults to `C:/Program Files/Epic Games/UE_5.9`, checks version `5.9.0`, and records the engine's complete `Build.version` in `Saved/EngineBuildVersion.json`. Use `-ExpectedEngineVersion` only for an intentionally selected 5.9 patch version; this is not a bypass for other engine versions.

Build settings V7 and the 5.8 include order are initially retained so actual compiler diagnostics can identify required changes. No new GAS, PCG, multiplayer, or animation framework is substituted as part of the compatibility check.

## Reproduce

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
