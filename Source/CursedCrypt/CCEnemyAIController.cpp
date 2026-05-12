#include "CCEnemyAIController.h"
#include "BehaviorTree/BehaviorTree.h"

ACCEnemyAIController::ACCEnemyAIController()
{
}

void ACCEnemyAIController::BeginPlay()
{
    Super::BeginPlay();
    RunEnemyBT();
}

void ACCEnemyAIController::RunEnemyBT()
{
    if (BehaviorTreeAsset)
    {
        RunBehaviorTree(BehaviorTreeAsset);
    }
}