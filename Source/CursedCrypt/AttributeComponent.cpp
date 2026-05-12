#include "AttributeComponent.h"
#include "Net/UnrealNetwork.h"

UAttributeComponent::UAttributeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	MaxHealth = 100.0f;
	Health = MaxHealth;
	MaxStamina = 100.0f;
	Stamina = MaxStamina;
}

void UAttributeComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UAttributeComponent, Health);
	DOREPLIFETIME(UAttributeComponent, MaxHealth);
	DOREPLIFETIME(UAttributeComponent, Stamina);
	DOREPLIFETIME(UAttributeComponent, MaxStamina);
}

void UAttributeComponent::OnRep_Health(float OldHealth)
{
	float Delta = Health - OldHealth;
	OnHealthChanged.Broadcast(nullptr, this, Health, Delta);
}

void UAttributeComponent::OnRep_Stamina(float OldStamina)
{
	float Delta = Stamina - OldStamina;
	OnStaminaChanged.Broadcast(nullptr, this, Stamina, Delta);
}

void UAttributeComponent::BeginPlay()
{
	Super::BeginPlay();
	Health = FMath::Clamp(Health, 0.f, MaxHealth);
	Stamina = FMath::Clamp(Stamina, 0.f, MaxStamina);
}

float UAttributeComponent::Clamp01(float Value, float MinV, float MaxV) const
{
	return FMath::Clamp(Value, MinV, MaxV);
}

bool UAttributeComponent::ApplyDamage(AActor* InstigatorActor, float DamageAmount)
{
	if (!GetOwner()->HasAuthority()) return false;

	if (DamageAmount <= 0.f || !IsAlive()) return false;

	const float OldHealth = Health;
	Health = Clamp01(Health - DamageAmount, 0.f, MaxHealth);
	const float Delta = Health - OldHealth;

	OnHealthChanged.Broadcast(InstigatorActor, this, Health, Delta);
	return true;
}

bool UAttributeComponent::ApplyHeal(AActor* InstigatorActor, float HealAmount)
{
	if (!GetOwner()->HasAuthority()) return false;

	if (HealAmount <= 0.f || !IsAlive()) return false;

	const float OldHealth = Health;
	Health = Clamp01(Health + HealAmount, 0.f, MaxHealth);
	const float Delta = Health - OldHealth;

	OnHealthChanged.Broadcast(InstigatorActor, this, Health, Delta);
	return true;
}

bool UAttributeComponent::ConsumeStamina(AActor* InstigatorActor, float Amount)
{
	if (!GetOwner()->HasAuthority()) return false;

	if (Amount <= 0.f || Stamina < Amount) return false;

	const float Old = Stamina;
	Stamina = Clamp01(Stamina - Amount, 0.f, MaxStamina);
	const float Delta = Stamina - Old;

	OnStaminaChanged.Broadcast(InstigatorActor, this, Stamina, Delta);
	return true;
}

bool UAttributeComponent::RestoreStamina(AActor* InstigatorActor, float Amount)
{
	if (!GetOwner()->HasAuthority()) return false;

	if (Amount <= 0.f) return false;

	const float Old = Stamina;
	Stamina = Clamp01(Stamina + Amount, 0.f, MaxStamina);
	const float Delta = Stamina - Old;

	OnStaminaChanged.Broadcast(InstigatorActor, this, Stamina, Delta);
	return true;
}