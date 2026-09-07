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

	// --- Components ---
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	UCameraComponent* FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes")
	UAttributeComponent* Attributes;

	// --- Input ---
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

	// --- Combat ---
	UPROPERTY(EditAnywhere, Category = "Combat")
	UAnimMontage* AttackMontage;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float AttackStaminaCost = 15.f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float StaminaRegenRate = 10.f;

	// --- Input handlers ---
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

	void Attack();

	// RPC: server-side attack validation and stamina consumption.
	UFUNCTION(Server, Reliable)
	void Server_Attack();

	// RPC: play the attack animation on all clients.
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayAttackAnim();

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	// --- Attack cooldown ---
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	bool bIsAttacking = false;

	FTimerHandle TimerHandle_AttackLock;

	// Releases the attack lock when the montage finishes.
	void ResetAttackLock() { bIsAttacking = false; }
};