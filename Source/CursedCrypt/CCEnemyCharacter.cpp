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
        FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(EnemyAttackLOS), false);
        TraceParams.AddIgnoredActor(this);
        TraceParams.AddIgnoredActor(TargetActor);

        FCollisionObjectQueryParams ObjectParams;
        ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
        ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
        ObjectParams.AddObjectTypesToQuery(ECC_PhysicsBody);

        auto IsHitSolidWall = [&](const FHitResult& InHit) -> bool
        {
            if (!InHit.bBlockingHit) return false;
            AActor* HitActor = InHit.GetActor();
            // If actor is null, this is level geometry (BSP / static geometry) -> solid wall!
            if (!HitActor) return true;
            if (HitActor == TargetActor || HitActor->GetAttachParentActor() == TargetActor) return false;
            if (HitActor->IsA<APawn>()) return false;

            const bool bIsBreakable = (HitActor->FindComponentByClass<UAttributeComponent>() != nullptr)
                || HitActor->ActorHasTag(TEXT("Barricade"))
                || HitActor->ActorHasTag(TEXT("Breakable"))
                || HitActor->GetName().Contains(TEXT("Barricade"));

            return !bIsBreakable;
        };

        // Check at multiple heights: Eye (+50), Chest (+15), and Pelvis/Low (-20)
        const float CheckHeights[] = { 50.0f, 15.0f, -20.0f };
        for (float H : CheckHeights)
        {
            const FVector Start = GetActorLocation() + FVector(0.0f, 0.0f, H);
            const FVector End = ClosestPoint + FVector(0.0f, 0.0f, H);

            FHitResult HitObj, HitChan;
            if (World->LineTraceSingleByObjectType(HitObj, Start, End, ObjectParams, TraceParams) && IsHitSolidWall(HitObj))
            {
                return false;
            }
            if (World->LineTraceSingleByChannel(HitChan, Start, End, ECC_Visibility, TraceParams) && IsHitSolidWall(HitChan))
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
        }
        return true;
    }

    return false;
}