// Fill out your copyright notice in the Description page of Project Settings.

#include "AICharacter.h"
#include "BehaviourComponent.h"
#include "HealthComponent.h"
#include "AGP/Pathfinding/PathfindingSubsystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "SquadSubsystem.h"
#include "AGP/Pickups/Pickup.h"
#include "Net/UnrealNetwork.h"
#include "Perception/PawnSensingComponent.h"

AAICharacter::AAICharacter(): PathfindingSubsystem(nullptr), SquadSubsystem(nullptr), BehaviourComponent(nullptr), SquadLeader(nullptr)
{
	PrimaryActorTick.bCanEverTick = true;
	
	PawnSensingComponent = CreateDefaultSubobject<UPawnSensingComponent>("Pawn Sensing Component");
	BehaviourComponent = CreateDefaultSubobject<UBehaviourComponent>(TEXT("BehaviourComponent"));
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
	if(!BehaviourComponent)
	{
		BehaviourComponent->InitializeComponent();
	}

	if (PawnSensingComponent)
	{
		PawnSensingComponent->OnSeePawn.AddDynamic(this, &AAICharacter::OnSensedPawn);
	}

	
	PathfindingSubsystem = GetWorld()->GetSubsystem<UPathfindingSubsystem>();
	SquadSubsystem = GetWorld()->GetSubsystem<USquadSubsystem>();
	CurrentPath = PathfindingSubsystem->GetRandomPath(GetActorLocation());
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

bool AAICharacter::ShouldSenseActors()
{
	if(CurrentState == EAIState::Patrol || CurrentState == EAIState::Engage) return true;
	return false;
}

void AAICharacter::SenseActors()
{
	if (!SensedCharacter.IsValid() || !ShouldSenseActors()) return;
	
	if (PawnSensingComponent && !PawnSensingComponent->HasLineOfSightTo(SensedCharacter.Get()))
	{
		SensedCharacter = nullptr;
	}
}


void AAICharacter::SetWeaponStats(EWeaponType Weapon)
{
	if (Weapon == WeaponStats.WeaponType) return; // Avoid duplicate setup for the same weapon

	switch (Weapon)
	{
	case Pistol:
		WeaponStats = {Pistol, 0.5f, 0.9f, 4.0f, 6, 3};
		break;
	case Sniper:
		WeaponStats = {Sniper, 0.9f, 7.0f, 50.0f, 5, 15};
		break;
	case Rifle:
		WeaponStats = DefaultWeaponStats;
		break;
	}

	EquipWeapon(bHasWeapon, WeaponStats);
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
		//Climb above the lip of the 
		CurrentPath[CurrentPath.Num() - 2] = CurrentPath[CurrentPath.Num() - 2] + FVector(0,0,350);
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
	if(GetLocalRole() == ROLE_Authority)
	{
		AITeam = InAITeam;
		SetAIType(InAIType); // Custom function to set AI type

		// Log for debugging purposes
		UE_LOG(LogTemp, Log, TEXT("Multicast_SetupAI called: Team = %d, Type = %d"), (int)AITeam, (int)InAIType);
	}
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
		MovementComponent->MaxWalkSpeed = 500.0f;
		break;
	case EMoveState::RUNNING:
		bIsCrouching = false;
		if(!MovementComponent->IsFalling()){
			MovementComponent->MovementMode = MOVE_Walking;
		}
		MovementComponent->MaxWalkSpeed = 1000.0f;
		break;
	case EMoveState::CLIMBINGUP:
		bIsCrouching = false;
		MovementComponent->MovementMode = MOVE_Flying;
		MovementComponent->MaxFlySpeed = 200.0f;
			break;
	case EMoveState::CLIMBINGDOWN:
		bIsCrouching = false;
		MovementComponent->MovementMode = MOVE_Flying;
		MovementComponent->MaxFlySpeed = 200.0f;
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
	if(SquadLeader != this && SquadMembers.Num() > 0 && SquadMembers.Contains(SquadLeader))
	{
		CurrentState = EAIState::Follow;
	}
	
	switch (CurrentState)
	{
	case EAIState::Patrol:
		if(HealthComponent->GetCurrentHealthPercentage()*100 < 40.0f)
		{
			CurrentState = EAIState::Cover;
			BehaviourComponent->TickCover();
			break;
		}
		BehaviourComponent->TickPatrol();
		break;
		
	case EAIState::Follow:
		if(HealthComponent->GetCurrentHealthPercentage()*100 < 30.0f)
		{
			CurrentState = EAIState::Cover;
			BehaviourComponent->TickCover();
			break;
		}
		BehaviourComponent->TickFollowLeader();
		break;

	case EAIState::Engage:
		if (HealthComponent->GetCurrentHealthPercentage()*100 < 0.5f)
		{
			CurrentState = EAIState::Evade;
			BehaviourComponent->TickEvade();
			break;
		}
		BehaviourComponent->TickEngage();
		break;

	case EAIState::Evade:
		if (HealthComponent->GetCurrentHealthPercentage()*100 > 0.5f)
		{
			CurrentState = EAIState::Patrol;
			BehaviourComponent->TickPatrol();
			break;
		}
		BehaviourComponent->TickEvade();
		break;

	case EAIState::Cover:
		if (HealthComponent->GetCurrentHealthPercentage()*100 > 0.5f)
		{
			
			CurrentState = EAIState::Patrol;
			BehaviourComponent->bIsTakingCover = false;
			BehaviourComponent->TickPatrol();
			break;
		}
		BehaviourComponent->TickCover();
		break;

	default:
		break;
	}
}

UBehaviourComponent* AAICharacter::GetBehaviourComponent()
{
	return BehaviourComponent;
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
		if(AIType == EAIType::Sniper && IsValid(BehaviourComponent->SniperVantageNode) &&
			FVector::Distance(GetActorLocation(), BehaviourComponent->SniperVantageNode->GetActorLocation())
				< PathfindingError)
		{
			BehaviourComponent->bSniperInPosition = true;
		}else if(CurrentState == EAIState::Cover && !BehaviourComponent->CoverVector.IsZero())
		{
			BehaviourComponent->bIsTakingCover = true;
		}
		
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
	float Radius = 300 * SquadSize; // The radius of the circle
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
		return Rifle;
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
		PawnSensingComponent->SightRadius = 50000;
	}else
	{
		SetWeaponStats(Rifle);
	}
}

void AAICharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Execute squad-related functions at intervals
	SquadCheckTimer += DeltaTime;
	if (SquadSubsystem && SquadCheckTimer >= SquadCheckInterval)
	{
		SquadSubsystem->DetectNearbySquadMembers(this);
		SquadSubsystem->AssignSquadLeader(this);
		
		if (!SquadLeader.IsValid()) // Only call OnLeaderDeath if leader has changed
		{
			SquadSubsystem->OnLeaderDeath(this);
		}
		SquadSubsystem->AdjustBehaviorBasedOnSquadSize(this); // Optional: Only on squad size change
		SquadCheckTimer = 0.0f;
	}

	if(bNextMoveCanBeSet)
	{
		CalculateNextMoveState();
	}
	
	SenseActors();
	if(!bIgnoreStandardTick)
	{
		UpdateState();
	}
	UpdateMoveState();

	// Distance check to see if actor has moved
	MovementCheckTimer += DeltaTime;
	if (MovementCheckTimer >= 2.0f) // 2-second interval
	{
		float DistanceMoved = FVector::Dist(GetActorLocation(), LastPosition);
		if (DistanceMoved < MinDistanceThreshold)
		{
			CurrentPath.Empty();
		}
		// Reset the timer and update last position
		MovementCheckTimer = 0.0f;
		LastPosition = GetActorLocation();
	}
	
	if(!CurrentPath.IsEmpty())
	{
		MoveAlongPath();
	}
}