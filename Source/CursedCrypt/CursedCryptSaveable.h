// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CursedCryptSaveable.generated.h"

/**
 * Interface marking an object as participating in the save/load system.
 *
 * Implement this on any actor (or component) whose state should be persisted.
 * The save subsystem will call GatherSaveData() during save and ApplySaveData()
 * during load to serialize/restore the object's relevant state as JSON.
 */
UINTERFACE(MinimalAPI, Blueprintable)
class UCursedCryptSaveable : public UInterface
{
    GENERATED_BODY()
};

class CURSEDCRYPT_API ICursedCryptSaveable
{
    GENERATED_BODY()

public:
    /**
     * Called by the save subsystem to collect this object's state.
     * Implementations should serialize their relevant state into a JSON string.
     * @return Serialized JSON string representing this object's state.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Save System")
    FString GatherSaveData();
    virtual FString GatherSaveData_Implementation() { return TEXT("{}"); }

    /**
     * Called by the save subsystem to restore this object's state.
     * @param JsonData The JSON string previously returned by GatherSaveData().
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Save System")
    void ApplySaveData(const FString& JsonData);
    virtual void ApplySaveData_Implementation(const FString& JsonData) {}

    /**
     * Stable identifier used as the key when storing this object's data.
     * Default implementation returns the actor's name; override if you need a
     * different key (e.g. a GUID that survives renames).
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Save System")
    FString GetSaveableId() const;
    virtual FString GetSaveableId_Implementation() const { return FString(); }
};

/**
 * Static helper utilities for JSON parsing in Blueprint contexts.
 *
 * UE's JSON API is C++-only. BP cannot easily parse JSON natively, so this
 * exposes parsing helpers via UBlueprintFunctionLibrary callable from BP.
 */
UCLASS()
class CURSEDCRYPT_API UCursedCryptSaveableLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * Parse a player save payload JSON string into individual fields.
     * Expected JSON shape: {"id":"...","x":N,"y":N,"z":N,"hp":N}
     *
     * @param JsonData  Raw JSON string previously produced by GatherSaveData()
     * @param X,Y,Z     Output position coordinates
     * @param HP        Output health value
     * @return          True if parsing succeeded and all expected fields were found
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Save System")
    static bool ParsePlayerSaveData(const FString& JsonData, float& X, float& Y, float& Z, float& HP);

    /**
     * Format player save data into a locale-invariant JSON string.
     * Uses FString::Printf which always emits '.' as decimal separator,
     * regardless of system locale (critical: Turkish Windows uses ',' by default).
     *
     * @param Id   Saveable identifier (e.g. "Player_01")
     * @param X,Y,Z Position coordinates
     * @param HP   Health value
     * @return     JSON string like {"id":"Player_01","x":1234.5,"y":-567.8,"z":98.2,"hp":75.0}
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Save System")
    static FString FormatPlayerSaveData(const FString& Id, float X, float Y, float Z, float HP);
};