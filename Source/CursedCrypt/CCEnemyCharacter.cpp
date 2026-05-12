#include "CCEnemyCharacter.h"
#include "AttributeComponent.h"
#include "Components/CapsuleComponent.h"

ACCEnemyCharacter::ACCEnemyCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    // Can bileþenini oluþtur
    Attributes = CreateDefaultSubobject<UAttributeComponent>(TEXT("Attributes"));

    // AI'nýn kýlýç darbesini almasý için Collision ayarý
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void ACCEnemyCharacter::BeginPlay()
{
    Super::BeginPlay();
}

bool ACCEnemyCharacter::TryAttack(AActor* TargetActor)
{
    if (!TargetActor || !AttackMontage || !Attributes) return false;

    // Can kontrolü
    if (!Attributes->IsAlive()) return false;

    // Mesafe Kontrolü
    const float Dist = FVector::Dist(TargetActor->GetActorLocation(), GetActorLocation());
    if (Dist > AttackRange) return false;

    // Animasyon oynat
    if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
    {
        AnimInstance->Montage_Play(AttackMontage);
        return true;
    }

    return false;
}