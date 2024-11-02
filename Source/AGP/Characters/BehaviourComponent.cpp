// Fill out your copyright notice in the Description page of Project Settings.


#include "BehaviourComponent.h"

#include "AICharacter.h"
#include "AGP/Pathfinding/PathfindingSubsystem.h"
#include "GameFramework/CharacterMovementComponent.h"

class AAICharacter;
// Sets default values for this component's properties
UBehaviourComponent::UBehaviourComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UBehaviourComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<AAICharacter>(GetOwner());
	
}

void UBehaviourComponent::TickFollowLeader()
{
	OwnerCharacter->GetCharacterMovement()->MaxWalkSpeed = 700.0f;
	if(!OwnerCharacter->CurrentPath.IsEmpty())
	{
		OwnerCharacter->CurrentPath.Empty();
	}
	if (!OwnerCharacter->SquadLeader || OwnerCharacter->SquadMembers.IsEmpty()) return;

	int32 Index = OwnerCharacter->SquadLeader->SquadMembers.Find(OwnerCharacter);
	FVector FormationOffset = OwnerCharacter->GetCircleFormationOffset(Index,
		OwnerCharacter->SquadMembers.Num());
	FVector TargetLocation = OwnerCharacter->SquadLeader->GetActorLocation() + FormationOffset;
    
	FVector MovementDirection = TargetLocation - OwnerCharacter->GetActorLocation();
	float DistanceToTarget = MovementDirection.Size();

	float MinDistanceThreshold = 10.0f; 
	if (DistanceToTarget > MinDistanceThreshold)
	{
		MovementDirection.Normalize();
		OwnerCharacter->AddMovementInput(MovementDirection);
	}

	if(OwnerCharacter->SensedCharacter.Get())
	{
		if (OwnerCharacter->HasWeapon())
		{
			// Check if the weapon's magazine is empty and reload if necessary
			if (OwnerCharacter->WeaponComponent->IsMagazineEmpty())
			{
				OwnerCharacter->WeaponComponent->Reload();
			}
			OwnerCharacter->Fire(OwnerCharacter->SensedCharacter->GetActorLocation());
		}
	}
}

void UBehaviourComponent::TickPatrol()
{
	if (OwnerCharacter->CurrentPath.IsEmpty())
	{
		OwnerCharacter->CurrentPath = OwnerCharacter->PathfindingSubsystem->GetRandomPath(OwnerCharacter->GetActorLocation());
	}
	OwnerCharacter->MoveAlongPath();
}

void UBehaviourComponent::TickEngage()
{
	if (!OwnerCharacter->SensedCharacter.IsValid())
	{
		OwnerCharacter->CurrentState = EAIState::Patrol;
		return;
	}

	// Define safe distance range
	const float MinDistance = 300.0f; // Minimum distance to maintain from the target
	const float MaxDistance = 800.0f; // Maximum distance to maintain from the target

	FVector ToTarget = OwnerCharacter->SensedCharacter->GetActorLocation() - OwnerCharacter->GetActorLocation();
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
		OwnerCharacter->AddMovementInput(MovementDirection);
	}

	// Weapon logic
	if (OwnerCharacter->HasWeapon())
	{
		if (OwnerCharacter->WeaponComponent->IsMagazineEmpty())
		{
			OwnerCharacter->Reload();
		}
		OwnerCharacter->Fire(OwnerCharacter->SensedCharacter->GetActorLocation());
	}

	// Update path if needed
	if (OwnerCharacter->CurrentPath.Num() > 0)
	{
		OwnerCharacter->CurrentPath.Pop();    
	}
}

void UBehaviourComponent::TickEvade()
{
	// Find the player and return if it can't find it.
	if (OwnerCharacter->SensedCharacter.IsValid())
	{
		OwnerCharacter->bIsRunningFromEnemy = true;
		if (OwnerCharacter->CurrentPath.IsEmpty())
		{
			OwnerCharacter->CurrentPath = OwnerCharacter->PathfindingSubsystem->GetPathAway(
				OwnerCharacter->GetActorLocation(),
				OwnerCharacter->SensedCharacter->GetActorLocation());
		}
	}else if(!OwnerCharacter->SensedCharacter.IsValid())
	{
		if (OwnerCharacter->bIsRunningFromEnemy)
		{
			OwnerCharacter->CurrentPath.Pop();
			OwnerCharacter->bIsRunningFromEnemy = false;
		}
		if (OwnerCharacter->CurrentPath.IsEmpty())
		{
			OwnerCharacter->CurrentPath = OwnerCharacter->PathfindingSubsystem->GetRandomPath(
				OwnerCharacter->GetActorLocation());
		}
	}
	OwnerCharacter->MoveAlongPath();
}

void UBehaviourComponent::TickCover()
{
	//TODO DURING ASSESSMENT 4
	OwnerCharacter->CurrentPath.Empty();

	FVector CoverVector = OwnerCharacter->PathfindingSubsystem->FindInMap(
		OwnerCharacter->GetActorLocation(), FName("RockObstacle"));

	OwnerCharacter->CurrentPath.Add(CoverVector);
}

void UBehaviourComponent::CheckSpecialActions()
{
	EAIType	Type = OwnerCharacter->AIType;

	switch (Type)
	{
	case EAIType::Scout:
		if(OwnerCharacter->SensedMoney.IsValid())
		{
			OwnerCharacter->bIgnoreStandardTick = true;
			
		}else
		{
			OwnerCharacter->bIgnoreStandardTick = false;
		}
		break;
	case EAIType::Medic:
		//Medic logic function
			break;
	case EAIType::Sniper:
		//Sniper logic function (if time permits)
			break;
	case EAIType::Soldier:
		OwnerCharacter->bIgnoreStandardTick = false;
		break;
	}
}


// Called every frame
void UBehaviourComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

