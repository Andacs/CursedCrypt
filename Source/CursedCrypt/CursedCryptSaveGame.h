// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "CursedCryptSaveGame.generated.h"

/**
 * Persistent storage container for the CursedCrypt save/load system.
 *
 * Stores per-actor JSON payloads gathered through ICursedCryptSaveable,
 * along with global game-wide state (e.g. language preference).
 */
UCLASS()
class CURSEDCRYPT_API UCursedCryptSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    /** JSON payloads keyed by saveable id (typically actor name or GUID). */
    UPROPERTY(BlueprintReadWrite, Category = "Save System")
    TMap<FString, FString> SerializedActorData;

    /** Active language code at the time of save ("TR" or "EN"). */
    UPROPERTY(BlueprintReadWrite, Category = "Save System")
    FString SavedLanguage;

    /** Timestamp of when the save was created (UTC, ISO-8601). */
    UPROPERTY(BlueprintReadWrite, Category = "Save System")
    FString SaveTimestamp;

    /** Version of the save format. Bump when schema changes break compatibility. */
    UPROPERTY(BlueprintReadWrite, Category = "Save System")
    int32 SaveVersion = 1;
};