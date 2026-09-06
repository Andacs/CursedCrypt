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
#include "CCEnemyCharacter.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"

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
	// TargetActor ALWAYS represents the Player. We never overwrite TargetActor.
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

	// 2. Synchronous pathfinding query for detour path to the player
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
	float ObstacleHealth = 100.0f;
	if (BlockingObstacle)
	{
		IsActorBreakable(BlockingObstacle, ObstacleHealth);

		const float DPS = FMath::Max(1.0f, DefaultEnemyDPS);
		const float TimeToBreak = ObstacleHealth / DPS;

		const float DistToObstacle = FVector::Dist(StartLoc, BlockingObstacle->GetActorLocation());
		const float DistObstacleToTarget = FVector::Dist(BlockingObstacle->GetActorLocation(), TargetLoc);
		const float DirectWalkTime = (DistToObstacle + DistObstacleToTarget) / WalkSpeed;

		BreakTime = DirectWalkTime + TimeToBreak;
	}

	// 4. Decision with Hysteresis (Stability Threshold)
	// BlockerBarricadeKey is set when breaking is preferred/required; cleared when detouring or path is open.
	UObject* CurrentBlockerObj = BlackboardComp->GetValueAsObject(BlockerBarricadeKey.SelectedKeyName);
	AActor* CurrentBlocker = Cast<AActor>(CurrentBlockerObj);

	auto ApplyDecision = [&](AActor* ChosenBlocker)
	{
		if (ChosenBlocker)
		{
			BlackboardComp->SetValueAsObject(BlockerBarricadeKey.SelectedKeyName, ChosenBlocker);
		}
		else
		{
			BlackboardComp->ClearValue(BlockerBarricadeKey.SelectedKeyName);
		}
	};

	// Proximity check: If the enemy is already in front of a blocking obstacle (within 250 units), prioritize breaking it!
	bool bInMeleeRangeOfObstacle = false;
	if (BlockingObstacle && IsValid(BlockingObstacle))
	{
		FVector ObsOrigin, ObsExtents;
		BlockingObstacle->GetActorBounds(true, ObsOrigin, ObsExtents);
		const FBox ObsBox(ObsOrigin - ObsExtents, ObsOrigin + ObsExtents);
		const FVector ClosestObsPt = ObsBox.GetClosestPointTo(StartLoc);
		const float DistToObs = FVector::Dist(StartLoc, ClosestObsPt);

		if (DistToObs <= 250.0f)
		{
			bInMeleeRangeOfObstacle = true;
		}
	}

	// Condition 1: No complete detour path exists OR enemy is already in front of the barricade
	if (!bHasCompletePath || bInMeleeRangeOfObstacle)
	{
		ApplyDecision(BlockingObstacle);
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
				ApplyDecision(nullptr);
			}
			else
			{
				ApplyDecision(BlockingObstacle);
			}
		}
		else
		{
			// AI is currently detouring towards player:
			// Stay detouring UNLESS Breaking is faster by more than TimeTolerance
			if (BreakTime < (DetourTime - TimeTolerance))
			{
				ApplyDecision(BlockingObstacle);
			}
			else
			{
				ApplyDecision(nullptr);
			}
		}
	}
	else
	{
		// Path is clear or no breakable obstacle exists -> Detour/direct to target
		ApplyDecision(nullptr);
	}

#if WITH_EDITOR
	if (bShowDebugDraw && GEngine)
	{
		FColor StatusColor = FColor::Green;
		FString StatusText = TEXT("DETOUR_TO_PLAYER");

		if (!bHasCompletePath)
		{
			StatusColor = FColor::Red;
			StatusText = FString::Printf(TEXT("FULL_BLOCK -> %s"),
				BlockingObstacle ? *FString::Printf(TEXT("ATTACK BARRICADE [%s] (HP: %.0f)"), *BlockingObstacle->GetName(), ObstacleHealth) : TEXT("NO BREAKABLE FOUND"));
		}
		else if (BlockingObstacle && BreakTime < (DetourTime - TimeTolerance))
		{
			StatusColor = FColor::Orange;
			StatusText = FString::Printf(TEXT("BREAK SHORTCUT [%s] (T_break: %.1fs < T_detour: %.1fs)"),
				*BlockingObstacle->GetName(), BreakTime, DetourTime);
		}
		else
		{
			StatusColor = FColor::Green;
			StatusText = FString::Printf(TEXT("DETOUR TO PLAYER (T_detour: %.1fs <= T_break: %.1fs)"),
				DetourTime, (BreakTime < FLT_MAX) ? BreakTime : -1.0f);
		}

		GEngine->AddOnScreenDebugMessage(
			102938, 0.6f, StatusColor,
			FString::Printf(TEXT("[CC AI PathCost] %s"), *StatusText)
		);
	}
