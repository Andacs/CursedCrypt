#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "CCEnemyAIController.generated.h"

class UBehaviorTree;

UCLASS()
class CURSEDCRYPT_API ACCEnemyAIController : public AAIController
{
    GENERATED_BODY()

public:
    ACCEnemyAIController();

protected:
    virtual void BeginPlay() override;

    // Unreal içinden BT dosyasýný seçebilmen için
    UPROPERTY(EditDefaultsOnly, Category = "AI")
    UBehaviorTree* BehaviorTreeAsset;

public:
    UFUNCTION(BlueprintCallable, Category = "AI")
    void RunEnemyBT();
};