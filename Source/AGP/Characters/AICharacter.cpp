// Fill out your copyright notice in the Description page of Project Settings.

#include "AICharacter.h"
#include "HealthComponent.h"
#include "AGP/Pathfinding/PathfindingSubsystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "SquadSubsystem.h"
#include "Perception/PawnSensingComponent.h"

AAICharacter::AAICharacter(): PathfindingSubsystem(nullptr), SquadSubsystem(nullptr), SquadLeader(nullptr)
{
	PrimaryActorTick.bCanEverTick = true;

	PawnSensingComponent = CreateDefaultSubobject<UPawnSensingComponent>("Pawn Sensing Component");
}

ETeam AAICharacter::GetTeam()
{
	return AITeam;
}

void AAICharacter::BeginPlay()
{
	Super::BeginPlay();
	PathfindingSubsystem = GetWorld()->GetSubsystem<UPathfindingSubsystem>();
	SquadSubsystem = GetWorld()->GetSubsystem<USquadSubsystem>();
	CurrentPath = PathfindingSubsystem->GetRandomPath(GetActorLocation());

	if (PawnSensingComponent)
	{
		PawnSensingComponent->OnSeePawn.AddDynamic(this, &AAICharacter::OnSensedPawn);
	}

	EquipWeapon(bHasWeapon, DefaultWeaponStats);
}

void AAICharacter::TickFollowLeader()
{
	if(!CurrentPath.IsEmpty())
	{
		CurrentPath.Empty();
	}
	if (!SquadLeader || SquadMembers.IsEmpty()) return;

	int32 Index = SquadLeader->SquadMembers.Find(this);
	FVector FormationOffset = GetCircleFormationOffset(Index, SquadMembers.Num());
	FVector TargetLocation = SquadLeader->GetActorLocation() + FormationOffset;
    
	FVector MovementDirection = TargetLocation - GetActorLocation();
	float DistanceToTarget = MovementDirection.Size();

	float MinDistanceThreshold = 10.0f; 
	if (DistanceToTarget > MinDistanceThreshold)
	{
		MovementDirection.Normalize();
		AddMovementInput(MovementDirection);
	}

	if(SensedCharacter)
	{
		if (HasWeapon())
		{
			// Check if the weapon's magazine is empty and reload if necessary
			if (WeaponComponent->IsMagazineEmpty())
			{
				Reload();
			}
			Fire(SensedCharacter->GetActorLocation());
		}
	}
}

void AAICharacter::TickPatrol()
{
	if (CurrentPath.IsEmpty())
	{
		CurrentPath = PathfindingSubsystem->GetRandomPath(GetActorLocation());
	}
	MoveAlongPath();
}

void AAICharacter::TickEngage()
{
	if (!SensedCharacter || !IsValid(SensedCharacter))
	{
		CurrentState = EAIState::Patrol;
		return;
	}

	// Define safe distance range
	const float MinDistance = 300.0f; // Minimum distance to maintain from the target
	const float MaxDistance = 800.0f; // Maximum distance to maintain from the target

	FVector ToTarget = SensedCharacter->GetActorLocation() - GetActorLocation();
	float DistanceToTarget = ToTarget.Size();

	// Determine the movement direction
	FVector MovementDirection;
	if (DistanceToTarget > MaxDistance)
	{
		// Too far away, move closer
		MovementDirection = ToTarget;
	}
	else if (DistanceToTarget < MinDistance)
	{
		// Too close, move away
		MovementDirection = -ToTarget;
	}
	else
	{
		// Oscillate between advancing and retreating when within the safe range
		MovementDirection = (FMath::Sin(GetWorld()->GetTimeSeconds()) > 0) ? ToTarget : -ToTarget;
	}

	// Normalize movement direction
	if (!MovementDirection.IsZero())
	{
		MovementDirection.Normalize();
		AddMovementInput(MovementDirection);
	}

	// Weapon logic
	if (HasWeapon())
	{
		if (WeaponComponent->IsMagazineEmpty())
		{
			Reload();
		}
		Fire(SensedCharacter->GetActorLocation());
	}

	// Update path if needed
	if (CurrentPath.Num() > 0)
	{
		CurrentPath.Pop();    
	}
}

