#include "BTService_CCCheckPath.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationSystem.h"
#include "Kismet/KismetSystemLibrary.h"
#include "DrawDebugHelpers.h"
#include "AttributeComponent.h"
#include "Engine/World.h"

UBTService_CCCheckPath::UBTService_CCCheckPath()
{
	NodeName = TEXT("CC Check Path & Cost Evaluator");
	Interval = 0.5f;
	RandomDeviation = 0.1f;
	bNotifyTick = true;

	// BlockerBarricadeKey filter for Actor-derived objects
	BlockerBarricadeKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_CCCheckPath, BlockerBarricadeKey), AActor::StaticClass());
}

void UBTService_CCCheckPath::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	UBlackboardData* BBAsset = GetBlackboardAsset();
	if (ensure(BBAsset))
	{
		BlockerBarricadeKey.ResolveSelectedKey(*BBAsset);
	}
}

void UBTService_CCCheckPath::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIC = OwnerComp.GetAIOwner();
	APawn* ControlledPawn = AIC ? AIC->GetPawn() : nullptr;
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();

	if (!ControlledPawn || !BlackboardComp) return;

	// 1. Retrieve TargetActor from Blackboard (using inherited BlackboardKey)
	AActor* TargetActor = Cast<AActor>(BlackboardComp->GetValueAsObject(GetSelectedBlackboardKey()));
	if (!TargetActor)
	{
		BlackboardComp->ClearValue(BlockerBarricadeKey.SelectedKeyName);
		return;
	}

	UWorld* World = ControlledPawn->GetWorld();
	if (!World) return;

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	if (!NavSys) return;

	// Retrieve AI walk speed (fallback to 400 uu/s)
	float WalkSpeed = 400.0f;
	if (ACharacter* Char = Cast<ACharacter>(ControlledPawn))
	{
		if (UCharacterMovementComponent* MoveComp = Char->GetCharacterMovement())
		{
			WalkSpeed = FMath::Max(100.0f, MoveComp->MaxWalkSpeed);
		}
	}

	const FVector StartLoc = ControlledPawn->GetActorLocation();
	const FVector TargetLoc = TargetActor->GetActorLocation();

	// 2. Synchronous pathfinding query for detour path
	FPathFindingQuery Query(ControlledPawn, *NavSys->GetDefaultNavDataInstance(), StartLoc, TargetLoc);
	FPathFindingResult PathResult = NavSys->FindPathSync(Query);

	const bool bHasCompletePath = PathResult.IsSuccessful() && PathResult.Path.IsValid() && !PathResult.Path->IsPartial();

	float DetourTime = FLT_MAX;
	if (bHasCompletePath)
	{
		const float PathLen = PathResult.Path->GetLength();
		DetourTime = PathLen / WalkSpeed;
	}

	// 3. Detect any blocking breakable obstacle along the corridor or partial path
	AActor* BlockingObstacle = FindBlockingBreakable(ControlledPawn, TargetActor, PathResult);

	float BreakTime = FLT_MAX;
	if (BlockingObstacle)
	{
		float ObstacleHealth = 100.0f;
		IsActorBreakable(BlockingObstacle, ObstacleHealth);

		const float DPS = FMath::Max(1.0f, DefaultEnemyDPS);
		const float TimeToBreak = ObstacleHealth / DPS;

		const float DistToObstacle = FVector::Dist(StartLoc, BlockingObstacle->GetActorLocation());
		const float DistObstacleToTarget = FVector::Dist(BlockingObstacle->GetActorLocation(), TargetLoc);
		const float DirectWalkTime = (DistToObstacle + DistObstacleToTarget) / WalkSpeed;

		BreakTime = DirectWalkTime + TimeToBreak;
	}

	// 4. Decision with Hysteresis (Stability Threshold)
	UObject* CurrentBlockerObj = BlackboardComp->GetValueAsObject(BlockerBarricadeKey.SelectedKeyName);
	AActor* CurrentBlocker = Cast<AActor>(CurrentBlockerObj);

	// Condition 1: No complete detour path exists (full block / dead-end)
	if (!bHasCompletePath)
	{
		if (BlockingObstacle)
		{
			BlackboardComp->SetValueAsObject(BlockerBarricadeKey.SelectedKeyName, BlockingObstacle);
		}
		else
		{
			BlackboardComp->ClearValue(BlockerBarricadeKey.SelectedKeyName);
		}
		return;
	}

	// Both Detour and Break are possible -> Compare costs
	if (BlockingObstacle && BreakTime < FLT_MAX)
	{
		if (CurrentBlocker != nullptr)
		{
			// AI is already targeting the barricade:
			// Stay targeting the barricade UNLESS Detour is faster by more than TimeTolerance
			if (DetourTime < (BreakTime - TimeTolerance))
			{
				BlackboardComp->ClearValue(BlockerBarricadeKey.SelectedKeyName);
			}
			else
			{
				BlackboardComp->SetValueAsObject(BlockerBarricadeKey.SelectedKeyName, BlockingObstacle);
			}
		}
		else
		{
			// AI is currently detouring towards player:
			// Stay detouring UNLESS Breaking is faster by more than TimeTolerance
			if (BreakTime < (DetourTime - TimeTolerance))
			{
				BlackboardComp->SetValueAsObject(BlockerBarricadeKey.SelectedKeyName, BlockingObstacle);
			}
			else
			{
				BlackboardComp->ClearValue(BlockerBarricadeKey.SelectedKeyName);
			}
		}
	}
	else
	{
		// Path is clear or no breakable obstacle exists -> Detour/direct to target
		BlackboardComp->ClearValue(BlockerBarricadeKey.SelectedKeyName);
	}