#endif
}

AActor* UBTService_CCCheckPath::FindBlockingBreakable(APawn* ControlledPawn, AActor* TargetActor, const FPathFindingResult& PathResult) const
{
	UWorld* World = ControlledPawn->GetWorld();
	if (!World) return nullptr;

	TArray<AActor*> Candidates;
	FCollisionQueryParams CollisionParams(SCENE_QUERY_STAT(CCCheckPath), false);
	CollisionParams.AddIgnoredActor(ControlledPawn);
	CollisionParams.AddIgnoredActor(TargetActor);

	FCollisionObjectQueryParams ObjectQueryParams(FCollisionObjectQueryParams::AllObjects);

	const FVector PawnLoc = ControlledPawn->GetActorLocation();
	const FVector TargetLoc = TargetActor->GetActorLocation();

	// 1. Search around TargetActor (catches barricades enclosing or guarding the player)
	TArray<FOverlapResult> TargetOverlaps;
	World->OverlapMultiByObjectType(
		TargetOverlaps, TargetLoc, FQuat::Identity,
		ObjectQueryParams, FCollisionShape::MakeSphere(ObstacleSearchRadius), CollisionParams
	);
	for (const FOverlapResult& Overlap : TargetOverlaps)
	{
		if (Overlap.GetActor() && !Candidates.Contains(Overlap.GetActor()))
		{
			Candidates.Add(Overlap.GetActor());
		}
	}

	// 2. Search around ControlledPawn (catches barricades right in front of the enemy)
	TArray<FOverlapResult> PawnOverlaps;
	World->OverlapMultiByObjectType(
		PawnOverlaps, PawnLoc, FQuat::Identity,
		ObjectQueryParams, FCollisionShape::MakeSphere(ObstacleSearchRadius), CollisionParams
	);
	for (const FOverlapResult& Overlap : PawnOverlaps)
	{
		if (Overlap.GetActor() && !Candidates.Contains(Overlap.GetActor()))
		{
			Candidates.Add(Overlap.GetActor());
		}
	}

	// 3. Forward sweep in front of the enemy (catches barricades directly in face within 350cm)
	const FVector PawnForward = ControlledPawn->GetActorForwardVector();
	const FVector PawnForwardEnd = PawnLoc + (PawnForward * 350.0f);
	TArray<FHitResult> FaceHits;
	World->SweepMultiByObjectType(
		FaceHits, PawnLoc, PawnForwardEnd, FQuat::Identity,
		ObjectQueryParams, FCollisionShape::MakeSphere(100.0f), CollisionParams
	);
	for (const FHitResult& Hit : FaceHits)
	{
		if (Hit.GetActor() && !Candidates.Contains(Hit.GetActor()))
		{
			Candidates.Add(Hit.GetActor());
		}
	}

	// 4. Search around terminal point of partial path (if any)
	if (PathResult.Path.IsValid() && PathResult.Path->GetPathPoints().Num() > 0)
	{
		const TArray<FNavPathPoint>& PathPoints = PathResult.Path->GetPathPoints();
		const FVector PathEnd = PathPoints.Last().Location;
		const FVector DirToTarget = (TargetLoc - PathEnd).GetSafeNormal();
		const FVector SweepEnd = PathEnd + (DirToTarget * 400.0f);

		TArray<FHitResult> SweepHits;
		World->SweepMultiByObjectType(
			SweepHits, PathEnd, SweepEnd, FQuat::Identity,
			ObjectQueryParams, FCollisionShape::MakeSphere(ObstacleDetectionRadius), CollisionParams
		);
		for (const FHitResult& Hit : SweepHits)
		{
			if (Hit.GetActor() && !Candidates.Contains(Hit.GetActor()))
			{
				Candidates.Add(Hit.GetActor());
			}
		}

		TArray<FOverlapResult> PathEndOverlaps;
		World->OverlapMultiByObjectType(
			PathEndOverlaps, PathEnd, FQuat::Identity,
			ObjectQueryParams, FCollisionShape::MakeSphere(ObstacleDetectionRadius * 1.5f), CollisionParams
		);
		for (const FOverlapResult& Overlap : PathEndOverlaps)
		{
			if (Overlap.GetActor() && !Candidates.Contains(Overlap.GetActor()))
			{
				Candidates.Add(Overlap.GetActor());
			}
		}
	}

	// 5. Direct corridor sweep between Pawn and Target
	const FVector RayDir = (TargetLoc - PawnLoc).GetSafeNormal();
	const float MaxDist = FMath::Min(FVector::Dist(PawnLoc, TargetLoc), MaxObstacleCheckDistance);
	const FVector RayEnd = PawnLoc + (RayDir * MaxDist);

	TArray<FHitResult> CorridorHits;
	World->SweepMultiByObjectType(
		CorridorHits, PawnLoc, RayEnd, FQuat::Identity,
		ObjectQueryParams, FCollisionShape::MakeSphere(ObstacleDetectionRadius), CollisionParams
	);
	for (const FHitResult& Hit : CorridorHits)
	{
		if (Hit.GetActor() && !Candidates.Contains(Hit.GetActor()))
		{
			Candidates.Add(Hit.GetActor());
		}
	}

	// 6. Filter for alive breakable obstacles and pick the best obstacle
	AActor* BestObstacle = nullptr;
	float BestCost = FLT_MAX;

	for (AActor* Candidate : Candidates)
	{
		float ObstacleHP = 0.0f;
		if (IsActorBreakable(Candidate, ObstacleHP))
		{
			FVector ObsOrigin, ObsExtents;
			Candidate->GetActorBounds(true, ObsOrigin, ObsExtents);
			const FBox CandBox(ObsOrigin - ObsExtents, ObsOrigin + ObsExtents);
			const FVector ClosestPt = CandBox.GetClosestPointTo(PawnLoc);
			const float DistFromPawn = FVector::Dist(PawnLoc, ClosestPt);
			const float DistToTarget = FVector::Dist(ClosestPt, TargetLoc);

			// Wall Check: Only skip if candidate is walled off behind an impenetrable solid wall from the Pawn
			if (DistFromPawn > 250.0f)
			{
				FHitResult Hit;
				FCollisionQueryParams Params(SCENE_QUERY_STAT(CheckCandidateLOS), false);
				Params.AddIgnoredActor(ControlledPawn);
				Params.AddIgnoredActor(Candidate);

				const FVector PawnEye = PawnLoc + FVector(0.0f, 0.0f, 50.0f);
				const FVector CandCenter = ObsOrigin;

				if (World->LineTraceSingleByChannel(Hit, PawnEye, CandCenter, ECC_Visibility, Params))
				{
					AActor* HitActor = Hit.GetActor();
					float DummyHP = 0.0f;
					if (HitActor && !IsActorBreakable(HitActor, DummyHP))
					{
						// Separated by an unbreakable solid wall, skip this candidate
						continue;
					}
				}
			}

			// If obstacle is right in front of the enemy, drastically lower cost so it is picked first
			float Cost = DistFromPawn + (DistToTarget * 0.5f);
			if (DistFromPawn <= 250.0f)
			{
				Cost = DistFromPawn * 0.05f; // Absolute priority!
			}

			if (Cost < BestCost)
			{
				BestCost = Cost;
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

	// Critical: Pawns (Player or Enemy characters) are NEVER breakable obstacles!
	if (CandidateActor->IsA<APawn>()) return false;

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

	// 3. Fallback: Check actor tags or class name
	if (CandidateActor->ActorHasTag(TEXT("Barricade")) || CandidateActor->ActorHasTag(TEXT("Breakable")) ||
		CandidateActor->GetName().Contains(TEXT("Barricade")))
	{
		OutHealth = 100.0f;
		return true;
	}

	return false;
}
