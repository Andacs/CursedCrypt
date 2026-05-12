#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "CCPlayerCharacter.generated.h"

class UInputMappingContext;
class UInputAction;
class USpringArmComponent;
class UCameraComponent;
class UAttributeComponent;
class UAnimMontage;

UCLASS()
class CURSEDCRYPT_API ACCPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ACCPlayerCharacter();

protected:
	virtual void BeginPlay() override;

	// --- BÝLEÞENLER ---
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	UCameraComponent* FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes")
	UAttributeComponent* Attributes;

	// --- INPUT ---
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* IA_Move;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* IA_Look;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* IA_Jump;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* IA_Attack;

	// --- DÖVÜÞ ---
	UPROPERTY(EditAnywhere, Category = "Combat")
	UAnimMontage* AttackMontage;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float AttackStaminaCost = 15.f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float StaminaRegenRate = 10.f;

	// --- FONKSÝYONLAR ---
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

	void Attack();

	// RPC: Sunucuda saldýrý onayý ve stamina harcama
	UFUNCTION(Server, Reliable)
	void Server_Attack();

	// RPC: Animasyonu herkese oynat
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayAttackAnim();

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// --- SALDIRI KÝLÝDÝ (COOLDOWN) ---
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bIsAttacking = false;

	FTimerHandle TimerHandle_AttackLock;

	// Animasyon bitince kilidi açar
	void ResetAttackLock() { bIsAttacking = false; }
};