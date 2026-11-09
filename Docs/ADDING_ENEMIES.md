# Adding enemies to ActionRPG for UE 5.8.4

This guide extends the sample's existing single-player enemy, Gameplay Ability System (GAS), and wave systems. Start with a working goblin variant, then replace its behavior or animation assets one dependency at a time. A mesh, a pawn Blueprint, an attack ability, and a wave entry are separate parts of an enemy; copying one does not register the others.

Paths beginning with `/Game/` are Content Browser package paths. For example, `/Game/Blueprints/NPC/NPC_GoblinBP` is [Content/Blueprints/NPC/NPC_GoblinBP.uasset](../Content/Blueprints/NPC/NPC_GoblinBP.uasset). Open binary assets in Unreal Editor. Proposed names such as `NPC_MyGoblin` below are examples to create, not included assets.

## Existing assets to study

| Responsibility | Existing asset or source |
| --- | --- |
| Native character, ASC, attribute set, startup grants | [RPGCharacterBase.h](../Source/ActionRPG/Public/RPGCharacterBase.h), [RPGCharacterBase.cpp](../Source/ActionRPG/Private/RPGCharacterBase.cpp) |
| Shared character behavior | `/Game/Blueprints/BP_Character` |
| Shared enemy behavior | `/Game/Blueprints/BP_EnemyCharacter` |
| Goblin template and existing variants | `/Game/Blueprints/NPC/NPC_GoblinBP`, `NPC_Goblin_Level_01`, `NPC_Goblin_Level_02`, `NPC_Goblin_Level_03` |
| Goblin animation Blueprint | `/Game/Blueprints/NPC/NPC_AnimBP_Base` |
| Goblin controllers and behavior tree | `/Game/Blueprints/AI/GruntAI/AIC_NPC`, `AIC_NPC_Range`, `BT_NPC` |
| Blackboard | `/Game/Blueprints/AI/BB_Base` |
| Goblin stats and attacks | `/Game/Abilities/Enemies/Goblin/GE_GoblinStats`, `GA_GoblinMelee`, `GE_GoblinMelee`, `GA_GoblinRange01`, `GE_GoblinRange` |
| Goblin montages | `/Game/Animations/NPC/AM_Guardian_Attack`, `AM_Guardian_Attack02`, `AM_Guardian_Hit`, `AM_Guardian_Falldown` |
| Goblin weapon actors | `/Game/Blueprints/Weapon/Goblin/GoblinWeapon_Base`, `GoblinWeapon_Axe`, `GoblinWeapon_Torch` |
| Different skeleton and boss behavior example | `/Game/Blueprints/Boss/NPC_SpiderBoss`, `ABP_SpiderBoss`; `/Game/Blueprints/AI/BossAI/AIC_Boss`, `BT_Boss`; `/Game/Abilities/Enemies/Spider/` |
| Spawn and wave data | `/Game/Blueprints/NPC/NPC_SpawnBox`; `/Game/Blueprints/Progression/WavesProgression`, `WavesStruct`, `SpawnGroupStruct` |
| Game coordination | `/Game/Blueprints/BP_GameMode`, `/Game/Blueprints/BP_GameState` |

The inheritance chain is `ARPGCharacterBase` → `BP_Character` → `BP_EnemyCharacter` → `NPC_GoblinBP` → the three goblin variants. The spider derives from `BP_EnemyCharacter` directly. The existing goblin variants differ in more than level:

| Variant | CharacterLevel | Controller | Weapon actor | Ability in Weapon slot 0 | Time bonus | SoulMIN–SoulMAX |
| --- | --- | --- | --- | --- | --- | --- |
| `NPC_Goblin_Level_01` | 1 | `AIC_NPC` | `GoblinWeapon_Base` | `GA_GoblinMelee` | 2 | 1–3 |
| `NPC_Goblin_Level_02` | 2 | `AIC_NPC_Range` | `GoblinWeapon_Axe` | `GA_GoblinRange01` | 3 | 2–4 |
| `NPC_Goblin_Level_03` | 3 | `AIC_NPC` | `GoblinWeapon_Torch` | `GA_GoblinMelee` | 4 | 3–5 |

All three use `GE_GoblinStats`, have Auto Possess AI set to **Placed in World or Spawned**, and retain the ordinary Actor tag `Enemy`. Use Level 01 for the smallest melee example and Level 02 for a ranged example. The names do not mean that all three share the same attack configuration.

