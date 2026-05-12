#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AttributeComponent.generated.h"

// Delegate Tanýmlarý
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnHealthChanged, AActor*, InstigatorActor, UAttributeComponent*, OwningComp, float, NewHealth, float, Delta);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnStaminaChanged, AActor*, InstigatorActor, UAttributeComponent*, OwningComp, float, NewStamina, float, Delta);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class CURSEDCRYPT_API UAttributeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAttributeComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// --- Delegate Eventleri ---
	UPROPERTY(BlueprintAssignable, Category = "Attributes")
	FOnHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Attributes")
	FOnStaminaChanged OnStaminaChanged;

	// --- Getter Fonksiyonlar ---
	UFUNCTION(BlueprintPure, Category = "Attributes")
	float GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category = "Attributes")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "Attributes")
	bool IsAlive() const { return Health > 0.f; }

	// --- Ýþlem Fonksiyonlarý ---
	UFUNCTION(BlueprintCallable, Category = "Attributes")
	bool ApplyDamage(AActor* InstigatorActor, float DamageAmount);

	UFUNCTION(BlueprintCallable, Category = "Attributes")
	bool ApplyHeal(AActor* InstigatorActor, float HealAmount);

	UFUNCTION(BlueprintCallable, Category = "Attributes")
	bool ConsumeStamina(AActor* InstigatorActor, float Amount);

	UFUNCTION(BlueprintCallable, Category = "Attributes")
	bool RestoreStamina(AActor* InstigatorActor, float Amount);

	// --- MELEE SÝSTEMÝ (ORTAK HAFIZA) ---
	UPROPERTY()
	TArray<AActor*> HitActorsDuringAttack;

	void ResetMeleeHitList() { HitActorsDuringAttack.Empty(); }

	bool CanHitActor(AActor* TargetActor) const
	{
		return TargetActor != nullptr && !HitActorsDuringAttack.Contains(TargetActor);
	}

	void AddToMeleeHitList(AActor* TargetActor)
	{
		if (TargetActor) HitActorsDuringAttack.AddUnique(TargetActor);
	}

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnRep_Health(float OldHealth);

	UFUNCTION()
	void OnRep_Stamina(float OldStamina);

	// --- DEÐÝÞKENLER ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Replicated, Category = "Attributes", meta = (ClampMin = "0.0"))
	float MaxHealth = 100.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "Attributes", meta = (ClampMin = "0.0"))
	float Health = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Replicated, Category = "Attributes", meta = (ClampMin = "0.0"))
	float MaxStamina = 100.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_Stamina, Category = "Attributes", meta = (ClampMin = "0.0"))
	float Stamina = 100.f;

private:
	float Clamp01(float Value, float MinV, float MaxV) const;
};