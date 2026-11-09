# Adding items in ActionRPG — UE 5.9 Preview

This guide applies to the `preview/ue5.9` branch. It retains the item/GAS architecture documented for the 5.8.4 baseline. The 5.9 build and packaged inventory, shop, ability, and save checks are recorded in [preview validation](UE59_PREVIEW.md); individual new-item recipes still require their acceptance checks.

This guide extends the sample's existing inventory and Gameplay Ability System (GAS). Make changes in a separate branch or project copy, keep the supplied assets as working references, and test both a fresh save and an existing save. Adding a weapon appearance, a new attack, and a new item sold in the shop are separate changes; start with one working variation before changing all three.

Paths beginning with `/Game/` are Content Browser paths. For example, `/Game/Items/Weapons/Weapon_Axe` is the repository file `Content/Items/Weapons/Weapon_Axe.uasset`. The editor displays native property names with spaces, so `GrantedAbility` appears as **Granted Ability**.

## 1. Understand what an item is

An inventory item is an instance of a native **Data Asset** class derived from `URPGItem`/`UPrimaryDataAsset`. It is not an Actor Blueprint or a Gameplay Ability Blueprint. Although some derived C++ class comments say “should be blueprinted,” the sample's item discovery explicitly uses `bHasBlueprintClasses=False`, and `GetPrimaryAssetId()` assumes a Data Asset object.

| Item kind | Data Asset class | Scanned folder | Existing reference |
| --- | --- | --- | --- |
| Weapon | `RPGWeaponItem` | `/Game/Items/Weapons` | `Weapon_Axe`, `Weapon_Hammer_1`, `Weapon_Sword_2` |
| Skill | `RPGSkillItem` | `/Game/Items/Skills` | `Skill_FireBall`, `Skill_FireWave` |
| Potion | `RPGPotionItem` | `/Game/Items/Potions` | `Potion_Health`, `Potion_Mana`, `Potion_DeathsDoor` |
| Token/currency | `RPGTokenItem` | `/Game/Items/Tokens` | `Token_Souls` |

A weapon Data Asset has an additional `WeaponActor` class reference. That points to the Blueprint actor spawned for the visible weapon and collision behavior. Its `GrantedAbility` points to the Gameplay Ability class used when that inventory item is slotted. A world pickup is another actor, responsible for awarding an item; it is not the inventory Data Asset.

Relevant source: [item fields](../Source/ActionRPG/Public/Items/RPGItem.h), [weapon field](../Source/ActionRPG/Public/Items/RPGWeaponItem.h), [item ID implementation](../Source/ActionRPG/Private/Items/RPGItem.cpp), and [Asset Manager scan settings](../Config/DefaultGame.ini).

## 2. Create a discoverable item Data Asset

1. In the Content Browser, open the scanned folder for the intended item type.
2. Duplicate the closest existing item. This preserves its native Data Asset class and useful defaults. Alternatively, create a Data Asset and choose the appropriate native class from the table above; do not create a Blueprint subclass of `RPGItem`.
3. Give the asset a unique name within its type, such as `Weapon_TrainingAxe`. Avoid giving two weapon Data Assets the same asset name in different subfolders: both would try to use the same primary ID.
4. Set the following fields, then save the asset.

| Field | Meaning and important limits |
| --- | --- |
| `ItemType` | Set by the native subclass (`Weapon`, `Skill`, `Potion`, or `Token`). It is not a substitute for selecting the correct class. |
| `ItemName` / `ItemDescription` | Player-facing text; changing these does not rename the asset's persistent ID. |
| `ItemIcon` | A Slate Brush used by the item UI. Assign the appropriate icon resource and check its appearance in the actual inventory/shop. |
| `Price` | Price data read by purchase logic. Assigning it does not itself charge currency when `AddInventoryItem` is called. |
| `MaxCount` | Maximum owned count; zero or negative means unlimited stacking. The sample also defines `IsConsumable()` as **`MaxCount <= 0`**, rather than checking the Potion class. |
| `MaxLevel` | Clamp for the stored inventory `ItemLevel`; zero or negative means no configured upper limit. |
| `GrantedAbility` | Gameplay Ability class granted through a slot. An item can exist in inventory without an ability; it then provides no item ability override. |
| `AbilityLevel` | In this sample's actual grant code, used directly for **Weapon items only**. Set a positive value for weapons. See the level explanation below. |
| `WeaponActor` | Weapon items only: the actor class to use for the equipped visual/collision representation. |

