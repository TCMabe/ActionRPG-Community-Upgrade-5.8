#include "RPGUpgradeSampleCommandlet.h"

#include "Engine/Blueprint.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "GameplayTagsManager.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "K2Node_CallFunction.h"
#include "NavigationSystem.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"
#include "Animation/WidgetAnimation.h"
#include "Components/Widget.h"
#include "Tracks/MovieScenePropertyTrack.h"
#include "MovieScene.h"
#include "MovieSceneBinding.h"

URPGUpgradeSampleCommandlet::URPGUpgradeSampleCommandlet()
{
    IsClient = false;
    IsEditor = true;
    IsServer = false;
    LogToConsole = true;
}

int32 URPGUpgradeSampleCommandlet::Main(const FString& Params)
{
    if (Params.Contains(TEXT("InspectWidgets")))
    {
        const TCHAR* Paths[] = {
            TEXT("/Game/Blueprints/WidgetBP/HUD/WB_HUD_Mobile"),
            TEXT("/Game/Blueprints/WidgetBP/HUD/WB_HUD_PC"),
            TEXT("/Game/Blueprints/WidgetBP/Inventory/WB_Equipment"),
            TEXT("/Game/Blueprints/WidgetBP/Inventory/WB_PurchaseConfirm"),
            TEXT("/Game/Blueprints/WidgetBP/WB_WaveStart")
        };
        for (const TCHAR* Path : Paths)
        {
            UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, Path);
            if (!Blueprint) return 1;
            TArray<UObject*> Objects;
            GetObjectsWithOuter(Blueprint, Objects, EGetObjectsFlags::IncludeNestedObjects);
            UE_LOG(LogTemp, Display, TEXT("PORT_WIDGET %s"), Path);
            for (UObject* Object : Objects)
            {
                if (UWidget* Widget = Cast<UWidget>(Object))
                {
                    UE_LOG(LogTemp, Display, TEXT("PORT_WIDGET widget=%s class=%s"), *Widget->GetName(), *Widget->GetClass()->GetName());
                }
                if (UWidgetAnimation* Animation = Cast<UWidgetAnimation>(Object))
                {
                    for (const FWidgetAnimationBinding& Binding : Animation->GetBindings())
                    {
                        UE_LOG(LogTemp, Display, TEXT("PORT_WIDGET animation=%s widget=%s slot=%s root=%d guid=%s"),
                            *Animation->GetName(), *Binding.WidgetName.ToString(), *Binding.SlotWidgetName.ToString(), Binding.bIsRootWidget, *Binding.AnimationGuid.ToString());
                    }
                }
                if (const UMovieScenePropertyTrack* Track = Cast<UMovieScenePropertyTrack>(Object))
                {
                    UE_LOG(LogTemp, Display, TEXT("PORT_WIDGET track=%s property=%s path=%s"), *Track->GetPathName(), *Track->GetPropertyName().ToString(), *Track->GetPropertyPath().ToString());
                }
            }
        }
        return 0;
    }
    const TCHAR* EffectPath = TEXT("/Game/Abilities/Enemies/Goblin/GE_GoblinRange.GE_GoblinRange");
    UBlueprint* EffectBlueprint = LoadObject<UBlueprint>(nullptr, EffectPath);
    if (!EffectBlueprint || !EffectBlueprint->GeneratedClass)
    {
        UE_LOG(LogTemp, Error, TEXT("Sample upgrade: cannot load %s"), EffectPath);
        return 1;
    }

    UGameplayEffect* Effect = Cast<UGameplayEffect>(EffectBlueprint->GeneratedClass->GetDefaultObject());
    const FGameplayTag CooldownTag = FGameplayTag::RequestGameplayTag(TEXT("Cooldown.Goblin.Ranged"));
    if (!Effect || !CooldownTag.IsValid() || Effect->DurationPolicy != EGameplayEffectDurationType::HasDuration)
    {
        UE_LOG(LogTemp, Error, TEXT("Sample upgrade: unexpected goblin cooldown configuration"));
        return 1;
    }

    if (!Effect->GetGrantedTags().HasTagExact(CooldownTag))
    {
        Effect->Modify();
        UTargetTagsGameplayEffectComponent& Tags = Effect->FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
        FInheritedTagContainer Changes = Tags.GetConfiguredTargetTagChanges();
        Changes.Added.AddTag(CooldownTag);
        Tags.SetAndApplyTargetTagChanges(Changes);
        EffectBlueprint->Modify();
        FKismetEditorUtilities::CompileBlueprint(EffectBlueprint);

        Effect = CastChecked<UGameplayEffect>(EffectBlueprint->GeneratedClass->GetDefaultObject());
        if (EffectBlueprint->Status == BS_Error || !Effect->GetGrantedTags().HasTagExact(CooldownTag))
        {
            UE_LOG(LogTemp, Error, TEXT("Sample upgrade: cooldown repair did not survive Blueprint compilation"));
            return 1;
        }

        UPackage* Package = EffectBlueprint->GetOutermost();
        FSavePackageArgs SaveArgs;
        SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
        SaveArgs.Error = GError;
        const FString Filename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
        if (!UPackage::SavePackage(Package, EffectBlueprint, *Filename, SaveArgs))
        {
            UE_LOG(LogTemp, Error, TEXT("Sample upgrade: failed to save %s"), *Filename);
            return 1;
        }
    }

    UE_LOG(LogTemp, Display, TEXT("Sample upgrade: goblin ranged cooldown grants %s"), *CooldownTag.ToString());

    // The old node is pure. Keep its evaluation behavior and existing data links
    // when replacing the deprecated API with the current callable function.
    const TCHAR* NavigationBlueprints[] = {
        TEXT("/Game/Blueprints/AI/BTTask_GoAroundTarget.BTTask_GoAroundTarget"),
        TEXT("/Game/Blueprints/BP_PlayerCharacter.BP_PlayerCharacter")
    };
    for (const TCHAR* Path : NavigationBlueprints)
    {
        UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, Path);
        if (!Blueprint)
        {
            UE_LOG(LogTemp, Error, TEXT("Sample upgrade: cannot load %s"), Path);
            return 1;
        }
        TArray<UK2Node_CallFunction*> Nodes;
        FBlueprintEditorUtils::GetAllNodesOfClass(Blueprint, Nodes);
        bool bChanged = false;
        for (UK2Node_CallFunction* Node : Nodes)
        {
            if (Node->FunctionReference.GetMemberName() == TEXT("K2_GetRandomPointInNavigableRadius") &&
                Node->FunctionReference.GetMemberParentClass() == UNavigationSystemV1::StaticClass())
            {
                Node->Modify();
                Node->SetFromFunction(UNavigationSystemV1::StaticClass()->FindFunctionByName(TEXT("K2_GetRandomLocationInNavigableRadius")));
                // Purity is an editor-serialized setting without a public setter.
                FProperty* Purity = FindFProperty<FProperty>(UK2Node_CallFunction::StaticClass(), TEXT("NodePurityOverride"));
                if (!Purity || !Purity->ImportText_Direct(TEXT("Pure"), Purity->ContainerPtrToValuePtr<void>(Node), Node, PPF_None))
                {
                    UE_LOG(LogTemp, Error, TEXT("Sample upgrade: cannot preserve navigation node purity"));
                    return 1;
                }
                Node->ReconstructNode();
                bChanged = true;
            }
        }
        if (bChanged)
        {
            FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
            FKismetEditorUtilities::CompileBlueprint(Blueprint);
            if (Blueprint->Status == BS_Error)
            {
                UE_LOG(LogTemp, Error, TEXT("Sample upgrade: navigation Blueprint failed compilation: %s"), Path);
                return 1;
            }
            FSavePackageArgs SaveArgs;
            SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
            SaveArgs.Error = GError;
            UPackage* Package = Blueprint->GetOutermost();
            const FString Filename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
            if (!UPackage::SavePackage(Package, Blueprint, *Filename, SaveArgs))
            {
                return 1;
            }
        }
        UE_LOG(LogTemp, Display, TEXT("Sample upgrade: navigation nodes checked: %s"), Path);
    }

    // These bindings refer to widgets absent from the original sample's trees.
    // Keep the animations and all valid tracks; remove only the existing no-op bindings.
    const TCHAR* WidgetPaths[] = {
        TEXT("/Game/Blueprints/WidgetBP/HUD/WB_HUD_Mobile"),
        TEXT("/Game/Blueprints/WidgetBP/HUD/WB_HUD_PC"),
        TEXT("/Game/Blueprints/WidgetBP/Inventory/WB_Equipment"),
        TEXT("/Game/Blueprints/WidgetBP/Inventory/WB_PurchaseConfirm"),
        TEXT("/Game/Blueprints/WidgetBP/WB_WaveStart")
    };
    for (const TCHAR* Path : WidgetPaths)
    {
        UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, Path);
        if (!Blueprint) return 1;
        TArray<UObject*> Objects;
        GetObjectsWithOuter(Blueprint, Objects, EGetObjectsFlags::IncludeNestedObjects);
        TSet<FName> WidgetNames;
        for (UObject* Object : Objects)
        {
            if (const UWidget* Widget = Cast<UWidget>(Object)) WidgetNames.Add(Widget->GetFName());
        }
        bool bChanged = false;
        for (UObject* Object : Objects)
        {
            if (UMovieScenePropertyTrack* Track = Cast<UMovieScenePropertyTrack>(Object))
            {
                if (FString(Path).EndsWith(TEXT("WB_WaveStart")) && Track->GetPropertyName() == TEXT("ContentColorAndOpacity"))
                {
                    bool bTargetsWaveText = false;
                    if (const UWidgetAnimation* Animation = Track->GetTypedOuter<UWidgetAnimation>())
                    {
                        for (const FWidgetAnimationBinding& Binding : Animation->GetBindings())
                        {
                            if (Binding.WidgetName == TEXT("WaveText") && Animation->MovieScene)
                            {
                                if (const FMovieSceneBinding* SceneBinding = Animation->MovieScene->FindBinding(Binding.AnimationGuid))
                                {
                                    bTargetsWaveText |= SceneBinding->GetTracks().Contains(Track);
                                }
                            }
                        }
                    }
                    if (!bTargetsWaveText)
                    {
                        // Borders legitimately have ContentColorAndOpacity; leave those tracks alone.
                        continue;
                    }
                    // The target is WaveText (TextBlock), whose color is a SlateColor.
                    // MovieScene's color property traits support SlateColor directly.
                    Track->Modify();
                    Track->SetPropertyNameAndPath(TEXT("ColorAndOpacity"), TEXT("ColorAndOpacity"));
                    bChanged = true;
                    UE_LOG(LogTemp, Display, TEXT("Sample upgrade: repaired WaveText color animation property"));
                }
            }
            if (UWidgetAnimation* Animation = Cast<UWidgetAnimation>(Object))
            {
                const TArray<FWidgetAnimationBinding> Bindings = Animation->GetBindings();
                for (const FWidgetAnimationBinding& Binding : Bindings)
                {
                    const bool bKnownOrphan = Binding.WidgetName == TEXT("NPCKillCount") ||
                        Binding.WidgetName == TEXT("CanvasPanel_0") || Binding.WidgetName == TEXT("CanvasPanel_1");
                    if (bKnownOrphan && !Binding.bIsRootWidget && !WidgetNames.Contains(Binding.WidgetName))
                    {
                        Animation->Modify();
                        Animation->RemoveBinding(Binding);
                        bChanged = true;
                        UE_LOG(LogTemp, Display, TEXT("Sample upgrade: removed orphan animation binding %s in %s"), *Binding.WidgetName.ToString(), Path);
                    }
                }
            }
        }
        if (bChanged)
        {
            FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
            FKismetEditorUtilities::CompileBlueprint(Blueprint);
            if (Blueprint->Status == BS_Error) return 1;
            FSavePackageArgs SaveArgs;
            SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
            SaveArgs.Error = GError;
            UPackage* Package = Blueprint->GetOutermost();
            const FString Filename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
            if (!UPackage::SavePackage(Package, Blueprint, *Filename, SaveArgs)) return 1;
        }
    }
    return 0;
}
