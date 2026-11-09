# ActionRPG Community Upgrade — Unreal Engine 5.9 Preview

Experimental `preview/ue5.9` branch of the faithful Epic ActionRPG community upgrade. The repository's `main` branch remains the validated **UE 5.8.4** release. This branch targets **UE 5.9.0 MegaJam Preview**, changelist **60351404**.

Editor/game compilation, all 179 project Blueprints, both GAS automation tests, Windows Development packaging, and packaged gameplay smoke checks pass on 5.9. See [preview status and limits](Docs/UE59_PREVIEW.md). This is a preview compatibility port, not exhaustive gameplay certification.

The sample retains its original gameplay, assets, class names, GAS architecture, inventory, AI, and wave tables. Multiplayer, Paragon integration, PCG dungeon generation, and IGF are separate Community Mod work.

## Get started

Install UE 5.9 Preview alongside your existing engines. Use a separate project copy and save directory for preview testing.

```powershell
git clone --branch preview/ue5.9 https://github.com/TCMabe/ActionRPG-Community-Upgrade-5.8.git ActionRPG_59_Preview
cd ActionRPG_59_Preview
.\Scripts\Build-Sample.ps1 -Phase Validate -RunAutomation
```

Open `ActionRPG.uproject` and start from `/Game/Maps/ActionRPG_Main` for the original menu/save flow. Git LFS is not required; the editable sample content is included. Obtain the engine separately from Epic.

## Build requirements

- UE **5.9.0 Preview**; default path `C:/Program Files/Epic Games/UE_5.9`.
- Visual Studio with **MSVC 14.50.35723 or newer in the 14.50 family** and Windows SDK 10.0.26100.0. Validated with MSVC **14.50.35739**. UE 5.9 explicitly bans MSVC 14.51 and older 14.50 revisions; see the included [.vsconfig](.vsconfig).
- Windows PowerShell 5.1 or PowerShell 7 and sufficient space for intermediates and cooked output.

The workflow checks the exact engine version and records its Build.version locally. For an intentionally selected later 5.9 patch, use `-ExpectedEngineVersion 5.9.x` with the actual numeric patch value. The stable 5.8.4 workflow remains on `main`.

## Build workflow

Close the editor before building its modules or saving assets through commandlets.

```powershell
# Editor build, Blueprint audit, and both required GAS tests.
.\Scripts\Build-Sample.ps1 -Phase Validate -RunAutomation

# Build/cook/stage/archive Windows Development output.
.\Scripts\Build-Sample.ps1 -Phase Package

# Use an engine installed elsewhere.
.\Scripts\Build-Sample.ps1 -EngineRoot 'D:/Epic Games/UE_5.9' -Phase Validate -RunAutomation
```

| Phase | Purpose |
| --- | --- |
| Validate | Build editor, compile project Blueprints; optionally run the two named GAS tests. |
| Package | Build, cook, stage, pak, and archive Windows Development to `Artifacts/Windows`. |
| Upgrade | Run the inherited repair commandlet and resave project content. Writes assets. |
| Lighting | Rebuild production lighting for Main, then the composed P world. Writes maps/build data. |
| All | Upgrade, Lighting, Validate, then Package. |

Package does not automatically run the other phases. Upgrade and Lighting should be used deliberately on this preview copy, with changes committed/backed up first. Logs are in `Saved/Logs` and automation results in `Saved/Automation`.

Both targets use V8 build settings and the 5.9 include order. No runtime C++ API changes were needed for the editor or game build. Optional stable shader-key generation remains disabled from the 5.8 port; offline stable PSO-cache generation has not been certified on 5.9.

## Add items and enemies

- [Documentation index and data flow](Docs/README.md)
- [Items: weapons, skills, potions, tokens, shops, pickups, GAS, and saves](Docs/ADDING_ITEMS.md)
- [Enemies: variants, stats, AI, animation, rewards, death, and waves](Docs/ADDING_ENEMIES.md)
- [Preview validation and known limits](Docs/UE59_PREVIEW.md)
- [Original 5.8 migration history](Docs/MIGRATION_NOTES.md)
- [Contributing](CONTRIBUTING.md)

The authoring guides were established against the 5.8 sample and describe the retained gameplay architecture. Goblin child/duplicate initialization passes on 5.9; full new-enemy AI/combat/death/wave verification is still a separate requirement.

This is an unofficial, free community project. Preserve Epic notices and follow the applicable terms in [licensing and attribution](LICENSE.md). The engine, additional asset packs, and build outputs are not bundled.