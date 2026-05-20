// Copyright Epic Games, Inc. All Rights Reserved.


#include "CursedCryptPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "CursedCrypt.h"
#include "CursedCryptSaveSubsystem.h"
#include "Engine/GameInstance.h"
#include "Widgets/Input/SVirtualJoystick.h"

void ACursedCryptPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// only spawn touch controls on local player controllers
	if (SVirtualJoystick::ShouldDisplayTouchInterface() && IsLocalPlayerController())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		}
		else {

			UE_LOG(LogCursedCrypt, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}
}

void ACursedCryptPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!SVirtualJoystick::ShouldDisplayTouchInterface())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}

		// Bind save/load debug keys directly. L = Save, K = Load.
		// Raw key binding (not Enhanced Input) is used for these temporary debug
		// shortcuts; they will be replaced by a pause menu later.
		InputComponent->BindKey(EKeys::L, IE_Pressed, this, &ACursedCryptPlayerController::HandleSaveKey);
		InputComponent->BindKey(EKeys::K, IE_Pressed, this, &ACursedCryptPlayerController::HandleLoadKey);
	}
}

void ACursedCryptPlayerController::HandleSaveKey()
{
	// Resolve save subsystem from owning GameInstance and trigger async save.
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UCursedCryptSaveSubsystem* SaveSubsystem = GI->GetSubsystem<UCursedCryptSaveSubsystem>())
		{
			UE_LOG(LogTemp, Log, TEXT("[Controller] Save key pressed"));
			SaveSubsystem->SaveGameAsync(FString());
		}
	}
}

void ACursedCryptPlayerController::HandleLoadKey()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UCursedCryptSaveSubsystem* SaveSubsystem = GI->GetSubsystem<UCursedCryptSaveSubsystem>())
		{
			UE_LOG(LogTemp, Log, TEXT("[Controller] Load key pressed"));
			SaveSubsystem->LoadGameAsync(FString());
		}
	}
}