For a stackable potion, follow the existing potion's `MaxCount` convention. Setting a positive cap changes `IsConsumable()` to false; it is not merely a stack-size adjustment. If you want “consumable with a finite stack cap,” change the underlying item/consumption design deliberately rather than assuming the current flag supports it.

### Discovery and cooking

The project uses `RPGAssetManager`, configured in [DefaultEngine.ini](../Config/DefaultEngine.ini). [DefaultGame.ini](../Config/DefaultGame.ini) scans the four item folders above using the corresponding native classes, `bHasBlueprintClasses=False`, and `CookRule=AlwaysCook`.

Keep new items in those folders or add the intended directory to that type's Asset Manager scan settings. Merely placing a Data Asset somewhere under `/Game` does not ensure it is registered as an item. Save it, confirm the expected primary asset ID is discoverable, and test a cooked package. The item ID is `Type:AssetName`, for example `Weapon:Weapon_TrainingAxe`; it is not the object's `/Game/...` path and does not end in `_C`.

`AlwaysCook` is a rule on the scanned item assets. Keep required ability, effect, actor, montage, mesh, material, icon, and sound references intact, and verify the cooked result. A working editor reference is not proof that all runtime dependencies are packaged.

## 3. Inventory, slots, and ability levels

The player controller owns `InventoryData`, a map from item Data Asset to `FRPGItemData` (`ItemCount`, `ItemLevel`), and `SlottedItems`, a map from `FRPGItemSlot` to item. A slot is an item type plus a **zero-based** slot number. Several copies of one item share one count and one stored level; this is not a per-instance equipment system with separate rolls or durability.