## How startup stats and abilities work

The native character creates its own `URPGAbilitySystemComponent` and `URPGAttributeSet`. On possession it initializes ASC actor information and grants startup gameplay abilities. In standalone play the local game has authority, so these authority-only grants run normally.

The character's **Abilities** defaults expose:

| Native property | Purpose |
| --- | --- |
| `CharacterLevel` | Level passed into startup ability specs and passive effect specs. |
| `GameplayAbilities` | Ability classes granted for activation by tags or events. |
| `DefaultSlottedAbilities` | Ability classes associated with sample item slots. |
| `PassiveGameplayEffects` | Effects applied to initialize or maintain the character's attributes. |

Keep the template's stat effect until its replacement is ready. Without it the native defaults include Health and MaxHealth of 1, AttackPower and DefensePower of 1, and MoveSpeed of 1. An enemy can therefore exist in the world while its gameplay configuration is incomplete.

Use `CharacterLevel` in Class Defaults for a new enemy's initial level. To change an already initialized character, use `SetCharacterLevel`; it removes and regrants startup abilities/effects. This is a gameplay operation, not a harmless display-label change. Editing a level value does not itself define scaling: inspect the referenced effect magnitudes/curves to see what changes at that level.

For independent balance, create a separate stats effect based on `GE_GoblinStats`, then replace its entry in the new enemy's `PassiveGameplayEffects`. Do not append a second complete stats initializer accidentally. Inspect inherited modifier operations and scalable magnitudes before changing values. Copy a shared curve/table if only this enemy should change; editing a shared asset affects all users.

`GE_GoblinStats` is an **Instant** effect derived from `GE_StatsBase`. Its five Override modifiers use the Curve Table `/Game/Abilities/DataTables/StartingStats`: `DefaultMaxHealth`, `DefaultMaxMana`, `DefaultAttackPower`, `DefaultDefensePower`, and `DefaultMoveSpeed`. Their scalable-float coefficient is 1. This is where the supplied goblin levels obtain their stat scaling. Health/Mana are adjusted when their maxima change by the native attribute set; do not assume there must be separate Health/Mana modifier entries in this effect.

Attributes and damage handling are defined in [RPGAttributeSet.h](../Source/ActionRPG/Public/Abilities/RPGAttributeSet.h), [RPGAttributeSet.cpp](../Source/ActionRPG/Private/Abilities/RPGAttributeSet.cpp), and [RPGDamageExecution.cpp](../Source/ActionRPG/Private/Abilities/RPGDamageExecution.cpp). The sample damage execution computes:

```text
DamageDone = Damage × source AttackPower ÷ target DefensePower
```

A zero DefensePower is treated as 1. AttackPower and source Damage are captured when the effect spec is created; DefensePower is evaluated when damage executes. This matters for projectiles already in flight. `Damage` is a temporary attribute that is consumed into Health, then reset. Health is clamped between zero and MaxHealth. Keep this pipeline for normal combat so hit information, instigator attribution, and Blueprint damage callbacks remain available.

## First milestone: a goblin variant

1. Run the unchanged sample once and identify the goblin behavior you want to preserve.
2. In the Content Browser, create a child Blueprint of the closest existing goblin variant. Use a new folder/name such as `/Game/Blueprints/NPC/Custom/NPC_MyGoblin`. A child inherits future parent fixes; duplicating a Blueprint makes an independent copy of that Blueprint but still shares referenced abilities, montages, meshes, effects, and weapons.
3. Compile and save without changing behavior. Check Class Settings for the expected parent and compare Class Defaults with the source enemy, including inherited fields.
4. Change only a visual material or another clearly isolated default. Run one enemy before editing its abilities or wave integration.
5. Create separate stats/ability/weapon assets only where this variant needs independent behavior. Replace the corresponding references; merely duplicating a dependency does not make the pawn use it.
6. Test both a placed instance and an enemy spawned through the sample's actual wave path. These exercise different possession and spawn configuration.

Do not add a second ASC or AttributeSet component. They already exist in the native parent. When overriding an inherited event, inspect the parent implementation and preserve its required initialization/cleanup with a parent call where appropriate.

