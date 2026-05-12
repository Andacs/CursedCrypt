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

    // Hocanýn istediði Attribute bileþeni
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attributes")
    UAttributeComponent* Attributes;

    // Saldýrý Ayarlarý
    UPROPERTY(EditAnywhere, Category = "Combat")
    UAnimMontage* AttackMontage;

    UPROPERTY(EditAnywhere, Category = "Combat")
    float AttackRange = 200.f;

public:
    // Yapay zekanýn saldýrý yapmasý için çaðrýlan fonksiyon
    UFUNCTION(BlueprintCallable, Category = "Combat")
    bool TryAttack(AActor* TargetActor);
};