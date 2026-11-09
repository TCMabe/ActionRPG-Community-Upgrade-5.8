# Faithful ActionRPG sample migration

Target: installed Unreal Engine 5.8.4, Windows Development. Original 4.27 project remains in the sibling ActionRPG folder. Native class names and existing asset paths are preserved.

## Native compatibility and GAS changes

- Set the engine association to 5.8 and use BuildSettingsVersion.V7 and Unreal5_8 include order.
- Update legacy WindowsNoEditor/MacNoEditor descriptor platform names to Windows/Mac. Only Windows is verified here.
- Remove unavailable SlateRemote. Disable AndroidFileServer so a Windows sample conversion does not generate unrelated file-server configuration.
- Expose GAS and AI module dependencies used by the runtime module's public headers.
- Resolve the ability task's weak ASC pointer explicitly, include AbilitySystemLog directly, and unwrap the target actor's TObjectPtr before const_cast.
- End the montage/event task after playback failure, interruption, and successful cancellation; guard null ability access and invalid ASC access. Preserve the original Blueprint event names and normal blend-out behavior.
- Restore Super::UnPossessed and refresh cached ASC actor information after controller removal.
- Register replicated GAS attributes with REPNOTIFY_Always, retaining their existing RepNotify implementations. This follows GAS prediction conventions; it does not make the sample multiplayer-ready.
- Give the loading-screen GC referencer a name and retain its texture through a persistent TObjectPtr collected by FGCObject.

## Asset and configuration conversion

- Resave all 873 project packages, including four maps, with UE 5.8 serialization and automatic engine conversions.
- Repair GE_GoblinRange using a Grant Tags to Target Actor gameplay-effect component and the isolated Cooldown.Goblin.Ranged tag. The ranged ability already used this finite-duration effect as its cooldown; 5.8 rejects a cooldown effect that grants no tags. Its existing damage, duration, and ability flow are retained.
- Replace the deprecated K2_GetRandomPointInNavigableRadius call in BTTask_GoAroundTarget and BP_PlayerCharacter with K2_GetRandomLocationInNavigableRadius. Their signatures match and the deprecated implementation forwards to the replacement. Preserve pure evaluation and existing data links; enable the editor's supported pure-node conversion option.
- Remove four animation bindings to widgets absent from the inherited UI trees (NPCKillCount in both HUDs; CanvasPanel_0 in WB_Equipment; CanvasPanel_1 in WB_PurchaseConfirm). Keep the animations and valid bindings. These orphan bindings already had no runtime target; no replacement visual behavior is invented.
- Repair WB_WaveStart's color track bound to WaveText from ContentColorAndOpacity to the TextBlock's ColorAndOpacity. Preserve its existing channels/timing and leave the valid Border color track alone. MovieScene supports FSlateColor directly.
- Rebuild Production lighting for ActionRPG_Main, then ActionRPG_P with both streamed dungeon sublevels loaded. Save the maps and associated build data; avoid independently baking the geometry-only child map without its parent lights/importance volume.
- Rename packaging INI denylist keys and update the removed IOS_12 enum value to the earliest supported IOS_15. Android/iOS/Mac builds are not certified by this Windows validation pass.
- Retain UE 5.8's serialized input defaults/axis metadata while preserving the sample's action and axis mappings. Existing legacy action bindings remain in use; no Enhanced Input action-asset redesign is introduced.
- Disable optional bNeedsShaderStableKeys generation: the installed 5.8.4 engine asserts in ShaderPipelineCacheToolsCommandlet's StableShadersSerializationSelfTest after cooking all 1,366 discovered packages. The original project contains no bundled stable PSO cache. Runtime shader compilation/cooked shader libraries remain enabled; offline stable PSO-cache generation is not certified by this port.
- Add an editor-only, repeatable RPGUpgradeSample commandlet so asset repairs can be applied to a user's own original sample rather than distributing modified content binaries.

## Validation evidence

- Editor target: passes with the installed Visual Studio 14.51.36260 and Windows SDK 10.0.26100.0. The engine warns that this compiler is newer than its preferred 14.50 version.
- Project Blueprint audit: 179 Blueprints compiled, zero errors, zero warnings, zero failed loads (Saved/Logs/Port-Blueprints.log).
- ActionRPG.Port.GASCooldownAndPossession: passes. Exercises possession and ASC actor info, application/removal of the saved goblin cooldown and its granted tag, and unpossession cleanup. An empty-test-world navigation warning is recorded; no test errors (Saved/Automation/index.json).
- Windows Development build, cook, stage, pak, archive: passes. All four inherited maps are present through entry-map and streaming references; 1,366 discovered cook packages complete. Output: Artifacts/Windows/ActionRPG.exe.
- Final packaged dungeon runs offscreen on the rendering RHI and DumpUnbuiltLightInteractions reports zero lights and zero primitives with unbuilt interactions (Saved/Logs/PackagedLightingCheck.log).
- Interactive packaged checks exercised menu-to-arena and return travel, intro skipping, AI/autoplay melee combat, damage and kill rewards, inventory, purchase confirmation and soul deduction, fireball equipping/activation/mana/cooldown, potion use/decrement, pause/options, and the game-over flow. Restarting reloaded previously earned souls and inventory. Tests use an isolated Saved/PackagedQA user directory.
- Integrity check: all 988 original-project files match the untouched baseline (Saved/OriginalIntegrity.json).
- Source-only community patch: applies cleanly to an isolated copy of the original source/configuration; all 20 resulting files match the validated port after line-ending normalization (Saved/UpgradePatchQA.json). Content assets are not included. This checks patch application and source equivalence, not a second full build from scratch.

This is a working Windows port candidate, not exhaustive gameplay or platform certification. Full boss/wave completion, every weapon/skill combination, and mobile/Mac remain unverified. The final wave-banner property repair is compiled and source-reviewed; additional visual polish/long-session QA remains useful. Production lighting succeeds with four nonfatal warnings (including the main menu's missing importance volume); the minimal GAS test passes with an empty-world navigation warning. Stable offline PSO generation remains disabled as described above.

The first broad resave logged transient import problems while converting the legacy options struct and dependent widget; the subsequent fresh-process Blueprint audit and Windows cook are clean. Options can be changed and saved through the Back action. Previously saved inventory and currency reload in the packaged game.

## Community release boundary

The public Community Upgrade contains the faithful sample port, editable sample content, build workflow, and item/enemy authoring guides. Multiplayer, Paragon/Infinity Blade additions, PCG, and the separate Community Mod roadmap are outside this release. Original Epic notices are retained; see [licensing and attribution](../LICENSE.md).
## Enemy variant regression coverage

Both `ActionRPG.Port` tests pass. `GoblinVariantInitialization` creates transient child and duplicate Blueprints from `NPC_Goblin_Level_01`; validates inherited stats, Enemy tag, and melee ability after explicit possession; and checks increased health after scaling variants to 1.5 and raising them to level 3 using `SetCharacterLevel`. This is initialization coverage, not a full AI/combat/death/wave test or a diagnosis of every historical duplication report. No additional enemy assets are shipped.
