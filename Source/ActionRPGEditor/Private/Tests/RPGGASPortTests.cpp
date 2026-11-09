#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameplayEffect.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/Engine.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "RPGCharacterBase.h"
#include "Tests/AutomationEditorCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGGASPortTest, "ActionRPG.Port.GASCooldownAndPossession",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGGASPortTest::RunTest(const FString& Parameters)
{
    UWorld* World = FAutomationEditorCommonUtils::CreateNewMap();
    if (!TestNotNull(TEXT("Test world"), World)) return false;

    ARPGCharacterBase* Character = World->SpawnActor<ARPGCharacterBase>();
    APlayerController* Controller = World->SpawnActor<APlayerController>();
    if (!TestNotNull(TEXT("Character"), Character) || !TestNotNull(TEXT("Controller"), Controller)) return false;

    Controller->Possess(Character);
    UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent();
    if (!TestNotNull(TEXT("Character ASC"), ASC)) return false;
    TestTrue(TEXT("ASC avatar is the character"), ASC->GetAvatarActor() == Character);
    TestTrue(TEXT("ASC resolves the possessing controller"), ASC->AbilityActorInfo->PlayerController.Get() == Controller);

    UClass* CooldownClass = LoadClass<UGameplayEffect>(nullptr,
        TEXT("/Game/Abilities/Enemies/Goblin/GE_GoblinRange.GE_GoblinRange_C"));
    if (!TestNotNull(TEXT("Migrated goblin cooldown class"), CooldownClass)) return false;
    const UGameplayEffect* Cooldown = CooldownClass->GetDefaultObject<UGameplayEffect>();
    const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(TEXT("Cooldown.Goblin.Ranged"));
    TestTrue(TEXT("Cooldown has a finite duration"), Cooldown->DurationPolicy == EGameplayEffectDurationType::HasDuration);
    TestTrue(TEXT("Saved cooldown grants the expected tag"), Cooldown->GetGrantedTags().HasTagExact(Tag));

    FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectToSelf(Cooldown, 1.f, ASC->MakeEffectContext());
    TestTrue(TEXT("Cooldown applies as an active effect"), Handle.IsValid());
    TestTrue(TEXT("Applied cooldown tag is present on ASC"), ASC->HasMatchingGameplayTag(Tag));
    ASC->RemoveActiveGameplayEffect(Handle);
    TestFalse(TEXT("Removing cooldown removes its tag"), ASC->HasMatchingGameplayTag(Tag));

    Controller->UnPossess();
    TestNull(TEXT("Character controller clears on unpossession"), Character->GetController());
    TestFalse(TEXT("ASC no longer caches the old controller"), ASC->AbilityActorInfo->PlayerController.IsValid());

    Controller->Destroy();
    Character->Destroy();
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGGoblinVariantsTest, "ActionRPG.Port.GoblinVariantInitialization",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGGoblinVariantsTest::RunTest(const FString& Parameters)
{
    UBlueprint* Original = LoadObject<UBlueprint>(nullptr,
        TEXT("/Game/Blueprints/NPC/NPC_Goblin_Level_01.NPC_Goblin_Level_01"));
    if (!TestNotNull(TEXT("Original goblin Blueprint"), Original)) return false;
    UPackage* Package = CreatePackage(*FString::Printf(TEXT("/Temp/GoblinVariantTest_%s"), *FGuid::NewGuid().ToString()));
    Package->SetFlags(RF_Transient);
    UBlueprint* Child = FKismetEditorUtilities::CreateBlueprint(Original->GeneratedClass, Package,
        TEXT("ChildGoblin"), BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
    UBlueprint* Duplicate = DuplicateObject<UBlueprint>(Original, Package, TEXT("DuplicatedGoblin"));
    if (!TestNotNull(TEXT("Child Blueprint"), Child) || !TestNotNull(TEXT("Duplicated Blueprint"), Duplicate)) return false;
    FKismetEditorUtilities::CompileBlueprint(Child);
    FKismetEditorUtilities::CompileBlueprint(Duplicate);

    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Test world"), World)) return false;
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    World->InitializeActorsForPlay(FURL());
    float BaselineHealth = 0.f;
    const TArray<UBlueprint*> Blueprints = { Original, Child, Duplicate };
    for (int32 Index = 0; Index < Blueprints.Num(); ++Index)
    {
        UClass* Class = Blueprints[Index]->GeneratedClass;
        if (!TestNotNull(TEXT("Generated variant class"), Class)) continue;
        if (Index > 0)
        {
            TestTrue(TEXT("Variant has its own class"), Class != Original->GeneratedClass);
        }
        FActorSpawnParameters SpawnParameters;
        SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        ARPGCharacterBase* Pawn = World->SpawnActor<ARPGCharacterBase>(Class,
            FVector(Index * 500.f, 0.f, 100.f), FRotator::ZeroRotator, SpawnParameters);
        if (!TestNotNull(TEXT("Spawned goblin variant"), Pawn)) continue;
        if (Index > 0) Pawn->SetActorScale3D(FVector(1.5f));
        AController* Controller = World->SpawnActor<AController>(Pawn->AIControllerClass);
        if (!TestNotNull(TEXT("Inherited AI controller"), Controller)) continue;
        // Explicit possession tests GAS startup; this is not an AI navigation/combat test.
        Controller->Possess(Pawn);
        if (Index > 0) Pawn->SetCharacterLevel(3);
        if (Index > 0) TestTrue(TEXT("Larger variant retains actor scale"), Pawn->GetActorScale3D().Equals(FVector(1.5f)));
        UAbilitySystemComponent* ASC = Pawn->GetAbilitySystemComponent();
        TestTrue(TEXT("Enemy Actor tag inherited"), Pawn->ActorHasTag(TEXT("Enemy")));
        TestEqual(TEXT("Character level inherited/overridden"), Pawn->GetCharacterLevel(), Index == 0 ? 1 : 3);
        TestTrue(TEXT("Inherited stats effect initializes health"), Pawn->GetMaxHealth() > 1.f);
        TestTrue(TEXT("Health starts full"), FMath::IsNearlyEqual(Pawn->GetHealth(), Pawn->GetMaxHealth()));
        if (Index == 0) BaselineHealth = Pawn->GetMaxHealth();
        else TestTrue(TEXT("Higher-level variant has more health"), Pawn->GetMaxHealth() > BaselineHealth);
        if (TestNotNull(TEXT("Variant ASC"), ASC))
        {
            TestTrue(TEXT("ASC avatar initialized"), ASC->GetAvatarActor() == Pawn);
            UClass* Melee = LoadClass<UGameplayAbility>(nullptr,
                TEXT("/Game/Abilities/Enemies/Goblin/GA_GoblinMelee.GA_GoblinMelee_C"));
            TestTrue(TEXT("Inherited melee ability granted"), Melee && ASC->FindAbilitySpecFromClass(Melee) != nullptr);
        }
        Controller->UnPossess();
        Controller->Destroy();
        Pawn->Destroy();
    }
    GEngine->DestroyWorldContext(World);
    World->DestroyWorld(false);
    return true;
}
#endif
