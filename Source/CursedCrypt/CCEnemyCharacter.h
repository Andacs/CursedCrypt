#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CCEnemyCharacter.generated.h"

class UAttributeComponent;
class UAnimMontage;

UCLASS()
class CURSEDCRYPT_API ACCEnemyCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    ACCEnemyCharacter();

protected:
    virtual void BeginPlay() override;

    // Attribute component (health, stamina, etc.)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes")
    UAttributeComponent* Attributes;

    // Combat settings.
    UPROPERTY(EditAnywhere, Category = "Combat")
    UAnimMontage* AttackMontage;

    UPROPERTY(EditAnywhere, Category = "Combat")
    float AttackRange = 200.f;

public:
    // Called by the AI to trigger a melee attack against the target actor.
    UFUNCTION(BlueprintCallable, Category = "Combat")
    bool TryAttack(AActor* TargetActor);
};