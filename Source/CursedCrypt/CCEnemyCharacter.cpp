#include "CCEnemyCharacter.h"
#include "AttributeComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"

ACCEnemyCharacter::ACCEnemyCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    // Create the attribute component for health/stamina tracking.
    Attributes = CreateDefaultSubobject<UAttributeComponent>(TEXT("Attributes"));

    // Allow AI to be hit by melee attacks via overlap on the Pawn channel.
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void ACCEnemyCharacter::BeginPlay()
{
    Super::BeginPlay();
}

bool ACCEnemyCharacter::TryAttack(AActor* TargetActor)
{
    if (!TargetActor || !AttackMontage || !Attributes) return false;

    // Skip if dead.
    if (!Attributes->IsAlive()) return false;

    // Range check.
    const float Dist = FVector::Dist(TargetActor->GetActorLocation(), GetActorLocation());
    if (Dist > AttackRange) return false;

    // Play attack montage.
    if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
    {
        AnimInstance->Montage_Play(AttackMontage);
        return true;
    }

    return false;
}