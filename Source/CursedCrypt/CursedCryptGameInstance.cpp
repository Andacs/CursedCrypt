// Fill out your copyright notice in the Description page of Project Settings.

#include "CursedCryptGameInstance.h"

UCursedCryptGameInstance::UCursedCryptGameInstance()
{
    CurrentLanguage = TEXT("TR");
    InitializeTranslationTable();
}

void UCursedCryptGameInstance::InitializeTranslationTable()
{
    // Each entry: Key -> (Turkish text, English text)
    TranslationTable.Add(TEXT("play"), TPair<FText, FText>(
        FText::FromString(TEXT("Oyna")),
        FText::FromString(TEXT("Play"))));

    TranslationTable.Add(TEXT("quit"), TPair<FText, FText>(
        FText::FromString(TEXT("Çıkış")),
        FText::FromString(TEXT("Quit"))));

    TranslationTable.Add(TEXT("settings"), TPair<FText, FText>(
        FText::FromString(TEXT("Ayarlar")),
        FText::FromString(TEXT("Settings"))));

    TranslationTable.Add(TEXT("inventory"), TPair<FText, FText>(
        FText::FromString(TEXT("Envanter")),
        FText::FromString(TEXT("Inventory"))));

    TranslationTable.Add(TEXT("you_died"), TPair<FText, FText>(
        FText::FromString(TEXT("Oyunu Kaybettin")),
        FText::FromString(TEXT("You Died"))));

    TranslationTable.Add(TEXT("restart"), TPair<FText, FText>(
        FText::FromString(TEXT("Yeniden Oyna")),
        FText::FromString(TEXT("Restart"))));

    TranslationTable.Add(TEXT("main_menu"), TPair<FText, FText>(
        FText::FromString(TEXT("Ana Menü")),
        FText::FromString(TEXT("Main Menu"))));

    TranslationTable.Add(TEXT("language"), TPair<FText, FText>(
        FText::FromString(TEXT("Dil")),
        FText::FromString(TEXT("Language"))));

    TranslationTable.Add(TEXT("back"), TPair<FText, FText>(
        FText::FromString(TEXT("Geri")),
        FText::FromString(TEXT("Back"))));
}

FText UCursedCryptGameInstance::GetLocalizedText(const FString& Key) const
{
    const TPair<FText, FText>* Entry = TranslationTable.Find(Key);
    if (!Entry)
    {
        // Fallback: return the key wrapped in brackets so missing translations are visible.
        return FText::FromString(FString::Printf(TEXT("[%s]"), *Key));
    }

    // Return TR by default; EN only when explicitly selected.
    return (CurrentLanguage == TEXT("EN")) ? Entry->Value : Entry->Key;
}

void UCursedCryptGameInstance::SetLanguage(const FString& NewLanguage)
{
    if (NewLanguage != TEXT("TR") && NewLanguage != TEXT("EN"))
    {
        return; // Ignore invalid values.
    }

    if (CurrentLanguage == NewLanguage)
    {
        return; // Already set, no need to broadcast.
    }

    CurrentLanguage = NewLanguage;
    OnLanguageChanged.Broadcast();
}