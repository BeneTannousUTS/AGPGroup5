// Fill out your copyright notice in the Description page of Project Settings.

#include "AICharacter.h"
#include "HealthComponent.h"
#include "AGP/Pathfinding/PathfindingSubsystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "SquadSubsystem.h"
#include "AGP/Pickups/Pickup.h"
#include "Net/UnrealNetwork.h"
#include "Perception/PawnSensingComponent.h"

AAICharacter::AAICharacter(): PathfindingSubsystem(nullptr), SquadSubsystem(nullptr), SquadLeader(nullptr)
{
	PrimaryActorTick.bCanEverTick = true;
	
	PawnSensingComponent = CreateDefaultSubobject<UPawnSensingComponent>("Pawn Sensing Component");
}

void AAICharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AAICharacter, AITeam);
	DOREPLIFETIME(AAICharacter, AIType);
	DOREPLIFETIME(AAICharacter, SquadLeader);
	DOREPLIFETIME(AAICharacter, MovementState);
	DOREPLIFETIME(AAICharacter, CurrentState);
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

	if (WeaponComponent)
	{
		MulticastSetupAI(AITeam, AIType);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("WeaponComponent is null on MulticastSetupAI call."));
	}

}

void AAICharacter::TickFollowLeader()
{
	GetCharacterMovement()->MaxWalkSpeed = 700.0f;
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

	if(SensedCharacter.Get())
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
	if (!SensedCharacter.IsValid())
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
	if (SensedCharacter.IsValid())
	{
		bIsRunningFromEnemy = true;
		if (CurrentPath.IsEmpty())
		{
			CurrentPath = PathfindingSubsystem->GetPathAway(GetActorLocation(), SensedCharacter->GetActorLocation());
		}
	}else if(!SensedCharacter.IsValid())
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

	FVector CoverVector = PathfindingSubsystem->FindInMap(GetActorLocation(), FName("RockObstacle"));

	CurrentPath.Add(CoverVector);
}

void AAICharacter::CheckSpecialActions()
{
	EAIType	Type = AIType;

	switch (Type)
	{
	case EAIType::Scout:
		if(SensedMoney.IsValid())
		{
			bIgnoreStandardTick = true;
			
		}else
		{
			bIgnoreStandardTick = false;
		}
		break;
	case EAIType::Medic:
		//Medic logic function
		break;
	case EAIType::Sniper:
		//Sniper logic function (if time permits)
		break;
	case EAIType::Soldier:
		bIgnoreStandardTick = false;
		break;
	}
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
	APickup* TargetMoney = Cast<APickup>(SensedActor);
	if (PotentialEnemy)
	{
		// Check if the sensed character is on a different team
		if (PotentialEnemy->GetTeam() != GetTeam())
		{
			// Set the new sensed character
			SensedCharacter = PotentialEnemy;
            
			SquadSubsystem->SuggestTargetToLeader(this, PotentialEnemy);
		}
	}else if(TargetMoney)
	{
		SensedMoney = TargetMoney;
	}
}

void AAICharacter::SenseEnemy()
{
	if (!SensedCharacter.IsValid()) return;
	if (PawnSensingComponent)
	{
		if (!PawnSensingComponent->HasLineOfSightTo(SensedCharacter.Get()))
		{
			SensedCharacter = nullptr;
		}
	}
}

void AAICharacter::SetWeaponStats(EWeaponType Weapon)
{
	
	
	switch (Weapon)
	{
	case EWeaponType::Pistol:
		WeaponStats.WeaponType = EWeaponType::Pistol;
		WeaponStats.Accuracy = 0.5f;
		WeaponStats.FireRate = 0.9f;
		WeaponStats.BaseDamage = 4.0f;
		WeaponStats.MagazineSize = 6;
		WeaponStats.ReloadTime = 3;

		EquipWeapon(bHasWeapon, WeaponStats);
		break;
	case EWeaponType::Sniper:
		WeaponStats.WeaponType = EWeaponType::Sniper;
		WeaponStats.Accuracy = 1.0f;
		WeaponStats.FireRate = 7.0f;
		WeaponStats.BaseDamage = 50.0f;
		WeaponStats.MagazineSize = 5;
		WeaponStats.ReloadTime = 15;
		
		EquipWeapon(bHasWeapon, WeaponStats);
		break;
	case EWeaponType::Rifle:
		WeaponStats = DefaultWeaponStats;
		
		EquipWeapon(bHasWeapon, WeaponStats);
		break;
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

void AAICharacter::OnRep_MoveState()
{
	UpdateMoveState();
}

void AAICharacter::MulticastSetupAI_Implementation(ETeam InAITeam, EAIType InAIType)
{
	AITeam = InAITeam;
	SetAIType(InAIType); // Custom function to set AI type

	// Log for debugging purposes
	UE_LOG(LogTemp, Log, TEXT("Multicast_SetupAI called: Team = %d, Type = %d"), (int)AITeam, (int)InAIType);
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


void AAICharacter::ServerUpdateMoveState_Implementation(EAIState NewState)
{
	if (HasAuthority())
	{
		CurrentState = NewState;
		OnRep_MoveState();
	}
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

EWeaponType AAICharacter::GetWeaponType()
{
	if (!WeaponComponent) 
	{
		UE_LOG(LogTemp, Error, TEXT("WeaponComponent is null in %s"), *GetName());
		return EWeaponType::Rifle;
	}
	
	return WeaponComponent->GetWeaponType();
}

void AAICharacter::SetAIType(EAIType AITypeToSet)
{
	AIType = AITypeToSet;

	if(AIType == EAIType::Medic || AIType == EAIType::Scout)
	{
		SetWeaponStats(Pistol);
	}else if(AIType == EAIType::Sniper){
		SetWeaponStats(Sniper);
	}else
	{
		SetWeaponStats(Rifle);
	}
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
 
