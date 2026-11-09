# ActionRPG Community Upgrade — Unreal Engine 5.9 Preview investigation

**Experimental branch: not yet validated on UE 5.9.** The published `main` branch remains the verified 5.8.4 baseline. This checkout is an isolated preview investigation; the 5.8.4 evidence below describes the starting point, not a successful 5.9 conversion.

Read [5.9 preview status and verification](Docs/UE59_PREVIEW.md). The build workflow in this branch now defaults to UE_5.9 and expects 5.9.0; do not run its upgrade/resave phase against your stable 5.8 project.

---

## Original 5.8.4 baseline documentation

This is a faithful Windows port of Epic's original ActionRPG sample. It retains the original gameplay, GAS abilities, assets, native class names, and source/content paths while addressing engine compatibility. This repository is the Community Upgrade: a faithful sample baseline, separate from the expanded Community Mod.

The editor and Windows Development package build successfully. All 179 project Blueprints compile without errors or warnings, and both focused GAS regression tests pass. This is a working port candidate with partial interactive QA, not exhaustive gameplay certification. This is an unofficial, free community release; see LICENSE.md for attribution and applicable Epic terms. No multiplayer, Paragon integration, or PCG features are included. The inherited mobile configurations remain present; this port's validation targets Windows only.


## Add your own items and enemies

- [Documentation index and architecture](Docs/README.md)
- [Adding items: weapons, skills, potions, tokens, shops, pickups, GAS, and saves](Docs/ADDING_ITEMS.md)
- [Adding enemies: variants, meshes, animation, AI, stats, rewards, and waves](Docs/ADDING_ENEMIES.md)
- [Contributing and verification](CONTRIBUTING.md)

The guides use the sample's actual asset paths, Blueprint defaults/graphs, and native source. They identify inherited limitations rather than silently changing the sample's behavior.

## Local prerequisites

- Unreal Engine **5.8.4**, installed by default at `C:/Program Files/Epic Games/UE_5.8`.
- Visual Studio with the C++ toolchain and Windows SDK required by that engine installation.
- Windows PowerShell 5.1 or PowerShell 7, enough disk space for the engine, intermediate files, and a Windows package.
- Unreal Engine access and compliance with the applicable Epic terms. This repository includes the upgraded original sample content; the engine and additional content packs are not bundled.

Close the editor before upgrading assets or rebuilding its modules. Back up or commit local changes first: Upgrade and Lighting write project assets. Lighting requires rendering support and can take a long time.

## Reproduce the port locally

Clone the repository, then run from the project directory:

```powershell
git clone https://github.com/TCMabe/ActionRPG-Community-Upgrade-5.8.git
cd ActionRPG-Community-Upgrade-5.8
```

```powershell
# Default: build the editor and compile project Blueprints.
.\Scripts\Build-Sample.ps1

# Repair/resave, rebuild production lighting, validate, then package.
.\Scripts\Build-Sample.ps1 -Phase All -RunAutomation

# Rebuild production lighting separately (potentially slow).
.\Scripts\Build-Sample.ps1 -Phase Lighting

# An engine installed elsewhere.
.\Scripts\Build-Sample.ps1 -EngineRoot 'D:/Epic Games/UE_5.8' -Phase Validate -RunAutomation
```

Every phase first builds `ActionRPGEditor Win64 Development`. Each native command's exit status is checked; the script stops on failure.

| Phase | Work performed |
| --- | --- |
| `Upgrade` | Run `RPGUpgradeSample`, then `ResavePackages -ProjectOnly -IgnoreChangelist`. |
| `Lighting` | Run `ResavePackages -BuildLighting -Quality=Production -AllowCommandletRendering -Map=/Game/Maps/ActionRPG_Main+/Game/Maps/ActionRPG_P -MapsOnly -ProjectOnly -IgnoreChangelist` to rebuild production lighting for both entry maps and their streamed content. |
| `Validate` (default) | Generate `Saved/BlueprintAllowList.txt` from every project `.uasset` as exact `/Game/Path/Asset.Asset` object paths; run `CompileAllBlueprints` against that list. Add `-RunAutomation` for the `ActionRPG.Port` test group. |
| `Package` | Use `RunUAT BuildCookRun` to build, cook, stage, pak, and archive Win64 Development to `Artifacts/Windows`. |
| `All` | Run Upgrade, Lighting, Validate, and Package in that order. `-RunAutomation` enables automation during validation. |

`Package` does not implicitly run the asset upgrade, lighting rebuild, or Blueprint validation. Run `All` for the full sequence. Engine compatibility is checked against 5.8.4. Command arguments use arrays and forward paths so spaces in installation/project paths remain intact.

Lighting builds `ActionRPG_Main` first, then `ActionRPG_P` as the final composed world. Building `ActionRPG_P` loads and makes both streaming sublevels visible and saves their build data with the parent world's lights and importance volume. The workflow does not bake `Dungeon02_Asset` independently.

The editor-only `Source/ActionRPGEditor` module contains the upgrade commandlet and the `ActionRPG.Port` regression tests. The commandlet repairs the `GE_GoblinRange` granted cooldown tag, replaces two deprecated navigation nodes while preserving their pure evaluation behavior, removes orphan widget animation bindings, and repairs an obsolete TextBlock color property in `WB_WaveStart`. Existing runtime source and asset paths remain in place.

Commandlet logs are under `Saved/Logs/Port-*.log`; automation reports are under `Saved/Automation`. Packaging also emits Unreal Automation Tool logs at the location printed by UAT. Inspect failures before rerunning the relevant phase.

With `-RunAutomation`, the previous `Saved/Automation/index.json` is removed before launching the test process. Validation requires a newly written report from that run, at least one passing test, and zero failed, not-run, or in-process tests; process exit code alone is insufficient. Tests that pass with warnings are reported explicitly.

## Validation and distribution status

Current evidence includes a successful Windows package, clean compilation of 179 project Blueprints, and two passing GAS automation tests. The cooldown/possession test has one known warning: its empty editor world has no RecastNavMesh. The goblin variant test creates transient child and duplicate Blueprints, explicitly possesses each pawn, checks inherited stats/melee ability/Enemy tag, and verifies a 1.5-scale variant raised to level 3 through SetCharacterLevel has increased health. It does not certify AI navigation, attacks, death, or wave completion for new enemies. Packaged interactive checks have exercised combat and damage, kills and soul rewards, inventory, fireball purchase/equip/cooldown/mana, and potion consumption.

Production lighting has been rebuilt and packaged for both entry maps and the streamed dungeon. The packaged rendering check reports zero lights and zero primitives with unbuilt interactions. Interactive QA also exercised pause/options, game over, menu/arena transitions, and persistence of earned currency and inventory across restart. Full boss/wave completion, every equipment/skill combination, and extended play remain unverified. See [migration notes](Docs/MIGRATION_NOTES.md) for precise evidence and limitations.

Optional stable shader-key generation is disabled because the installed 5.8.4 engine asserts during its serialization self-test. Cooked shader libraries and runtime PSO caching remain available. The engine also warns that the installed MSVC 14.51 compiler is newer than its preferred 14.50 version.

This repository contains the editable upgraded sample content and source. Clone the complete repository; Git LFS is not required. Build outputs, engine files, personal saves, and the separate Community Mod roadmap are excluded.

Epic retains ownership of its original materials. Preserve notices and comply with the applicable Epic agreements when using or redistributing this project. See [licensing and attribution](LICENSE.md).
