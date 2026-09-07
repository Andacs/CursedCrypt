#include "AnimNotifyState_MeleeTrace.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "AttributeComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"

UAnimNotifyState_MeleeTrace::UAnimNotifyState_MeleeTrace()
{
    Radius = 30.f;
    Damage = 20.f;
    StartSocketName = "hand_r";
    EndSocketName = "hand_r";
}

void UAnimNotifyState_MeleeTrace::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

    // Reset hit list for AI attackers at the start of each attack.
    // (Player's hit list is reset inside Server_Attack to avoid blend bugs)
    if (AActor* OwnerActor = MeshComp->GetOwner())
    {
        if (APawn* PawnOwner = Cast<APawn>(OwnerActor))
        {
            if (!PawnOwner->IsPlayerControlled())
            {
                if (UAttributeComponent* Attr = OwnerActor->FindComponentByClass<UAttributeComponent>())
                {
                    Attr->ResetMeleeHitList();
                }
            }
        }
    }
}

void UAnimNotifyState_MeleeTrace::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

    if (!MeshComp || !MeshComp->GetOwner()) return;

    AActor* OwnerActor = MeshComp->GetOwner();

    if (OwnerActor->HasAuthority())
    {
        DoSphereSweep(MeshComp, OwnerActor);
    }
}

bool UAnimNotifyState_MeleeTrace::DoSphereSweep(USkeletalMeshComponent* MeshComp, AActor* OwnerActor)
{
    // Attacker must have an Attribute Component to perform an attack.
    UAttributeComponent* OwnerAttr = OwnerActor->FindComponentByClass<UAttributeComponent>();
    if (!OwnerAttr) return false;

    FVector StartLocation = MeshComp->GetSocketLocation(StartSocketName);
    FVector EndLocation = (StartSocketName == EndSocketName) ?
        (StartLocation + (MeshComp->GetRightVector() * 60.f)) : MeshComp->GetSocketLocation(EndSocketName);

    TArray<AActor*> ActorsToIgnore;
    ActorsToIgnore.Add(OwnerActor); // Do not hit ourselves.

    TArray<FHitResult> OutHits;

    bool bHit = UKismetSystemLibrary::SphereTraceMulti(
        MeshComp, StartLocation, EndLocation, Radius,
        UEngineTypes::ConvertToTraceType(ECC_Pawn), false,
        ActorsToIgnore, EDrawDebugTrace::None, OutHits, true
    );

    if (bHit)
    {
        UWorld* World = MeshComp->GetWorld();

        for (const FHitResult& Hit : OutHits)
        {
            AActor* HitActor = Hit.GetActor();
            if (!HitActor) continue;

            // Friendly fire protection: same class actors do not damage each other.
            if (OwnerActor->GetClass() == HitActor->GetClass()) continue;

            // Line of sight check between attacker and victim to prevent hitting through solid walls
            if (World)
            {
                FHitResult WallHit;
                FCollisionQueryParams WallParams(SCENE_QUERY_STAT(MeleeTraceWallLOS), false);
                WallParams.AddIgnoredActor(OwnerActor);
                WallParams.AddIgnoredActor(HitActor);

                FCollisionObjectQueryParams ObjParams;
                ObjParams.AddObjectTypesToQuery(ECC_WorldStatic);

                auto IsWall = [&](const FHitResult& H) -> bool
                {
                    if (!H.bBlockingHit) return false;
                    AActor* A = H.GetActor();
                    if (!A) return true; // Level geometry
                    if (A == OwnerActor || A == HitActor) return false;
                    if (A->IsA<APawn>()) return false;
                    return (A->FindComponentByClass<UAttributeComponent>() == nullptr)
                        && !A->ActorHasTag(TEXT("Barricade"))
                        && !A->ActorHasTag(TEXT("Breakable"))
                        && !A->GetName().Contains(TEXT("Barricade"));
                };

                const FVector AttackerChest = OwnerActor->GetActorLocation() + FVector(0.0f, 0.0f, 30.0f);
                const FVector VictimChest = HitActor->GetActorLocation() + FVector(0.0f, 0.0f, 30.0f);

                if ((World->LineTraceSingleByObjectType(WallHit, AttackerChest, VictimChest, ObjParams, WallParams) && IsWall(WallHit)) ||
                    (World->LineTraceSingleByChannel(WallHit, AttackerChest, VictimChest, ECC_Visibility, WallParams) && IsWall(WallHit)))
                {
                    continue; // A solid wall is between attacker and victim -> NO damage!
                }
            }

            // If we have not hit this actor yet in this attack:
            if (OwnerAttr->CanHitActor(HitActor))
            {
                // 1. Add to hit list (prevents multi-hit from a single swing)
                OwnerAttr->AddToMeleeHitList(HitActor);

                // 2. Apply damage: prefer our AttributeComponent system if target has one,
                //    otherwise fall back to standard Blueprint damage (for pots, chests, etc.)
                if (UAttributeComponent* HitAttr = HitActor->FindComponentByClass<UAttributeComponent>())
                {
                    HitAttr->ApplyDamage(OwnerActor, Damage);
                }
                else
                {
                    UGameplayStatics::ApplyDamage(HitActor, Damage, OwnerActor->GetInstigatorController(), OwnerActor, nullptr);
                }
            }
        }
    }
    return bHit;
}