In `NPC_GoblinBP`, BeginPlay calls the parent, spawns the `Weapon` class using `WeaponTransform`, sets the spawned weapon's Instigator to the goblin, stores it in `CurrentWeapon`, and attaches it to the mesh at `b_MF_Weapon_R` with relative attachment rules. The existing variants use a weapon transform scale of 0.6. Changing the mesh or replacing BeginPlay without this work can leave an otherwise valid enemy unable to hit.

For a concrete first variant, create a child of `NPC_Goblin_Level_01`, keep Weapon slot 0 mapped to `GA_GoblinMelee`, keep `AIC_NPC`, and replace only its stats effect with your duplicated effect. Test that configuration before creating a custom melee ability. Changing `Weapon` changes the spawned weapon actor; it does not automatically replace the slotted GAS ability.

## Controller, navigation, and targeting

Verify the pawn's **AI Controller Class** and **Auto Possess AI** settings against the working template. If your own test uses both editor-placed and dynamically spawned actors, configure possession for both or explicitly create the appropriate controller in your spawning logic. A class setting is not proof that a particular live pawn has been possessed: inspect its controller during Play.

Possession is especially important here because native startup GAS grants run from `PossessedBy`. An unpossessed enemy may have an ASC object but lack its configured startup abilities and stats.

For navigation, test the actual playable persistent map with its streamed dungeon geometry. Show navigation in the viewport, verify a navigable surface under the spawn and target locations, and make sure the new capsule dimensions fit the available paths. A successful spawn does not prove that the AI can find a path. Check behavior-tree execution and blackboard values on the specific live enemy instead of changing movement speed blindly.

The native `GetGenericTeamId()` returns player team 0 when the pawn has an `APlayerController`, otherwise AI team 1. There is no editable per-enemy faction field in that implementation. Keep this rule for a sample enemy; adding friendly factions requires deliberate targeting changes. Generic team identity alone does not prove that every Blueprint trace or target query filters allies—inspect the targeting implementation used by the attack.

The existing `BTService_FindNearestTarget` also uses **ordinary Actor tags**. If the controlled actor has tag `Player`, it searches for actors tagged `Enemy`; otherwise it searches for `Player`. It validates candidate actors through `BP_Character` and `IsAlive`, then updates its blackboard keys. Keep `Enemy` in the enemy's Actor Tags array. Adding a GAS tag named Enemy instead does not satisfy this check. This distinction also matters in the sample's AI-controlled player mode.

`AIC_NPC` initializes `BB_Base`, runs `BT_NPC`, and writes its `bIsRanged` default into blackboard key `isRanged`. `AIC_NPC_Range` inherits that controller, sets `bIsRanged` true, and updates `CanSeePlayer` from perception of an actor tagged `Player`. A new ranged ability paired with the melee controller will not automatically switch the tree's range behavior.

`BTTask_AttackMelee` calls `BP_Character.DoMeleeAttack`. Despite that function's name, the function activates `CurrentWeaponSlot`, so the ranged goblin works through Weapon slot 0 containing `GA_GoblinRange01`. `DoSkillAttack` instead activates abilities tagged `Ability.Skill`. `BP_EnemyCharacter.CanUseAnyAbility` requires an alive character, an unpaused game, and no currently playing montage. Check those gates when an ability works in isolation but the AI will not use it.

## New attack abilities

Start from the closest attack under `/Game/Abilities/Enemies/Goblin` or `/Game/Abilities/Enemies/Spider`. Keep the existing activation, event, and cleanup structure while replacing the attack's data.

The native `URPGGameplayAbility` exposes `EffectContainerMap`, keyed by a Gameplay Tag. Each entry contains a `TargetType` and `TargetGameplayEffectClasses`. [RPGGameplayAbility.cpp](../Source/ActionRPG/Private/Abilities/RPGGameplayAbility.cpp) creates the outgoing specs at the granted ability's level unless an explicit override is supplied. [RPGTargetType.cpp](../Source/ActionRPG/Private/Abilities/RPGTargetType.cpp) includes owner targeting and targeting from event hit data or `EventData.Target`.

Check all of these connections when copying an attack:

