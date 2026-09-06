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

    // Line of sight check: do not attack through solid walls
    if (UWorld* World = GetWorld())
    {
        FHitResult Hit;
        FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(EnemyAttackLOS), false);
        TraceParams.AddIgnoredActor(this);
        TraceParams.AddIgnoredActor(TargetActor);

        const FVector EyeLoc = GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);
        const FVector TargetCenter = TargetActor->GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);

        if (World->LineTraceSingleByChannel(Hit, EyeLoc, TargetCenter, ECC_Visibility, TraceParams))
        {
            // Blocked by a solid wall/geometry between attacker and target
            return false;
        }
    }

    // Play attack montage.
    if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
    {
        AnimInstance->Montage_Play(AttackMontage);
        return true;
    }

    return false;
}