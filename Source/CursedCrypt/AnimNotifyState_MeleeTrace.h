#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_MeleeTrace.generated.h"

UCLASS()
class CURSEDCRYPT_API UAnimNotifyState_MeleeTrace : public UAnimNotifyState
{
    GENERATED_BODY()

public:
    UAnimNotifyState_MeleeTrace();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace")
    float Radius = 30.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace")
    float Damage = 25.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace")
    FName StartSocketName = "hand_r";

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace")
    FName EndSocketName = "hand_r";

    virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
    virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;

private:
    bool DoSphereSweep(USkeletalMeshComponent* MeshComp, class AActor* OwnerActor);
};