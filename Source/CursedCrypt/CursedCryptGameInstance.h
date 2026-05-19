// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "CursedCryptGameInstance.generated.h"

/**
 * Game Instance for CursedCrypt.
 * Manages persistent game-wide state including language selection (i18n).
 */
UCLASS()
class CURSEDCRYPT_API UCursedCryptGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    UCursedCryptGameInstance();

    /** Current active language code. "TR" or "EN". */
    UPROPERTY(BlueprintReadOnly, Category = "Localization")
    FString CurrentLanguage;

    /**
     * Returns the localized text for a given key based on the current language.
     * @param Key  Identifier for the string (e.g. "play", "quit", "you_died")
     * @return     Localized FText for the active language
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Localization")
    FText GetLocalizedText(const FString& Key) const;

    /**
     * Sets the active language and broadcasts the change so widgets can refresh.
     * @param NewLanguage  "TR" or "EN"
     */
    UFUNCTION(BlueprintCallable, Category = "Localization")
    void SetLanguage(const FString& NewLanguage);

    /** Broadcast when language changes; widgets bind to this to refresh their text. */
    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLanguageChanged);

    UPROPERTY(BlueprintAssignable, Category = "Localization")
    FOnLanguageChanged OnLanguageChanged;

private:
    /** Internal translation table. Key -> (TR text, EN text) */
    TMap<FString, TPair<FText, FText>> TranslationTable;

    /** Populates TranslationTable with all UI strings. */
    void InitializeTranslationTable();
};