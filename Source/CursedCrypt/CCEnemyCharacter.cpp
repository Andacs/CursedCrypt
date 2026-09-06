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

    // Range check using closest point on target bounding box (handles large barricades/tables)
    FVector Origin, Extents;
    TargetActor->GetActorBounds(true, Origin, Extents);
    const FBox TargetBox(Origin - Extents, Origin + Extents);
    const FVector ClosestPoint = TargetBox.GetClosestPointTo(GetActorLocation());
    const float DistToSurface = FVector::Dist(GetActorLocation(), ClosestPoint);
    const float DistToCenter = FVector::Dist(TargetActor->GetActorLocation(), GetActorLocation());

    const float EffectiveRange = FMath::Max(AttackRange, 250.0f);
    if (DistToSurface > EffectiveRange && DistToCenter > (EffectiveRange + Extents.GetMax()))
    {
        return false;
    }

    // Line of sight check: do not attack through solid unbreakable walls
    if (UWorld* World = GetWorld())
    {
        FHitResult Hit;
        FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(EnemyAttackLOS), false);
        TraceParams.AddIgnoredActor(this);
        TraceParams.AddIgnoredActor(TargetActor);

        const FVector EyeLoc = GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);
        const FVector TraceEnd = ClosestPoint + FVector(0.0f, 0.0f, 30.0f);

        if (World->LineTraceSingleByChannel(Hit, EyeLoc, TraceEnd, ECC_Visibility, TraceParams))
        {
            AActor* HitActor = Hit.GetActor();
            if (HitActor && !HitActor->ActorHasTag(TEXT("Barricade")) && !HitActor->ActorHasTag(TEXT("Breakable")) && !HitActor->GetName().Contains(TEXT("Barricade")))
            {
                return false;
            }
        }
    }

    // Play attack montage.
    if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
    {
        if (!AnimInstance->Montage_IsPlaying(AttackMontage))
        {
            AnimInstance->Montage_Play(AttackMontage);
            return true;
        }
    }

    return false;
}