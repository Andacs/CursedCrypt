#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Services/BTService_BlackboardBase.h"
#include "BTService_CCCheckPath.generated.h"

struct FPathFindingResult;

/**
 * UBTService_CCCheckPath
 * 
 * Evaluates navigation cost and time-to-reach the target.
 * Compares Detour Time vs Break Time through obstacles (with hysteresis to prevent flicker).
 * Automatically detects breakable barricades (via AttributeComponent / Combat interface)
 * without hard class references.
 */
UCLASS()
class CURSEDCRYPT_API UBTService_CCCheckPath : public UBTService_BlackboardBase
{
	GENERATED_BODY()

public:
	UBTService_CCCheckPath();

protected:
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	// --- Blackboard Keys ---
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector BlockerBarricadeKey;

	// --- Time & Cost Settings ---
	/** Minimum time difference required to switch between detour and break decisions (prevents oscillation / jitter). */
	UPROPERTY(EditAnywhere, Category = "Cost Evaluation", meta = (ClampMin = "0.0"))
	float TimeTolerance = 2.0f;

	/** Estimated damage per second of the enemy against breakable obstacles. */
	UPROPERTY(EditAnywhere, Category = "Cost Evaluation", meta = (ClampMin = "1.0"))
	float DefaultEnemyDPS = 20.0f;

	// --- Detection Settings ---
	/** Sphere sweep radius used to detect blocking breakables along path endpoints and corridors. */
	UPROPERTY(EditAnywhere, Category = "Detection", meta = (ClampMin = "10.0"))
	float ObstacleDetectionRadius = 80.0f;

	/** Maximum search distance forward along the path to find a blocking breakable. */
	UPROPERTY(EditAnywhere, Category = "Detection", meta = (ClampMin = "100.0"))
	float MaxObstacleCheckDistance = 1500.0f;

	/** Enable visual debug lines and spheres during PIE. */
	UPROPERTY(EditAnywhere, Category = "Debug")
	bool bShowDebugDraw = false;

private:
	/** Finds the closest breakable actor obstructing the path to TargetActor. */
	AActor* FindBlockingBreakable(APawn* ControlledPawn, AActor* TargetActor, const FPathFindingResult& PathResult) const;

	/** Checks if candidate actor has an alive AttributeComponent or implements combat interface. */
	bool IsActorBreakable(AActor* CandidateActor, float& OutHealth) const;
};