1. The enemy grants the new ability through the correct startup/slotted ability configuration.
2. The AI activation tag or slot matches that granted ability.
3. The ability plays a montage compatible with the enemy mesh and animation Blueprint.
4. The montage/weapon/projectile generates the gameplay event expected by the ability.
5. The event has valid target or hit data, and the selected target type converts it into targets.
6. The selected effect container points to the intended damage effect.
7. Completion, interruption, cancellation, and playback-failure paths end/clean up correctly.

Relevant shared assets include `/Game/Abilities/Shared/GA_AbilityBase`, `GA_MeleeBase`, `GA_SpawnProjectileBase`, `BP_AbilityProjectileBase`, `GE_DamageBase`, `GE_MeleeBase`, and `GE_RangedBase`. For projectiles, also inspect `/Game/Abilities/Player/Skills/BP_SlimeBall` before replacing the goblin's projectile behavior.

Existing registered tags are in [DefaultGameplayTags.ini](../Config/DefaultGameplayTags.ini). They include `Ability.Melee`, `Ability.Ranged`, `Ability.Skill`, `EffectContainer.Default`, and `Event.Montage.Shared.WeaponHit`. These are different roles: an ability-identification tag, an effect-container key, and an animation event tag are not interchangeable.

The actual goblin melee connection is `GA_GoblinMelee.MontageToPlay` → `AM_Guardian_Attack`, with `EffectContainerMap[Event.Montage.Shared.WeaponHit]` using `RPGTargetType_UseEventData` and `GE_GoblinMelee`. Its ability tag is `Ability.Melee`. The ranged ability uses `AM_Guardian_Attack02`, `BP_SlimeBall`, and a container keyed by `Event.Montage.Shared.UseSkill`; its existing damage-effect reference is `GE_PlayerSkillFireball`. Preserve or deliberately replace that cross-folder reference—its filename does not mean it is unused by enemies.

For a new cooldown, use a finite-duration Gameplay Effect that grants its cooldown tag through UE 5.8's **Grant Tags to Target Actor** component. The existing goblin ranged cooldown is `GE_GoblinRange` with `Cooldown.Goblin.Ranged`; this port repairs the legacy effect's missing tag. Copy it with care: inspect its inherited configuration as well as duration. Use a new registered cooldown tag when the new attack should cool down independently. Keeping the same tag intentionally shares the cooldown on that actor.

The custom [PlayMontageAndWaitForEvent task](../Source/ActionRPG/Private/Abilities/RPGAbilityTask_PlayMontageAndWaitForEvent.cpp) combines montage callbacks and gameplay-event listening. Preserve its existing Blueprint delegate meanings. A montage that visibly plays is only one stage of a working attack; validate that a hit actually applies damage once.

## New mesh or skeleton

A different material or compatible mesh is a small variant. A different skeleton requires a complete animation pass. Use the spider boss as an existing example of a different animation and AI setup, not as an automatic replacement for goblin references.

1. Import the mesh, skeleton, materials, and physics asset into a separate content folder.
2. Fit the capsule and mesh transform to the new character. Verify ground contact and collision at runtime.
3. Assign an animation Blueprint built for the new skeleton. Retarget or author locomotion, attacks, hit reactions, spawning, and death animations as needed.
4. Replace every used montage with a compatible version. Preserve required montage sections, slot routing, notify timing, and root-motion behavior. A retargeted animation alone does not establish all of those gameplay connections.
5. Recreate required attachment and trace sockets on the new skeleton or update the consuming weapon/Blueprint references together. Inspect the working assets for their actual socket names; do not assume a naming convention.
6. Review weapon collision/traces and projectile launch transforms against the new proportions. Test near and far targets and attacks against walls.
7. Keep the shared death/reward/wave lifecycle intact while replacing visual death behavior.

Animation helpers are under `/Game/Blueprints/AnimNotifies`, including `WeaponAttackNS`, `RangeAttackNS`, `Hit_Notify`, `Death_Notify`, `StopAndStartAI_NS`, and movement-related notify states. Inspect which ones the source montage actually uses before copying them. Moving or omitting a gameplay notify can change attack windows even when the animation looks correct.

The goblin Anim Blueprint has Summon, Locomotion, and ShowDeath states. Its update graph casts the pawn owner to `BP_Character`, gets velocity for `Speed`, and reads `IsAlive` into `bAlive`; ShowDeath is reached when `bAlive` is false. Locomotion routes through `DefaultGroup.DefaultSlot`. Preserve equivalent montage routing and health-driven death transitions in a replacement Anim Blueprint. Goblin hit reactions are also selected in `NPC_GoblinBP.OnDamaged`, using `AM_Guardian_Hit` or `AM_Guardian_Falldown` and sections named `0` or `1`; replacing only the attack montage leaves those skeleton-dependent references behind.

