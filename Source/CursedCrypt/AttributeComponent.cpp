#include "AttributeComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
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

	// Solid wall protection: if damage is dealt by an external actor (melee/ranged),
	// ensure there is no solid unbreakable wall between InstigatorActor and this victim actor.
	if (InstigatorActor && InstigatorActor != GetOwner())
	{
		if (UWorld* World = GetWorld())
		{
			FHitResult Hit;
			FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(DamageWallLOS), false);
			TraceParams.AddIgnoredActor(InstigatorActor);
			TraceParams.AddIgnoredActor(GetOwner());

			FCollisionObjectQueryParams ObjParams;
			ObjParams.AddObjectTypesToQuery(ECC_WorldStatic);

			auto IsSolidWall = [&](const FHitResult& InHit) -> bool
			{
				if (!InHit.bBlockingHit) return false;
				AActor* HitActor = InHit.GetActor();
				if (!HitActor) return true; // Level geometry
				if (HitActor == InstigatorActor || HitActor == GetOwner()) return false;
				if (HitActor->IsA<APawn>()) return false;

				const bool bIsBreakable = (HitActor->FindComponentByClass<UAttributeComponent>() != nullptr)
					|| HitActor->ActorHasTag(TEXT("Barricade"))
					|| HitActor->ActorHasTag(TEXT("Breakable"))
					|| HitActor->GetName().Contains(TEXT("Barricade"));

				return !bIsBreakable;
			};

			const FVector StartLoc = InstigatorActor->GetActorLocation() + FVector(0.0f, 0.0f, 20.0f);
			const FVector EndLoc = GetOwner()->GetActorLocation() + FVector(0.0f, 0.0f, 20.0f);

			if ((World->LineTraceSingleByObjectType(Hit, StartLoc, EndLoc, ObjParams, TraceParams) && IsSolidWall(Hit)) ||
				(World->LineTraceSingleByChannel(Hit, StartLoc, EndLoc, ECC_Visibility, TraceParams) && IsSolidWall(Hit)))
			{
				return false; // Solid wall blocked damage!
			}
		}
	}

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

void UAttributeComponent::SetHealth(float NewHealth)
{
	// Only server can authoritatively set health; clients will receive via OnRep_Health
	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	const float OldHealth = Health;
	Health = FMath::Clamp(NewHealth, 0.f, MaxHealth);
	const float Delta = Health - OldHealth;

	if (FMath::IsNearlyZero(Delta))
	{
		return;
	}

	// Broadcast on server so server-side listeners (HUD on listen-server) update.
	// Clients update via OnRep_Health automatically.
	OnHealthChanged.Broadcast(nullptr, this, Health, Delta);
}