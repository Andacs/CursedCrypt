#include "AnimNotifyState_MeleeTrace.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "AttributeComponent.h"
#include "GameFramework/Actor.h"

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

    // YAPAY ZEKA ÝÇÝN LÝSTE SIFIRLAMA:
    if (AActor* OwnerActor = MeshComp->GetOwner())
    {
        if (APawn* PawnOwner = Cast<APawn>(OwnerActor))
        {
            // Eðer saldýran kiþi Yapay Zeka (AI) ise listeyi burada sýfýrla.
            // (Oyuncunun listesi Server_Attack içinde sýfýrlanýr ki Blend bug'ý olmasýn)
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
    // Saldýran kiþinin Attribute'u yoksa saldýramaz
    UAttributeComponent* OwnerAttr = OwnerActor->FindComponentByClass<UAttributeComponent>();
    if (!OwnerAttr) return false;

    FVector StartLocation = MeshComp->GetSocketLocation(StartSocketName);
    FVector EndLocation = (StartSocketName == EndSocketName) ?
        (StartLocation + (MeshComp->GetRightVector() * 60.f)) : MeshComp->GetSocketLocation(EndSocketName);

    TArray<AActor*> ActorsToIgnore;
    ActorsToIgnore.Add(OwnerActor); // Kendimize vurmayalým

    TArray<FHitResult> OutHits;

    bool bHit = UKismetSystemLibrary::SphereTraceMulti(
        MeshComp->GetWorld(), StartLocation, EndLocation, Radius,
        UEngineTypes::ConvertToTraceType(ECC_Pawn), false,
        ActorsToIgnore, EDrawDebugTrace::None, OutHits, true
    );

    if (bHit)
    {
        for (const FHitResult& Hit : OutHits)
        {
            AActor* HitActor = Hit.GetActor();
            if (!HitActor) continue;

            // Dost Ateþi Korumasý
            if (OwnerActor->GetClass() == HitActor->GetClass()) continue;

            // Düþmana bu saldýrýda henüz VURMADIYSAK:
            if (OwnerAttr->CanHitActor(HitActor))
            {
                // 1. Listeye Ekle (Multi-hit engellenir)
                OwnerAttr->AddToMeleeHitList(HitActor);

                // 2. HASAR AYRIMI (Çifte hasarý ve tek atmayý engeller)
                if (UAttributeComponent* HitAttr = HitActor->FindComponentByClass<UAttributeComponent>())
                {
                    // Hedefin bizim can sistemimiz varsa SADECE onu kullan (AI ve Oyuncular)
                    HitAttr->ApplyDamage(OwnerActor, Damage);
                }
                else
                {
                    // Hedefin bizim sistemimiz YOKSA standart Blueprint hasarýný vur (Çömlekler, Sandýklar vb.)
                    UGameplayStatics::ApplyDamage(HitActor, Damage, OwnerActor->GetInstigatorController(), OwnerActor, nullptr);
                }
            }
        }
    }
    return bHit;
}