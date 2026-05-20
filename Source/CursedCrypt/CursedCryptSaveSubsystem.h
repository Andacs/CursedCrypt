// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CursedCryptSaveSubsystem.generated.h"

class UCursedCryptSaveGame;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSaveCompleted, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLoadCompleted, bool, bSuccess);

/**
 * GameInstance subsystem that orchestrates the save/load pipeline.
 *
 * Responsibilities:
 *  - Maintain a registry of saveable actors (ICursedCryptSaveable implementors).
 *  - On save: gather JSON from each registered saveable, write asynchronously.
 *  - On load: read save asynchronously, dispatch JSON back to each saveable.
 *  - Persist global state (language) on the SaveGame object.
 */
UCLASS()
class CURSEDCRYPT_API UCursedCryptSaveSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    /** Default slot name; passing an empty SlotName to Save/Load uses this. */
    static const FString DefaultSlotName;

    /** Register an actor (or any UObject) that implements ICursedCryptSaveable. */
    UFUNCTION(BlueprintCallable, Category = "Save System")
    void RegisterSaveable(UObject* Saveable);

    /** Unregister a previously registered saveable. */
    UFUNCTION(BlueprintCallable, Category = "Save System")
    void UnregisterSaveable(UObject* Saveable);

    /**
     * Save the game asynchronously.
     * @param SlotName  Save slot name. Empty means DefaultSlotName.
     */
    UFUNCTION(BlueprintCallable, Category = "Save System")
    void SaveGameAsync(const FString& SlotName);

    /**
     * Load the game asynchronously.
     * @param SlotName  Save slot name. Empty means DefaultSlotName.
     */
    UFUNCTION(BlueprintCallable, Category = "Save System")
    void LoadGameAsync(const FString& SlotName);

    /** Returns true if a save exists in the given slot. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Save System")
    bool DoesSaveExist(const FString& SlotName) const;

    /** Broadcast after an async save attempt completes. */
    UPROPERTY(BlueprintAssignable, Category = "Save System")
    FOnSaveCompleted OnSaveCompleted;

    /** Broadcast after an async load attempt completes. */
    UPROPERTY(BlueprintAssignable, Category = "Save System")
    FOnLoadCompleted OnLoadCompleted;

private:
    /** Weak references to registered saveable objects. */
    UPROPERTY()
    TArray<TWeakObjectPtr<UObject>> RegisteredSaveables;

    /** Resolves an empty slot name to the default. */
    FString ResolveSlotName(const FString& SlotName) const;

    /** Engine callback for AsyncSaveGameToSlot. */
    void HandleSaveCompleted(const FString& SlotName, int32 UserIndex, bool bSuccess);

    /** Engine callback for AsyncLoadGameFromSlot. */
    void HandleLoadCompleted(const FString& SlotName, int32 UserIndex, class USaveGame* LoadedSave);
};