// Fill out your copyright notice in the Description page of Project Settings.

#include "CursedCryptSaveable.h"

#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

bool UCursedCryptSaveableLibrary::ParsePlayerSaveData(
    const FString& JsonData,
    float& X, float& Y, float& Z, float& HP)
{
    X = Y = Z = HP = 0.f;

    if (JsonData.IsEmpty())
    {
        return false;
    }

    TSharedPtr<FJsonObject> JsonObject;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonData);
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    // All four numeric fields are required for a successful parse.
    const bool bHasAll =
        JsonObject->TryGetNumberField(TEXT("x"), X) &
        JsonObject->TryGetNumberField(TEXT("y"), Y) &
        JsonObject->TryGetNumberField(TEXT("z"), Z) &
        JsonObject->TryGetNumberField(TEXT("hp"), HP);

    return bHasAll;
}

FString UCursedCryptSaveableLibrary::FormatPlayerSaveData(const FString& Id, float X, float Y, float Z, float HP)
{
    // FString::Printf uses C locale (invariant), so '.' is always the decimal separator.
    // This is critical because BP's Format Text uses system locale and produces invalid
    // JSON on non-English locales (e.g. Turkish: 1.234 -> "1,234").
    return FString::Printf(TEXT("{\"id\":\"%s\",\"x\":%f,\"y\":%f,\"z\":%f,\"hp\":%f}"),
        *Id, X, Y, Z, HP);
}