## Death, drops, and rewards

There are three different milestones: Health reaches zero, the death presentation completes, and the actor is destroyed. The wave manager depends on the last one.

The verified shared enemy flow is:

1. GAS updates Health and calls `OnHealthChanged`. `BP_EnemyCharacter` updates damage text/health display and tests `IsAlive`.
2. On death, a Do Once gate invokes `OnDeathEvent`. It calls `BP_GameMode.Increase NPC Kill Count` with the enemy's `TimeBonusPerKill`, hides the relevant widgets, detaches the controller, changes capsule collision object type to WorldDynamic, and destroys a valid `CurrentWeapon`.
3. The animation system reacts to `IsAlive`. `Death_Notify` casts the mesh owner to `BP_Character` and calls `DelayedDestroy`. Despite its name, the current base `DelayedDestroy` event directly invokes Destroy Actor; the notify's placement supplies the presentation timing.
4. `BP_EnemyCharacter.Event Destroyed` runs `SpawnLoot` once, using a random inclusive integer between `SoulMIN` and `SoulMAX`, then calls the parent Destroyed event.
5. For enemies spawned by the wave manager, the actor's OnDestroyed delegate also updates the wave's active-enemy count.

Keep parent `OnDeathEvent` and Destroyed behavior when adding custom VFX. `NPC_Goblin_Level_03` demonstrates an extra death effect followed by a parent `OnDeathEvent` call. A custom corpse that is hidden but never destroyed can block the next wave. Conversely, destroying the pawn early can skip the intended death animation or kill/time-bonus callback. Do not grant rewards both in a custom death handler and again through the inherited destruction handler.

Useful inherited defaults are:

| Field | Meaning in the inspected graphs |
| --- | --- |
| `TimeBonusPerKill` | Added to the battle timer through the game mode's kill event. |
| `SoulMIN`, `SoulMAX` | Bounds of the random number of `BP_SoulItem` actors spawned on destruction. Each soul's collection path calls `Add Souls` with 1. |
| `PotionDropAmount` | Weight for `Random Bool with Weight`; despite its name, it is a probability, not a quantity. |
| `WeaponDropRate` | Weight for a weapon-drop roll made only if the potion roll failed. |
| `HPBarShowPercent` | Health-ratio threshold used by the shared health-bar visibility logic. |

Drop weights use 0–1 values. Level 01/02 use potion 0.1 and weapon 0.05; Level 03 uses 0.125 and 0.09. Because the weapon roll follows the failed potion branch, those are not two independent chances: the total weapon-drop probability is `(1 - PotionDropAmount) × WeaponDropRate` for valid weights. The existing pickup is `/Game/Items/Pickups/WeaponPickups/BP_Weapon_Pickup`, rather than the enemy's `Weapon` actor being converted automatically into an inventory item. See [Adding items](ADDING_ITEMS.md) for item/pickup authoring.

One inherited detail to understand before extending loot: `SpawnLoot` has three potion choices—Health, Mana, and ManaHealth—but its selection uses **Random Integer with Max 2**, which produces 0 or 1. The combined potion is therefore not selected by this graph as currently wired. This guide records that behavior; it does not change the sample's loot balance. If you deliberately extend the selection, test the new branch and treat it as a gameplay change.

## Add the enemy to waves

The wave implementation lives in `BP_GameMode`'s **EnemyManager** graph. `BP_GameState` is not the wave-data registration point in this sample.

`/Game/Blueprints/Progression/WavesProgression` is a Data Table using `WavesStruct`. Its editable fields are:

```text
Wave_N row
  SpawnGroup       array of SpawnGroupStruct
    Enemies        array of enemy Blueprint classes
  WaveTime         integer seconds
```

There is no separate Count field. Repeat an enemy class in `Enemies` for multiple instances. Groups are the sequential encounters inside a wave. The supplied table has `Wave_1` through `Wave_5`, each with `WaveTime` 60. Wave 1 contains groups of one, two, and two Level 01 goblins. Wave 4's last group includes the spider boss.

### Smallest reproducible wave test