void AAICharacter::TickEvade()
{
	// Find the player and return if it can't find it.
	if (SensedCharacter)
	{
		bIsRunningFromEnemy = true;
		if (CurrentPath.IsEmpty())
		{
			CurrentPath = PathfindingSubsystem->GetPathAway(GetActorLocation(), SensedCharacter->GetActorLocation());
		}
	}else if(!SensedCharacter)
	{
		if (bIsRunningFromEnemy)
		{
			CurrentPath.Pop();
			bIsRunningFromEnemy = false;
		}
		if (CurrentPath.IsEmpty())
		{
			CurrentPath = PathfindingSubsystem->GetRandomPath(GetActorLocation());
		}
	}
	
	MoveAlongPath();
}

void AAICharacter::TickCover()
{
	//TODO DURING ASSESSMENT 4
	CurrentPath.Empty();

	FVector CoverVector = PathfindingSubsystem->FindCover(GetActorLocation(), FName("RockObstacle"));

	CurrentPath.Add(CoverVector);
}

EMoveState AAICharacter::GetMoveState()
{
	return MovementState;
}

bool AAICharacter::GetCrouchState()
{
	return bIsCrouching;
}

void AAICharacter::OnSensedPawn(APawn* SensedActor)
{
	AAICharacter* PotentialEnemy = Cast<AAICharacter>(SensedActor);
	if (PotentialEnemy)
	{
		// Check if the sensed character is on a different team
		if (PotentialEnemy->GetTeam() != GetTeam())
		{
			// Unbind the OnDestroyed event of the previously sensed character
			if (SensedCharacter)
			{
				SensedCharacter->OnDestroyed.RemoveDynamic(this, &AAICharacter::OnSensedCharacterDestroyed);
			}

			// Set the new sensed character
			SensedCharacter = PotentialEnemy;

			// Bind to the OnDestroyed event to clear the reference when the character is destroyed
			SensedCharacter->OnDestroyed.AddDynamic(this, &AAICharacter::OnSensedCharacterDestroyed);
            
			SquadSubsystem->SuggestTargetToLeader(this, PotentialEnemy);
		}
	}
}

void AAICharacter::SenseEnemy()
{
	if (!SensedCharacter) return;
	if (PawnSensingComponent)
	{
		if (!PawnSensingComponent->HasLineOfSightTo(SensedCharacter))
		{
			SensedCharacter = nullptr;
		}
	}
}

void AAICharacter::OnSensedCharacterDestroyed(AActor* DestroyedActor)
{
	if (SensedCharacter == DestroyedActor)
	{
		// Clear the reference to the sensed character
		SensedCharacter = nullptr;
	}
}


void AAICharacter::CalculateNextMoveState()
{
	//UE_LOG(LogTemp, Warning, TEXT("Calculating Next Move state"));
	if(CurrentPath.Num() > 2)
	{
		TargetNode = PathfindingSubsystem->GetNodeFromLocation(CurrentPath[CurrentPath.Num()-1]);
		NextNode = PathfindingSubsystem->GetNodeFromLocation(CurrentPath[CurrentPath.Num()-2]);

		if(TargetNode)
		{
			TargetNodeType = TargetNode->NodeType;
		}

		if(NextNode)
		{
			NextNodeType = NextNode->NodeType;
		}
	}else
	{
		CurrentPath.Empty();
		return;
	}

	if(DelayedMoveChange)
	{
		NextMoveState = EMoveState::FINISHCLIMB;
		bNextMoveCanBeSet = false;
		DelayedMoveChange = false;
		return;
	}
	
	//Calculate next move state based on next two nodes
	if(NextNodeType == ENavigationNodeType::WALKING)
	{
		NextMoveState = EMoveState::RUNNING;
	}else if(TargetNodeType == ENavigationNodeType::CLIMBINGUP &&
		NextNodeType == ENavigationNodeType::CLIMBINGDOWN)
	{
		NextMoveState = EMoveState::CLIMBINGUP;
		CurrentPath[CurrentPath.Num() - 2] = CurrentPath[CurrentPath.Num() - 2] + FVector(0,0,200);
		DelayedMoveChange = true;
	}else if(TargetNodeType == ENavigationNodeType::CLIMBINGDOWN &&
		NextNodeType == ENavigationNodeType::CLIMBINGUP)
	{
		NextMoveState = EMoveState::CLIMBINGDOWN;
	}else if(NextNodeType == ENavigationNodeType::CRAWLING)
	{
		NextMoveState = EMoveState::CRAWLING;
	}
	
	bNextMoveCanBeSet = false;
	//UE_LOG(LogTemp, Warning, TEXT("Next Move state calculated"));
}

