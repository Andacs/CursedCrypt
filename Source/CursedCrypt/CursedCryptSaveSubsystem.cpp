// Fill out your copyright notice in the Description page of Project Settings.

#include "CursedCryptSaveSubsystem.h"

#include "CursedCryptSaveGame.h"
#include "CursedCryptSaveable.h"
#include "CursedCryptGameInstance.h"

#include "Kismet/GameplayStatics.h"
#include "Misc/DateTime.h"

const FString UCursedCryptSaveSubsystem::DefaultSlotName = TEXT("CursedCrypt_AutoSave");

FString UCursedCryptSaveSubsystem::ResolveSlotName(const FString& SlotName) const
{
    return SlotName.IsEmpty() ? DefaultSlotName : SlotName;
}

void UCursedCryptSaveSubsystem::RegisterSaveable(UObject* Saveable)
{
    if (!Saveable || !Saveable->Implements<UCursedCryptSaveable>())
    {
        return;
    }

    // Avoid duplicate registration.
    for (const TWeakObjectPtr<UObject>& Existing : RegisteredSaveables)
    {
        if (Existing.Get() == Saveable)
        {
            return;
        }
    }

    RegisteredSaveables.Add(Saveable);
}

void UCursedCryptSaveSubsystem::UnregisterSaveable(UObject* Saveable)
{
    RegisteredSaveables.RemoveAll(
        [Saveable](const TWeakObjectPtr<UObject>& Entry)
        {
            return !Entry.IsValid() || Entry.Get() == Saveable;
        });
}

bool UCursedCryptSaveSubsystem::DoesSaveExist(const FString& SlotName) const
{
    return UGameplayStatics::DoesSaveGameExist(ResolveSlotName(SlotName), 0);
}

void UCursedCryptSaveSubsystem::SaveGameAsync(const FString& SlotName)
{
    UCursedCryptSaveGame* SaveObject = Cast<UCursedCryptSaveGame>(
        UGameplayStatics::CreateSaveGameObject(UCursedCryptSaveGame::StaticClass()));

    if (!SaveObject)
    {
        OnSaveCompleted.Broadcast(false);
        return;
    }

    // Gather JSON from every live saveable.
    for (int32 Index = RegisteredSaveables.Num() - 1; Index >= 0; --Index)
    {
        UObject* Saveable = RegisteredSaveables[Index].Get();
        if (!Saveable)
        {
            RegisteredSaveables.RemoveAt(Index);
            continue;
        }

        const FString Id = ICursedCryptSaveable::Execute_GetSaveableId(Saveable);
        const FString Json = ICursedCryptSaveable::Execute_GatherSaveData(Saveable);

        if (!Id.IsEmpty())
        {
            SaveObject->SerializedActorData.Add(Id, Json);
        }
    }

    // Capture global state from the GameInstance.
    if (UCursedCryptGameInstance* GI = Cast<UCursedCryptGameInstance>(GetGameInstance()))
    {
        SaveObject->SavedLanguage = GI->CurrentLanguage;
    }

    SaveObject->SaveTimestamp = FDateTime::UtcNow().ToIso8601();

    FAsyncSaveGameToSlotDelegate Delegate;
    Delegate.BindUObject(this, &UCursedCryptSaveSubsystem::HandleSaveCompleted);
    UGameplayStatics::AsyncSaveGameToSlot(SaveObject, ResolveSlotName(SlotName), 0, Delegate);
}

void UCursedCryptSaveSubsystem::LoadGameAsync(const FString& SlotName)
{
    const FString ResolvedSlot = ResolveSlotName(SlotName);

    if (!UGameplayStatics::DoesSaveGameExist(ResolvedSlot, 0))
    {
        OnLoadCompleted.Broadcast(false);
        return;
    }

    FAsyncLoadGameFromSlotDelegate Delegate;
    Delegate.BindUObject(this, &UCursedCryptSaveSubsystem::HandleLoadCompleted);
    UGameplayStatics::AsyncLoadGameFromSlot(ResolvedSlot, 0, Delegate);
}

void UCursedCryptSaveSubsystem::HandleSaveCompleted(const FString& SlotName, int32 UserIndex, bool bSuccess)
{
    OnSaveCompleted.Broadcast(bSuccess);
}

void UCursedCryptSaveSubsystem::HandleLoadCompleted(const FString& SlotName, int32 UserIndex, USaveGame* LoadedSave)
{
    UCursedCryptSaveGame* SaveObject = Cast<UCursedCryptSaveGame>(LoadedSave);
    if (!SaveObject)
    {
        OnLoadCompleted.Broadcast(false);
        return;
    }

    // Restore global state first so language is set before UI refresh.
    if (UCursedCryptGameInstance* GI = Cast<UCursedCryptGameInstance>(GetGameInstance()))
    {
        if (!SaveObject->SavedLanguage.IsEmpty())
        {
            GI->SetLanguage(SaveObject->SavedLanguage);
        }
    }

    // Dispatch JSON back to every registered saveable that matches an entry.
    for (int32 Index = RegisteredSaveables.Num() - 1; Index >= 0; --Index)
    {
        UObject* Saveable = RegisteredSaveables[Index].Get();
        if (!Saveable)
        {
            RegisteredSaveables.RemoveAt(Index);
            continue;
        }

        const FString Id = ICursedCryptSaveable::Execute_GetSaveableId(Saveable);
        if (const FString* Payload = SaveObject->SerializedActorData.Find(Id))
        {
            ICursedCryptSaveable::Execute_ApplySaveData(Saveable, *Payload);
        }
    }

    OnLoadCompleted.Broadcast(true);
}