1. Save/commit your working baseline so the temporary test is easy to revert.
2. Open `WavesProgression`, select `Wave_1`, expand `SpawnGroup` element 0, then its `Enemies` array.
3. Replace its existing Level 01 class with your new enemy class. Do not leave an empty/None element. Keep the other groups and row name intact for this first test.
4. Save the table and new class. Start the actual sample game, enter the arena, and identify the first live actor's class in the World Outliner/debugger. The new asset's existence or display name alone does not prove the spawner uses it.
5. Defeat it and wait for its actor to be destroyed. Verify the next group appears, the kill/time bonus occurs once, and souls/drops are collected normally.
6. Restore the temporary replacement or make the desired permanent group edit. Add more instances by adding class entries; add later encounters by adding SpawnGroup elements.

For a separate table, duplicate `WavesProgression` and explicitly change the **Get Data Table Row WavesProgression** node in `BP_GameMode.EnemyManager` to the new table. The original graph hard-references the original table; a second table is not discovered automatically. Keep the same row struct unless you also intentionally rewrite the consuming graph.

`StartNewWave` constructs the row name from `Wave_` plus `CurrentWave` (initially 1), then loads `SpawnGroup` into its runtime `Wave` array and assigns `WaveTime` to `BattleTime`. Keep row names contiguous: a missing row reaches `OnAllWavesFinished` and the game-over/completion path, rather than skipping to the next row. When adding Wave 6, use the exact row name `Wave_6`; changing a display label elsewhere does not satisfy that lookup.

`SpawnNextEnemiesGroup` reads the first remaining group's Enemies, spawns their classes, and removes that group from the runtime queue. `SpawnEnemy` increments `SpawnedEnemies` and binds `EnemyDestroyed` to each spawned actor's OnDestroyed delegate. That delegate decrements the count and invokes `CheckCurrentWave`; when the active count reaches zero, the next group or wave is selected. There is no need to manually call both a death counter and a destruction counter from your new enemy.

### Spawn locations and failure cases

`NPC_SpawnBox` is a location marker, not the enemy list. `GetRandomSpawnPoint` collects actors of that class and returns a randomly chosen marker's actor transform. Place markers in the loaded playable world, clear of obstructions and above reachable navigation. Their orientation and scale are part of the selected transform; keep scale intentional. `StartEnemySpawn` only starts the wave sequence if a spawn marker is available.

The current `SpawnEnemy` node uses **Adjust If Possible But Always Spawn**. That does not make an obstructed position navigable. It also increments `SpawnedEnemies` without a separate validity branch before binding OnDestroyed. Avoid None classes and invalid spawn configurations: a failed spawn can leave a counter with no actor to decrement it. An empty first group similarly supplies no destruction event to advance the original flow. Keep each authored group nonempty and verify every class entry.

A hand-placed enemy can test combat, but it is not automatically registered in this runtime wave count. `GetNumberOfEnemies` separately counts actors of `BP_EnemyCharacter` (including subclasses); it is not the `SpawnedEnemies` bookkeeping used above. For a full encounter test, use the actual wave-spawn path.

## Troubleshooting a duplicated enemy

Do not assume duplication itself is broken. A duplicate can compile while still referencing the old attack, missing a spawn-table entry, or overriding an inherited initialization event. Follow the first failing stage:

| Symptom | Check in this sample |
| --- | --- |
| The old enemy still spawns | Inspect `WavesProgression.SpawnGroup[].Enemies[]`, the game mode's table node, and the actual spawned class. Duplicating a class does not replace references to the original. |
| Pawn appears but has 1 HP or no attacks | Confirm live possession, `AIControllerClass`, `AutoPossessAI`, `PassiveGameplayEffects`, and Weapon slot 0. Native startup grants happen at possession. |
| Enemy is not selected as a target | Retain Actor tag `Enemy` and the `BP_Character`/`BP_EnemyCharacter` ancestry expected by service/task casts. A GAS tag does not substitute for the Actor tag. |
| Enemy follows but never attacks | Match ranged/melee controller behavior with the configured slot/ability; inspect `CanUseAnyAbility`, montage state, and cooldown tags. |
| Attack animation plays without damage | Check `CurrentWeapon`, Instigator, `b_MF_Weapon_R`, collision/hit timing, gameplay event tag/payload, effect container, and target GAS attributes. |
| Animation stays in reference pose or attacks fail | Check mesh skeleton, animation Blueprint, montage skeleton, DefaultSlot routing, and inherited hit-reaction references. |
| Enemy dies but the wave stops | Check ShowDeath/health transition, `Death_Notify`, `DelayedDestroy`, actual actor destruction, and the wave manager's OnDestroyed binding. Health zero alone does not decrement `SpawnedEnemies`. |
| Rewards or time bonus disappear/double | Preserve parent death/destruction callbacks and Do Once gates; avoid manually duplicating their reward logic. |
| Enemy works in PIE but is absent from the package | Save assets, confirm cooked references to the class/dependencies, then test a fresh Development package and inspect its log. |