#if WITH_EDITOR
	if (bShowDebugDraw)
	{
		GEngine->AddOnScreenDebugMessage(
			INDEX_NONE, 0.5f, FColor::Cyan,
			FString::Printf(TEXT("[CC PathCost] DetourTime: %.2fs | BreakTime: %.2fs | Blocker: %s"),
				DetourTime, BreakTime,
				BlockingObstacle ? *BlockingObstacle->GetName() : TEXT("None"))
		);
	}
#endif
}

AActor* UBTService_CCCheckPath::FindBlockingBreakable(APawn* ControlledPawn, AActor* TargetActor, const FPathFindingResult& PathResult) const
{
	UWorld* World = ControlledPawn->GetWorld();
	if (!World) return nullptr;

	TArray<AActor*> Candidates;
	TArray<AActor*> IgnoredActors;
	IgnoredActors.Add(ControlledPawn);
	IgnoredActors.Add(TargetActor);

	// A. Check around the terminal reachable point if NavPath exists
	if (PathResult.Path.IsValid() && PathResult.Path->GetPathPoints().Num() > 0)
	{
		const TArray<FNavPathPoint>& PathPoints = PathResult.Path->GetPathPoints();
		const FVector PathEnd = PathPoints.Last().Location;
		const FVector DirToTarget = (TargetActor->GetActorLocation() - PathEnd).GetSafeNormal();
		const FVector SweepEnd = PathEnd + (DirToTarget * 350.0f);

		TArray<FHitResult> SweepHits;
		UKismetSystemLibrary::SphereTraceMulti(
			World, PathEnd, SweepEnd, ObstacleDetectionRadius,
			UEngineTypes::ConvertToTraceType(ECC_WorldDynamic), false,
			IgnoredActors,
			bShowDebugDraw ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None,
			SweepHits, true
		);

		for (const FHitResult& Hit : SweepHits)
		{
			if (Hit.GetActor() && !Candidates.Contains(Hit.GetActor()))
			{
				Candidates.Add(Hit.GetActor());
			}
		}
	}

	// B. Direct corridor sweep from Pawn to Target (covers disconnected NavMesh islands)
	const FVector PawnLoc = ControlledPawn->GetActorLocation();
	const FVector TargetLoc = TargetActor->GetActorLocation();
	const FVector RayDir = (TargetLoc - PawnLoc).GetSafeNormal();
	const float MaxDist = FMath::Min(FVector::Dist(PawnLoc, TargetLoc), MaxObstacleCheckDistance);
	const FVector RayEnd = PawnLoc + (RayDir * MaxDist);

	TArray<FHitResult> CorridorHits;
	UKismetSystemLibrary::SphereTraceMulti(
		World, PawnLoc, RayEnd, ObstacleDetectionRadius,
		UEngineTypes::ConvertToTraceType(ECC_WorldDynamic), false,
		IgnoredActors,
		bShowDebugDraw ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None,
		CorridorHits, true
	);

	for (const FHitResult& Hit : CorridorHits)
	{
		if (Hit.GetActor() && !Candidates.Contains(Hit.GetActor()))
		{
			Candidates.Add(Hit.GetActor());
		}
	}

	// Also trace WorldStatic in case barricade or door uses WorldStatic
	TArray<FHitResult> StaticHits;
	UKismetSystemLibrary::SphereTraceMulti(
		World, PawnLoc, RayEnd, ObstacleDetectionRadius,
		UEngineTypes::ConvertToTraceType(ECC_WorldStatic), false,
		IgnoredActors,
		bShowDebugDraw ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None,
		StaticHits, true
	);

	for (const FHitResult& Hit : StaticHits)
	{
		if (Hit.GetActor() && !Candidates.Contains(Hit.GetActor()))
		{
			Candidates.Add(Hit.GetActor());
		}
	}

	// C. Find the closest alive breakable obstacle to ControlledPawn
	AActor* BestObstacle = nullptr;
	float ClosestDistSq = FLT_MAX;

	for (AActor* Candidate : Candidates)
	{
		float ObstacleHP = 0.0f;
		if (IsActorBreakable(Candidate, ObstacleHP))
		{
			const float DistSq = FVector::DistSquared(PawnLoc, Candidate->GetActorLocation());
			if (DistSq < ClosestDistSq)
			{
				ClosestDistSq = DistSq;
				BestObstacle = Candidate;
			}
		}
	}

	return BestObstacle;
}

bool UBTService_CCCheckPath::IsActorBreakable(AActor* CandidateActor, float& OutHealth) const
{
	OutHealth = 0.0f;
	if (!IsValid(CandidateActor)) return false;

	// 1. Prefer AttributeComponent (health system)
	if (UAttributeComponent* Attr = CandidateActor->FindComponentByClass<UAttributeComponent>())
	{
		if (Attr->IsAlive())
		{
			OutHealth = Attr->GetHealth();
			return true;
		}
		return false; // Actor is dead / destroyed
	}

	// 2. Fallback: Check BPI_Combat interface function "TakeHit"
	if (CandidateActor->FindFunction(TEXT("TakeHit")))
	{
		OutHealth = 100.0f;
		return true;
	}

	return false;
}
