#include "RPGInspectContentCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Animation/AnimMontage.h"
#include "Components/ActorComponent.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "Engine/Blueprint.h"
#include "Engine/DataTable.h"
#include "StructUtils/UserDefinedStruct.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/UObjectHash.h"
#include "UObject/UnrealType.h"

namespace
{
    FString OneLine(FString Value)
    {
        return Value.Replace(TEXT("\r"), TEXT("")).Replace(TEXT("\n"), TEXT(" | "));
    }

    void DumpProperties(UObject* Object, FString& Out)
    {
        for (TFieldIterator<FProperty> It(Object->GetClass()); It; ++It)
        {
            FProperty* Property = *It;
            if (Property->HasAnyPropertyFlags(CPF_Transient) || !Property->HasAnyPropertyFlags(CPF_Edit | CPF_BlueprintVisible)) continue;
            FString Value;
            Property->ExportText_InContainer(0, Value, Object, Object, Object, PPF_None);
            if (Value == TEXT("None") || Value == TEXT("()") || Value.IsEmpty()) continue;
            Out += FString::Printf(TEXT("  %s [%s] = %s\n"), *Property->GetName(), *Property->GetCPPType(), *OneLine(Value));
        }
    }
}

URPGInspectContentCommandlet::URPGInspectContentCommandlet()
{
    IsClient = false;
    IsEditor = true;
    IsServer = false;
    LogToConsole = true;
}

int32 URPGInspectContentCommandlet::Main(const FString& Params)
{
    FString Output = FPaths::ProjectSavedDir() / TEXT("DocsInspection.txt");
    FParse::Value(*Params, TEXT("Output="), Output);
    FString Dump = TEXT("ActionRPG content inspection. Read-only; no packages saved.\n");
    FAssetRegistryModule& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    Registry.Get().SearchAllAssets(true);
    TArray<FAssetData> Assets;
    const TCHAR* Roots[] = {TEXT("/Game/Items"), TEXT("/Game/Blueprints"), TEXT("/Game/Abilities"), TEXT("/Game/Characters/Animations")};
    for (const TCHAR* Root : Roots) Registry.Get().GetAssetsByPath(FName(Root), Assets, true);
    Assets.Sort([](const FAssetData& A, const FAssetData& B) { return A.PackageName.LexicalLess(B.PackageName); });
    int32 Count = 0;
    for (const FAssetData& Asset : Assets)
    {
        const FString Path = Asset.PackageName.ToString();
        // UI is limited to inventory/shop: unrelated title/localization widgets add noise.
        if (Path.Contains(TEXT("/WidgetBP/")) && !Path.Contains(TEXT("/Inventory/"))) continue;
        UObject* Object = Asset.GetAsset();
        if (!Object) { UE_LOG(LogTemp, Error, TEXT("Cannot load authoring asset %s"), *Path); return 1; }
        UBlueprint* Blueprint = Cast<UBlueprint>(Object);
        UDataTable* Table = Cast<UDataTable>(Object);
        UUserDefinedStruct* Struct = Cast<UUserDefinedStruct>(Object);
        UAnimMontage* Montage = Cast<UAnimMontage>(Object);
        const bool bItem = Path.StartsWith(TEXT("/Game/Items/")) && !Blueprint;
        if (!Blueprint && !Table && !Struct && !Montage && !bItem) continue;
        ++Count;
        Dump += FString::Printf(TEXT("\n=== ASSET %s (%s) ===\n"), *Object->GetPathName(), *Object->GetClass()->GetName());
        if (Table) Dump += Table->GetTableAsJSON(EDataTableExportFlags::UseJsonObjectsForStructs) + TEXT("\n");
        if (Struct)
        {
            for (TFieldIterator<FProperty> It(Struct); It; ++It)
                Dump += FString::Printf(TEXT("FIELD %s display=%s type=%s\n"), *It->GetName(), *It->GetDisplayNameText().ToString(), *It->GetCPPType());
        }
        if (Blueprint && Blueprint->GeneratedClass)
        {
            Dump += TEXT("PARENT ") + GetPathNameSafe(Blueprint->ParentClass) + TEXT("\nDEFAULTS\n");
            UObject* CDO = Blueprint->GeneratedClass->GetDefaultObject();
            DumpProperties(CDO, Dump);
            TArray<UObject*> Subobjects;
            GetObjectsWithOuter(CDO, Subobjects, EGetObjectsFlags::IncludeNestedObjects);
            for (UObject* Subobject : Subobjects)
            {
                if (Cast<UActorComponent>(Subobject))
                {
                    Dump += TEXT("COMPONENT ") + Subobject->GetName() + TEXT(" ") + Subobject->GetClass()->GetName() + TEXT("\n");
                    DumpProperties(Subobject, Dump);
                }
            }
            TArray<UEdGraph*> Graphs;
            Blueprint->GetAllGraphs(Graphs);
            for (const UEdGraph* Graph : Graphs)
            {
                Dump += TEXT("GRAPH ") + Graph->GetName() + TEXT("\n");
                for (const UEdGraphNode* Node : Graph->Nodes)
                {
                    if (!Node) continue;
                    Dump += FString::Printf(TEXT("NODE %s %s %s\n"), *Node->NodeGuid.ToString(EGuidFormats::Digits), *Node->GetClass()->GetName(), *OneLine(Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString()));
                    for (const UEdGraphPin* Pin : Node->Pins)
                    {
                        if (!Pin || Pin->bHidden) continue;
                        FString Links;
                        for (const UEdGraphPin* Link : Pin->LinkedTo)
                            if (Link && Link->GetOwningNode()) Links += Link->GetOwningNode()->NodeGuid.ToString(EGuidFormats::Digits) + TEXT(".") + Link->PinName.ToString() + TEXT(" ");
                        Dump += FString::Printf(TEXT("  %s %s default=%s object=%s links=%s\n"), Pin->Direction == EGPD_Input ? TEXT("IN") : TEXT("OUT"), *Pin->PinName.ToString(), *OneLine(Pin->GetDefaultAsString()), *GetPathNameSafe(Pin->DefaultObject), *Links);
                    }
                }
            }
        }
        else if (bItem) DumpProperties(Object, Dump);
        if (Montage)
        {
            Dump += FString::Printf(TEXT("MONTAGE skeleton=%s length=%f\n"), *GetPathNameSafe(Montage->GetSkeleton()), Montage->GetPlayLength());
            for (const FAnimNotifyEvent& Notify : Montage->Notifies)
            {
                Dump += FString::Printf(TEXT("NOTIFY time=%f name=%s\n"), Notify.GetTime(), *Notify.NotifyName.ToString());
                if (Notify.Notify) DumpProperties(Notify.Notify, Dump);
                if (Notify.NotifyStateClass) DumpProperties(Notify.NotifyStateClass, Dump);
            }
        }
    }
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Output), true);
    if (!FFileHelper::SaveStringToFile(Dump, *Output, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)) return 1;
    UE_LOG(LogTemp, Display, TEXT("Inspected %d assets; wrote %s"), Count, *Output);
    return 0;
}