Use [the controller's public inventory functions](../Source/ActionRPG/Public/RPGPlayerControllerBase.h) when adding custom rewards or UI:

- `AddInventoryItem(Item, ItemCount, ItemLevel, bAutoSlot)` requires a non-null item and positive count/level. It adds/clamps count, replaces/clamps the stored level, optionally fills the lowest available matching slot, emits notifications, and saves if anything changed. Check its return value.
- `GetInventoryItemCount` and `GetInventoryItemData` query ownership without creating inventory entries.
- `SetSlottedItem` changes an existing slot, removes that same item from another slot, notifies listeners, and saves. Custom callers must validate ownership and matching item type: the native function itself does not enforce those two checks.
- `RemoveInventoryItem(Item, Count)` decrements count. A nonpositive removal count means remove all. Removing the last copy also clears every slot containing that item.

Do not edit the controller maps directly during gameplay: that bypasses the normal notifications, ability refresh, and persistence. Adding an item is not the same as equipping it. If `bAutoSlot` is true and all matching slots are occupied, the item remains owned but does not displace the equipped item.

### Three different levels

1. **`FRPGItemData.ItemLevel`** is stored per owned item and constrained by `MaxLevel` when adding/updating it.
2. **The item's `AbilityLevel` field** is read directly when granting a slotted **Weapon** ability. The implementation does not implement the header comment's “<= 0 means character level” fallback for weapons, so use a positive value unless you intentionally change the code.
3. **`CharacterLevel`** is used for slotted Skills and Potions, and for default slotted abilities. Editing a Skill/Potion item's `AbilityLevel` or stored `ItemLevel` does not change its granted GAS spec level in the current implementation.

See `FillSlottedAbilitySpecs` in [RPGCharacterBase.cpp](../Source/ActionRPG/Private/RPGCharacterBase.cpp). It first adds `DefaultSlottedAbilities`, then overrides slots whose inventory item has a `GrantedAbility`. The granted ability's source object is the item Data Asset. Inventory slot changes refresh the character's abilities; changing a value on an already granted asset is best tested after restarting play/regranting, because refresh compares ability class/source identity rather than treating every edited value as a new spec.

## 4. Defaults and persistent saves

The configured game instance is `/Game/Blueprints/BP_GameInstance`. Its native parent exposes `DefaultInventory` and `ItemSlotsPerType`; see [RPGGameInstanceBase.h](../Source/ActionRPG/Public/RPGGameInstanceBase.h).

To give a test item by default:

1. Open `BP_GameInstance` and its Class Defaults.
2. Add the exact primary asset ID to `DefaultInventory`, with a positive `ItemCount` and `ItemLevel`.
3. Check that `ItemSlotsPerType` has the required item type and enough slots. Slots are created from zero through count minus one. Increasing this count does not automatically build more HUD controls or bind inputs for them.
4. Start play through the normal game initialization, confirm ownership, then explicitly equip it if a matching slot is already occupied.

`AddDefaultInventory` runs for **existing saves as well as new saves**. It adds missing IDs but does not overwrite a saved entry's count or level. Consequently, increasing a default count does not top up an existing saved stack, while an entirely depleted/removed default item can be restored on the next load. Do not use this merge as a one-time reward system without accounting for that behavior.

The save stores inventory and slots by primary asset IDs, not live object pointers. `LoadInventory` resolves those IDs through `RPGAssetManager::ForceLoadItem`; unresolved IDs cannot restore the item. Keep released asset names/types stable. Renaming an item changes its ID; an editor object-path redirector alone is not a demonstrated migration for saved `Type:Name` keys. Implement and test an explicit save-ID migration or appropriate Asset Manager ID redirects before renaming released items. Moving an asset while preserving name/type can preserve its ID, but it must remain in a scanned directory and references still need checking.

Back up saves before testing. Use a separate `SaveSlot`/`SaveUserIndex` or the game's intentional reset flow for a fresh-save test; do not delete the only copy of a player's data. `ResetSaveGame` erases the in-memory state and is persisted by a later write. Save writes are asynchronous, so allow completion before closing the game. The existing [save serializer](../Source/ActionRPG/Private/RPGSaveGame.cpp) migrates an older item-list format; it does not provide arbitrary item-renaming migration.

## 5. How the item's gameplay reaches a target

The item's `GrantedAbility` references a Blueprint derived from `RPGGameplayAbility`. The item does not itself apply damage, play an animation, spend mana, or consume an inventory count. Preserve the relevant shared ability parent and its execution flow when duplicating an existing ability.

### Effect containers and targeting

`RPGGameplayAbility` exposes `EffectContainerMap`: a gameplay tag maps to a target-type class and one or more Gameplay Effect classes. `MakeEffectContainerSpec` looks up that **exact key**, creates outgoing effects at the ability's spec level unless explicitly overridden, and asks the target type to populate target data. `ApplyEffectContainerSpec` applies those effects to those targets. A valid effect with no valid targets still has nothing useful to affect.

The native target helpers are:

- `RPGTargetType_UseOwner`: returns the owning character, suitable for a self-targeted effect.
- `RPGTargetType_UseEventData`: uses a hit result from the event context, otherwise the event's `Target` actor. Sending only a tag with no suitable event payload does not provide a hit target.
- Blueprint target types can implement their own targeting, as the supplied melee/area skill examples do.

See [effect container types](../Source/ActionRPG/Public/Abilities/RPGAbilityTypes.h), [container creation/application](../Source/ActionRPG/Private/Abilities/RPGGameplayAbility.cpp), and [target helpers](../Source/ActionRPG/Private/Abilities/RPGTargetType.cpp).

### Montages, notifies, and event tags

The sample's `PlayMontageAndWaitForEvent` ability task combines montage playback and gameplay-event listening. It provides `EventReceived`, `OnCompleted`, `OnBlendOut`, `OnInterrupted`, and `OnCancelled`. Preserve the existing ability's success and cancellation/end paths; an animation finishing visually is not sufficient if the ability never ends.

When changing an attack or item montage, check all of these together:

1. The montage uses a compatible character skeleton and an animation slot the character's Anim Blueprint actually evaluates.
2. The ability plays the intended montage and listens for the intended event tags.
3. Required notify/notify-state windows remain at the intended hit/use frames, including the sender's gameplay-event path and target payload.
4. The received tag selects the matching `EffectContainerMap` key or the ability's intended action.
5. Interrupting, cancelling, rolling, or unequipping cannot leave collision, active effects, or the ability running indefinitely.

Existing tags in [DefaultGameplayTags.ini](../Config/DefaultGameplayTags.ini) include `Event.Montage.Shared.WeaponHit`, `Event.Montage.Shared.UseItem`, `Event.Montage.Shared.UseSkill`, and the player combo event tags. Register any new tags through the project's Gameplay Tags settings/config and update both sender and receiver. Renaming only one side breaks the chain.

Task behavior is defined in [RPGAbilityTask_PlayMontageAndWaitForEvent.h](../Source/ActionRPG/Public/Abilities/RPGAbilityTask_PlayMontageAndWaitForEvent.h). A failed montage play calls its cancelled path; use the log to distinguish that from a hit or targeting failure.

### Damage, mana cost, and cooldown

For effects that use `RPGDamageExecution`, the native calculation is:

```text
Final damage = captured base Damage × source AttackPower ÷ target DefensePower
```

Exactly zero DefensePower is treated as 1. Source AttackPower and Damage are snapshotted when the effect spec is made; target DefensePower is evaluated at application. Positive resulting Damage is handled as a temporary attribute, cleared, and subtracted from Health with clamping. The character's damage/health callbacks then receive the result. Editing an item description or a weapon mesh does not change these numbers.

Inspect the actual damage effect's modifiers/execution settings, referenced curve table and row, and level scaling. Check both attack and defense attributes before assuming that a visually successful hit must remove health. See [RPGDamageExecution.cpp](../Source/ActionRPG/Private/Abilities/RPGDamageExecution.cpp) and [RPGAttributeSet.cpp](../Source/ActionRPG/Private/Abilities/RPGAttributeSet.cpp).

For a mana skill, preserve and review its **Cost Gameplay Effect Class**, **Cooldown Gameplay Effect Class**, activation requirements, and commit path. A mana-cost effect should modify the intended Mana attribute; a cooldown needs a finite duration and the intended tag **granted to the owning ASC**. An effect's asset classification tag alone does not establish that granted cooldown state. In this 5.9 Preview port, use the target-tag Gameplay Effect component (`UTargetTagsGameplayEffectComponent`) for granted tags, following the migrated working sample effects.

`GetCooldownRemainingForTag` queries active effect owning tags and reports the longest matching remaining duration. `Cooldown.Skill` is an existing registered tag. If several abilities use the same granted cooldown tag, test whether they intentionally share a cooldown. For independent cooldowns, introduce the appropriate distinct registered tags and update the effect, ability checks, and UI query together. Test activation with insufficient mana, during cooldown, at the end of cooldown, and after an interruption; verify costs and damage occur once at the intended moment.

## 6. Add a weapon

### A. Create an appearance variation using an existing attack

The supplied axe pair demonstrates this separation:

| Data Asset | `WeaponActor` | `GrantedAbility` | `AbilityLevel` |
| --- | --- | --- | --- |
| `/Game/Items/Weapons/Weapon_Axe` | `/Game/Blueprints/Weapon/GreateBladeActor` | `/Game/Abilities/Player/Axe/GA_PlayerAxeMelee` | 1 |
| `/Game/Items/Weapons/Weapon_Axe_2` | `/Game/Blueprints/Weapon/FireAxeActor` | The same axe ability | 2 |

`GreateBladeActor` is the asset's actual spelling. The two items can reuse the attack implementation while selecting different actor visuals and ability-spec levels.

1. Duplicate `Weapon_Axe` under `/Game/Items/Weapons`, for example as `Weapon_TrainingAxe`.
2. Duplicate `GreateBladeActor` under `/Game/Blueprints/Weapon` as your new actor Blueprint. Keep its `WeaponActor` inheritance.
3. In the actor Blueprint, replace the visible mesh/materials and adjust its transform. Preserve the inherited collision and attack-event machinery. Check the collision shape against the new mesh rather than assuming a larger mesh has a larger hit region.
4. Set the new item Data Asset's `WeaponActor` to that actor class and keep `GrantedAbility = GA_PlayerAxeMelee` initially. Set `AbilityLevel` to a positive supported level, and update the name, description, icon, and positive shop price.
5. Add the item through a test reward/default inventory or buy it from the normal store. Equip it in a Weapon slot through the existing inventory UI, then switch to that weapon.
6. Confirm attachment, held orientation, collision timing, regular hits, combos, weapon switching, and unequipping in play.

`BP_PlayerCharacter`'s `CreateAllWeapons` reads the slotted weapon items, casts their actor classes to `WeaponActor` classes, and spawns them. `WeaponAttachMethod` attaches the current weapon to the character mesh's `hand_rSocket`. Retain a compatible socket and adjust the actor/mesh offset as needed. A generic Actor class may satisfy the Data Asset property's C++ type but fail this Blueprint cast; derive from the sample's weapon actor base.

### B. Give the weapon a different attack or damage profile

1. Duplicate `/Game/Abilities/Player/Axe/GA_PlayerAxeMelee` as your ability. Its parent is `/Game/Abilities/Shared/GA_MeleeBase`; retain the parent activation/end calls unless you are deliberately replacing their behavior.
2. In Class Defaults, inspect `MontageToPlay`. The reference uses `/Game/Characters/Animations/AM_Attack_Axe`. Reuse it first; duplicate the montage before changing the original attack's timings, sections, or notifies.
3. Review `EffectContainerMap`. The reference has separate entries for `Event.Montage.Shared.WeaponHit`, `Event.Montage.Player.Combo.BurstPound`, and `Event.Montage.Player.Combo.GroundPound`. Changing only the regular-hit effect leaves the combo effects unchanged.
4. Duplicate the Gameplay Effects you want to change and redirect the relevant container entries. The ordinary axe hit uses `RPGTargetType_UseEventData` and `/Game/Abilities/Player/Axe/GE_PlayerAxeMelee`.
5. The ordinary axe effect uses the `RPGDamageExecution` calculation and its captured Damage modifier reads the `DefaultAttack` row of `/Game/Abilities/DataTables/AttackDamage`. Adjust the intended scalable magnitude/curve reference, keeping its level behavior and damage-immunity filtering intentional. Duplicate a shared curve table before editing it if other weapons must keep their original damage.
6. Assign the new ability to the new weapon item's `GrantedAbility`, save and compile the changed Blueprints, then repeat the equip and combat checks.

The reference montage contains `WeaponAttackNS` windows. That notify state starts/ends attacks on the character's `CurrentWeapon`; the weapon actor enables/disables its `CapsuleCollision` and sends its configured attack event on valid overlaps. `GA_MeleeBase` commits the ability, plays the montage, and applies the effect container selected by each received event. Removing the notify, breaking the overlap setup, or changing only the event receiver can leave an animated swing with no damage. Preserve and test the multi-hit delay/count controls as well as the regular hit and combo paths.

## 7. Add a skill

For a projectile skill, use this complete working chain:

```text
/Game/Items/Skills/Skill_FireBall
  GrantedAbility -> /Game/Abilities/Player/Skills/GA_PlayerSkillFireball
  parent         -> /Game/Abilities/Shared/GA_SpawnProjectileBase
  MontageToPlay  -> /Game/Characters/Animations/AM_Skill_Fireball
  ProjectileClass -> /Game/Abilities/Player/Skills/BP_Fireball
  UseSkill container -> GE_PlayerSkillFireball
  cost           -> GE_PlayerSkillManaCost
  cooldown       -> GE_PlayerSkillCooldown
```

1. Duplicate `Skill_FireBall` in `/Game/Items/Skills`, then duplicate its Gameplay Ability into your own skill folder.
2. Point the new item's `GrantedAbility` to the new ability. The reference price is 50, `MaxCount` is 1, and `MaxLevel` is 1; choose suitable values without confusing item level with GAS spec level.
3. Retain the projectile ability parent. Duplicate `BP_Fireball` if changing projectile appearance, collision, speed, or impact behavior; set the ability's `ProjectileClass` to your new class.
4. Duplicate `GE_PlayerSkillFireball` if changing damage and select it in the `Event.Montage.Shared.UseSkill` effect container. Its reference captured Damage modifier is `1.5 × AttackDamage` curve row `HeavyAttack`, evaluated at the ability level before the attack/defense calculation.
5. Reuse or duplicate the montage and preserve the `RangeAttackNS` notify state's `Event.Montage.Shared.UseSkill` event. The projectile parent creates the effect spec at the event and passes it to the spawned projectile. `BP_AbilityProjectileBase` adds hit targets and applies the external effect container spec on impact. Do not add a second direct damage application to the same hit unless that is intentional.
6. Review cost/cooldown effects. The supplied mana cost is Instant with a Mana modifier of -10; the supplied cooldown lasts 2 seconds and grants `Cooldown.Skill`. Duplicate either effect before tuning it for only the new skill, then update the new ability's cost/cooldown class references.
7. Buy or grant the item, equip it in the Skill slot, and test with enough mana, insufficient mana, an enemy target, a wall, a miss, repeated input during cooldown, and after cooldown expires.

For a non-projectile area skill, start from `/Game/Items/Skills/Skill_FireWave` and `/Game/Abilities/Player/Skills/GA_PlayerSkillFireWave`, whose parent is `GA_SkillBase`. Preserve its target-type/container path rather than transplanting the projectile graph incompletely. The existing `Skill_Combust` item actually grants `GA_PlayerSkillMeteor`; inspect the references rather than inferring class names from the player's display name.

## 8. Add a potion

The health potion's reference chain is:

```text
Potion_Health (Data Asset, Price=25, MaxCount=0, MaxLevel=1)
  -> /Game/Abilities/Player/Potions/GA_PotionHealth
  -> parent /Game/Abilities/Shared/GA_PotionBase
  -> /Game/Characters/Animations/AM_Item_Potion
  -> UseItemNS: Event.Montage.Shared.UseItem
  -> owner-targeted /Game/Abilities/Player/Potions/GE_PotionHealth
```

1. Duplicate `Potion_Health` in `/Game/Items/Potions`, and give it a unique name, icon, description, and positive price. Retain the stack/consumable convention `MaxCount=0` for this workflow.
2. Duplicate `GA_PotionHealth`, keep its `GA_PotionBase` parent and parent calls, and assign it to the new item's `GrantedAbility`.
3. Duplicate `GE_PotionHealth` and select it in the ability's `Event.Montage.Shared.UseItem` container, retaining `RPGTargetType_UseOwner` for a self-heal.
4. Tune the effect deliberately. The original is an Instant Health modifier using `0.5 × StartingStats` curve row `PlayerMaxHealth`. It is not a dynamic “read my current MaxHealth and divide by two” calculation. If you introduce max-health buffs and require exactly half the buffed maximum, implement that different magnitude calculation explicitly.
5. Keep the item montage's `UseItemNS` event at the actual drink/use frame. Change the montage only after verifying the original flow. Use the supplied mana-potion chain (`Potion_Mana`, `GA_PotionMana`, `GE_PotionMana`, `AM_Item_Potion_Mana`) for a mana restorative instead.
6. Obtain at least two copies, equip the new potion in Potion slot 0, reduce the relevant resource, and use it. Confirm one effect and one count reduction at the intended moment. Use the last copy and verify its slot clears. Also interrupt before the use frame and test at full health/mana to understand the retained sample behavior.

In the inspected `GA_PotionBase` graph, `EventReceived` applies the event's effect container, then removes **one item from Potion slot 0** through the owning player controller. It does not select the consumable from the ability's source object. The listener accepts `Event.Montage` events broadly, so extra received events in a custom potion montage can also run the consumption path. Keep a single intended use event, or change/filter the logic explicitly. Adding a second potion slot requires adapting this hard-coded slot and the input/UI paths, not just increasing `ItemSlotsPerType`.

## 9. Add a token or currency

`/Game/Items/Tokens/Token_Souls` is an `RPGTokenItem`, with `MaxCount=0`, `Price=0`, and no granted ability. Its unlimited stack also makes `IsConsumable()` return true; that does not automatically consume a token or give it an activation ability.

1. Duplicate `Token_Souls` in `/Game/Items/Tokens` and assign a unique name and display fields.
2. Award it through `AddInventoryItem` and spend it through checked inventory-removal logic. Verify positive quantities, adequate balance, the last-token case, and save/reload.
3. Decide whether it is an independent collectible or replaces the shop's currency. Merely creating it does not make the shop accept it.
4. For a currency replacement, inspect `BP_PlayerController`'s `SoulsItem` reference and its `AddSouls`, `ConsumeSouls`, `CanPurchaseItem`, `PurchaseItem`, and inventory-change/HUD paths. Update reward producers and balance displays consistently. A new token with no such integration remains a separate inventory entry.

The default `ItemSlotsPerType` contains Potion, Skill, and Weapon only. Tokens are therefore not part of the store's normal type enumeration. Do not add a fake equip slot just to expose a currency: add the intended listing/reward/UI behavior explicitly if that is your design.

## 10. Shop listing, purchasing, and world pickups

### Automatic store listing

There is no per-item shop list you must manually append to for a normal new Weapon, Skill, or Potion. The inspected `BP_GameInstance` `InitializeStoreItems` event takes the keys of `ItemSlotsPerType`, requests each type's primary asset ID list, asynchronously loads the assets, casts them to `RPGItem`, and collects `StoreItems`. `GetStoreItems` filters that collection by type. The supplied defaults are **Potion: 1, Skill: 1, Weapon: 3**, with default inventory `Weapon:Weapon_Axe` (count/level 1).

1. Save the new correctly typed Data Asset inside its scanned directory.
2. Restart the normal game initialization so `InitializeStoreItems` runs again; an already initialized store cache need not discover assets added during play.
3. Open the matching inventory/equipment category. `/Game/Blueprints/WidgetBP/Inventory/WB_InventoryList` builds the owned and store lists; it calls `GetStoreItems` and creates `WB_PurchaseItem` entries.
4. Select the new entry and verify its name, icon, description, price, quantity behavior, affordability, and owned/equipped state. An item already owned can be handled differently by the list; compare with a fresh-save test before assuming discovery failed.

`WB_PurchaseConfirm` checks `BP_PlayerController.CanPurchaseItem`, and its quantity selector uses `IsConsumable()`. The controller's affordability check compares Souls count with `Price × Quantity`. Its purchase flow consumes Souls and adds the item with `bAutoSlot=false`, so **purchasing does not automatically equip the item**. Use the existing equip UI afterwards.

Use positive prices for ordinary purchasable items in this sample. `ConsumeSouls` forwards the charge directly as `RemoveInventoryItem`'s removal count, whose nonpositive value means “remove all.” A free item therefore needs an explicit zero-charge path; simply setting `Price=0` is not a safe free-purchase implementation. Keep quantities positive as well. The inherited purchase flow also spends currency before the add result is known: if you extend it to new caps or bulk transactions, validate capacity and implement refund/transaction handling for failures rather than charging for an item the inventory cannot accept.

### Give an item through a world pickup

1. Duplicate the closest supplied pickup, such as `/Game/Items/Pickups/PotionPickups/BP_Pickup_Health`, retaining the `/Game/Items/Pickups/BP_RPGItem_Pickup_Base` parent.
2. Set its `ItemType` **object reference** to your new Data Asset and set a positive `Count`. Despite its name, this pickup property references the item object, not the `Weapon`/`Potion` primary type name.
3. Change its presentation, place the actor in a test map, and confirm the player overlap path runs once.
4. Verify owned count, the optional auto-slot result, UI update, pickup removal, and save persistence. The base pickup calls `AddInventoryItem` with `ItemLevel=1` and `bAutoSlot=true`.

The shipped `/Game/Items/Pickups/WeaponPickups/BP_Weapon_Pickup` is a specialized example: its `ItemType` points at `/Game/Items/Pickups/WeaponPickups/Assets/Weapon_PickUp`, and its `Count` is 10. It is not already a generic pickup for each `/Game/Items/Weapons` entry. For a new weapon pickup, deliberately assign the intended new inventory Data Asset and count rather than assuming its mesh/name determines the awarded item. The base pickup ignores the add function's return value; if you introduce capacity limits or conditional grants, handle failure explicitly before allowing the pickup to disappear.

## 11. Validate the complete item

Compile and save the new/modified Blueprints, then run the local validation from the project root with the editor closed:

```powershell
.\Scripts\Build-Sample.ps1 -Phase Validate -RunAutomation
.\Scripts\Build-Sample.ps1 -Phase Package
```

Validation compiles project Blueprints and runs the existing `ActionRPG.Port` group when requested. The existing cooldown/possession regression test is not an automatic test of a newly authored item. Use `-Phase All -RunAutomation` when you also need the asset-upgrade and production-lighting phases; a small item data change does not inherently need a complete lighting rebuild. See [the build workflow](../Scripts/Build-Sample.ps1) and [README](../README.md).

Check the following in both PIE and the Windows package:

- **Discovery:** item appears in the correct type list after normal initialization; no load/cook warnings for its dependencies.
- **Economy:** exact price/quantity charged, insufficient funds rejected, repeated/non-stackable purchases handled, capacity failures do not lose currency.
- **Inventory:** correct count, item can be equipped/unequipped/swapped, removal of the last copy clears its slot, and UI follows every change.
- **Ability:** expected montage, notify timing, target selection, regular/combo damage, mana cost, cooldown, cancellation, and end state.
- **Consumables:** one use yields one intended effect and one decrement; empty/full-resource/last-copy cases behave intentionally.
- **Persistence:** quit after save completion, relaunch, verify count/level/slots, and test both fresh defaults and a save created before the item was added.
- **Presentation:** attachment/socket, icon, localized display text, audio/VFX, and controller/keyboard interactions remain usable.

### Troubleshooting by symptom

| Symptom | Check first |
| --- | --- |
| Item never appears in the store | Native Data Asset class, scanned directory/type, saved asset, unique primary ID, `InitializeStoreItems` rerun, matching UI type, and whether the item is already owned. |
| Works in editor, absent in package | Asset Manager registration/cook rule, required dependencies, actual package from the latest cook, and load errors in packaged logs. |
| Owned but cannot activate | Item is actually slotted, type/index exists, `GrantedAbility` is valid, and the expected input activates that slot. Purchasing has auto-slot disabled. |
| Weapon is invisible or at the wrong transform | `WeaponActor` derives from `WeaponActor`, spawn/cast succeeds, mesh/materials are valid, and `hand_rSocket` plus actor offset are compatible. |
| Swing animates but does no damage | `WeaponAttackNS` window, current weapon/collision overlap, sent event tag and target payload, exact container key, target type, damage effect/curve level, AttackPower/DefensePower, and damage immunity. |
| Projectile spawns but does no damage | Valid effect-container spec passed into it, collision/target filtering and impact path in the projectile base, and target added before external spec application. |
| Wrong damage/healing at higher levels | Weapon uses item's `AbilityLevel`; Skill/Potion uses `CharacterLevel`; stored `ItemLevel` is separate. Check curve-table row and scalable multiplier. |
| Skill has no cooldown or the indicator disagrees | Cost/cooldown class assignment, commit path, finite duration, tag actually granted to the ASC, matching ability/UI query, and intentional shared versus unique tags. |
| Potion consumes twice or the wrong stack | Duplicate/broad montage events and `GA_PotionBase`'s explicit Potion slot 0 lookup. It does not consume from the source-object item. |
| Default edit seems ignored, or a consumed default returns | Existing-save merge adds missing IDs without replacing present counts/levels. Test separately from a clean-save/reset flow. |
| Renamed item is missing after load | Saved primary ID changed or no longer resolves in the Asset Manager. Add/test migration rather than relying on an object-path redirector. |

This walkthrough's Blueprint names, references, defaults, and graph entry points were checked against this port's read-only content inspection. For deeper investigation, the editor-only `RPGInspectContent` commandlet can regenerate `Saved/DocsInspection.txt`; that diagnostic file is generated locally and is not required to use the guide.