### Class comparisons are not faction comparisons

The shared `/Game/Blueprints/Weapon/WeaponActor` overlap graph requires `IsAttacking` and compares `OtherActor.GetClass()` with `GetInstigator().GetClass()` using **Not Equal (Class)** before proceeding. This is exact class comparison. A duplicated enemy Blueprint or child variant has a different class even if it has the same skeleton, parent, or team. The graph is not a generic friendly-fire/team filter.

Keep this inherited behavior in mind when testing mixed enemy variants in close proximity. Do not claim that the native team IDs prevent every friendly hit. If your extension needs explicit hostility rules, implement and test them consistently in the attack targeting path as a separate gameplay change. Also distinguish `Cast To BP_Character`/class-family checks from exact-class equality when auditing new Blueprint logic; replacing a cast with equality can exclude otherwise valid child enemies.

`ActionRPG.Port.GoblinVariantInitialization` creates temporary child and duplicate Blueprints from Level 01, spawns and explicitly possesses them, and verifies inherited stats, melee ability, Enemy tag, and increased health after raising the larger variants to level 3 with `SetCharacterLevel`. These assets are transient and are not shipped as new enemies. The test passes on UE 5.8.4; it does not exercise automatic possession, AI navigation, animation damage, death, or wave progression.

There is no diagnosed universal cause for every “my duplicate enemy does not work” report. Compare a fresh, unchanged child of Level 01 with your edited enemy in the same map and spawn group, then reintroduce differences one at a time. This separates shared sample behavior from a missing dependency or a new override.

## Cooking and packaged verification

Enemy Blueprints are ordinary referenced content here, not a separately registered enemy primary-asset type. Keep a valid serialized reference from a cooked map, wave table, or another reachable asset to the new enemy class and its dependencies. [DefaultGame.ini](../Config/DefaultGame.ini) sets the sample's entry maps and item primary-asset scans. An unused asset existing in the Content Browser is not sufficient proof that it will ship.

Use Reference Viewer to inspect references in both directions. For content selected only through your own strings or otherwise undiscoverable runtime lookup, add an explicit cook inclusion strategy and verify it in a package; do not rely on editor memory hiding a missing reference. Save all changed Blueprints, effects, montages, tables, and maps before packaging.

Use a Windows Development package for the final test. Check its log for missing packages, invalid ability grants, montage playback failures, and Blueprint `Accessed None` errors. Test from a new launch, not just a PIE session that has already loaded your working assets.

## Acceptance checklist for a new enemy

- [ ] Blueprint compiles and the intended class is spawned from the wave table.
- [ ] Controller, blackboard, behavior tree, Actor tags, and navigation are correct on the live instance.
- [ ] Health/max health, level scaling, damage, defense, and movement match the intended values.
- [ ] Melee/projectile damage occurs at the intended animation time and does not duplicate hits unexpectedly.
- [ ] Ranged cooldown prevents immediate repeated attacks and expires normally.
- [ ] Hit reactions, attack interruption, and death work with the chosen skeleton.
- [ ] Death awards the expected timer/kill change once, cleans up the weapon, and eventually destroys the pawn.
- [ ] Souls and intended pickup branches work; the next spawn group and next wave advance.
- [ ] Mixed variants are tested together for the sample's exact-class overlap filtering behavior.
- [ ] The same test passes in a freshly launched Windows Development package.

The field names and graph descriptions in this guide were checked against the port's UE 5.8.4 reflected defaults and Blueprint graphs. New enemy variants described here are authoring examples, not additional enemies shipped or gameplay-tested as part of the baseline port.