void AAICharacter::UpdateMoveState()
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent) return;

	switch (MovementState)
	{
	case EMoveState::WALKING:
		bIsCrouching = false;
		if(!MovementComponent->IsFalling()){
			MovementComponent->MovementMode = MOVE_Walking;
		}
		MovementComponent->MaxWalkSpeed = 300.0f;
		break;
	case EMoveState::RUNNING:
		bIsCrouching = false;
		if(!MovementComponent->IsFalling()){
			MovementComponent->MovementMode = MOVE_Walking;
		}
		MovementComponent->MaxWalkSpeed = 600.0f;
		break;
	case EMoveState::CLIMBINGUP:
		bIsCrouching = false;
		MovementComponent->MovementMode = MOVE_Flying;
		MovementComponent->MaxFlySpeed = 100.0f;
			break;
	case EMoveState::CLIMBINGDOWN:
		bIsCrouching = false;
		MovementComponent->MovementMode = MOVE_Flying;
		MovementComponent->MaxFlySpeed = 100.0f;
			break;
	case EMoveState::CRAWLING:
		bIsCrouching = true;
		if(!MovementComponent->IsFalling()){
			MovementComponent->MovementMode = MOVE_Walking;
		}
		MovementComponent->MaxWalkSpeed = 600.0f;
		break;
	case EMoveState::FINISHCLIMB:
		bIsCrouching = false;
		MovementComponent->MovementMode = MOVE_Flying;
		MovementComponent->MaxFlySpeed = 600.0f;
		break;
	}
}

void AAICharacter::UpdateState()
{
	SenseEnemy();
	
	if(SquadLeader != this && SquadMembers.Num() > 0 && SquadMembers.Contains(SquadLeader))
	{
		CurrentState = EAIState::Follow;
	}
	
	switch (CurrentState)
	{
	case EAIState::Patrol:
		TickPatrol();
		break;
		
	case EAIState::Follow:
		if (ConfidenceLevel > 60)
		{
			CurrentState = EAIState::Engage;
		}
		TickFollowLeader();
		break;

	case EAIState::Engage:
		if (HealthComponent->GetCurrentHealthPercentage() < 0.5f)
		{
			CurrentState = EAIState::Evade;
			TickEvade();
			break;
		}
		TickEngage();
		break;

	case EAIState::Evade:
		TickEvade();
		break;

	case EAIState::Cover:
		TickCover();
		break;

	default:
		break;
	}
}

bool AAICharacter::IsLeader()
{
	if(SquadLeader == this)
	{
		
		return true;
	}
	return false;
}

void AAICharacter::MoveAlongPath()
{
	if (CurrentPath.IsEmpty()) return;
	FVector MovementDirection = CurrentPath[CurrentPath.Num()-1] - GetActorLocation();
	MovementDirection.Normalize();
	AddMovementInput(MovementDirection);
	if (FVector::Distance(GetActorLocation(), CurrentPath[CurrentPath.Num() - 1]) < PathfindingError)
	{
		CurrentPath.Pop();
		MovementState = NextMoveState;
		bNextMoveCanBeSet = true;
		//UE_LOG(LogTemp, Warning, TEXT("Next Move can be set = true"));
	}
}

void AAICharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)

{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

FVector AAICharacter::GetCircleFormationOffset(int32 MemberIndex, int32 SquadSize)
{
	float Radius = 75.0f * SquadSize; // The radius of the circle
	float AngleStep = 360.0f / SquadSize; // Calculate the angle step based on the number of members

	// Convert the angle to radians
	float AngleInRadians = FMath::DegreesToRadians(MemberIndex * AngleStep);

	// Calculate the offset based on the angle and radius
	float OffsetX = Radius * FMath::Cos(AngleInRadians);
	float OffsetY = Radius * FMath::Sin(AngleInRadians);

	return FVector(OffsetX, OffsetY, 0.0f);
}

void AAICharacter::Tick(float DeltaTime)
 {
 	Super::Tick(DeltaTime);
 
 	// Handle squad behavior via the subsystem
 	if (SquadSubsystem)
 	{
 		SquadSubsystem->DetectNearbySquadMembers(this);
 		SquadSubsystem->AssignSquadLeader(this);
 		SquadSubsystem->OnLeaderDeath(this);
 		SquadSubsystem->AdjustBehaviorBasedOnSquadSize(this);
 	}

	if(bNextMoveCanBeSet)
	{
		CalculateNextMoveState();
	}
	
 	SenseEnemy();
 	UpdateState();
 	UpdateMoveState();
 }