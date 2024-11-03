// Fill out your copyright notice in the Description page of Project Settings.


#include "BehaviourComponent.h"
#include "AICharacter.h"
#include "AIHelpers.h"
#include "EngineUtils.h"
#include "HealthComponent.h"
#include "AGP/Pathfinding/PathfindingSubsystem.h"

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
	if(OwnerCharacter->GetWeaponType() == Sniper)
	{
		OwnerCharacter->bIgnoreStandardTick = true;
	}
}

void UBehaviourComponent::TickFollowLeader()
{
	if(!OwnerCharacter->CurrentPath.IsEmpty())
	{
		OwnerCharacter->CurrentPath.Empty();
	}
	if (!OwnerCharacter->SquadLeader.Get() || OwnerCharacter->SquadMembers.IsEmpty()) return;

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
}

void UBehaviourComponent::TickEngage()
{
	if (!OwnerCharacter->SensedCharacter.IsValid())
	{
		OwnerCharacter->CurrentState = EAIState::Patrol;
		return;
	}

	FVector MovementDirection = OwnerCharacter->SensedCharacter->GetActorLocation() - OwnerCharacter->GetActorLocation();

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
			OwnerCharacter->CurrentPath.Empty();
			OwnerCharacter->bIsRunningFromEnemy = false;
			OwnerCharacter->CurrentState = EAIState::Patrol;
		}
	}
}

void UBehaviourComponent::TickCover()
{
	if(bIsTakingCover) return;
	OwnerCharacter->CurrentPath.Empty();

	FVector ObstacleLocation = OwnerCharacter->PathfindingSubsystem->FindInMap(
		OwnerCharacter->GetActorLocation(), FName("RockObstacle"));

	if (ObstacleLocation.IsZero())
	{
		// No obstacle found, return early to avoid unnecessary calculations
		return;
	}

	// Get the last known location where the character took damage
	
	FVector DamageLocation = OwnerCharacter->HealthComponent->GetLastKnownDamageLocation();

	// Calculate a position on the opposite side of the obstacle, facing away from the damage location
	FVector CoverDirection = (OwnerCharacter->GetActorLocation() - DamageLocation).GetSafeNormal();
	CoverVector = ObstacleLocation + CoverDirection * CoverOffsetDistance;

	// Check if this CoverVector is actually behind the obstacle
	if (!IsCoverPositionValid(CoverVector, ObstacleLocation))
	{
		// If not, try a fallback position (e.g., using a perpendicular vector or another obstacle)
		CoverVector = ObstacleLocation; // Fallback, refine this if needed.
	}
	
	// Set the calculated cover vector as the target cover point
	OwnerCharacter->CurrentPath = OwnerCharacter->PathfindingSubsystem->GetPath(
		OwnerCharacter->GetActorLocation(), CoverVector);
}

bool UBehaviourComponent::IsCoverPositionValid(const FVector& CoverPosition, const FVector& ObstacleLocation)
{
	// Perform a line trace to check if the cover position is hidden from the damage source
	FHitResult Hit;
	FVector TraceStart = ObstacleLocation;
	FVector TraceEnd = CoverPosition;

	// Use line tracing to determine if there's an obstacle blocking the view
	bool bHit = OwnerCharacter->GetWorld()->LineTraceSingleByChannel(
		Hit, TraceStart, TraceEnd, ECC_Visibility);
    
	// Return true if there's an obstacle (e.g., the rock) in between
	return bHit && IsValid(Hit.GetActor()) && Hit.GetActor()->ActorHasTag(FName("RockObstacle"));
}

//---------------SPECIAL TICK FUNCTIONS BEGIN HERE----------------------//

void UBehaviourComponent::CheckSpecialActions()
{
	EAIType	Type = OwnerCharacter->AIType;

	switch (Type)
	{
	case EAIType::Scout:
		ScoutTick();
		break;
	case EAIType::Medic:
		MedicTick();
		break;
	case EAIType::Sniper:
		SniperTick();
		break;
	case EAIType::Soldier:
		OwnerCharacter->bIgnoreStandardTick = false;
		break;
	}
}

void UBehaviourComponent::ScoutTick()
{
	if (OwnerCharacter->SensedMoney.IsValid())
	{
		OwnerCharacter->bIgnoreStandardTick = true;

		// Move toward money pickup
		FVector MoneyLocation = OwnerCharacter->SensedMoney->GetActorLocation();
		FVector MoveDirection = MoneyLocation - OwnerCharacter->GetActorLocation();
		MoveDirection.Normalize();
		OwnerCharacter->AddMovementInput(MoveDirection);
	}
	else
	{
		OwnerCharacter->bIgnoreStandardTick = false;
	}
}

void UBehaviourComponent::MedicTick()
{
	// Identify any squad members with low health
	for (auto& SquadMember : OwnerCharacter->SquadMembers)
	{
		if (SquadMember && SquadMember->HealthComponent->GetCurrentHealth()
			< SquadMember->HealthComponent->GetMaxHealth() * 0.5f)
		{
			OwnerCharacter->bIgnoreStandardTick = true;
            
			// Move towards the injured squad member
			FVector MemberLocation = SquadMember->GetActorLocation();
			FVector MoveDirection = MemberLocation - OwnerCharacter->GetActorLocation();
			MoveDirection.Normalize();
			OwnerCharacter->AddMovementInput(MoveDirection);

			// If close enough, heal the squad member
			float DistanceToMember = FVector::Dist(OwnerCharacter->GetActorLocation(), MemberLocation);
			if (DistanceToMember < 100.0f)
			{
				SquadMember->HealthComponent->ApplyHealing(5);
			}
			return;  // Exit after healing one target to avoid overlap
		}
	}

	// If no squad members need healing, enable standard behavior
	OwnerCharacter->bIgnoreStandardTick = false;
}

void UBehaviourComponent::SeekHealing()
{
	for(TActorIterator<AAICharacter> Itr(GetWorld()); Itr; ++Itr)
	{
		if(Itr->GetTeam() == OwnerCharacter->GetTeam())
		{
			if(!ClosestHealer.IsValid())
			{
				ClosestHealer = *Itr;
			}

			float CurrentHealerDistance = FVector::Dist(OwnerCharacter->GetActorLocation(), ClosestHealer->GetActorLocation());
			float NewHealerDistance = FVector::Dist(Itr->GetActorLocation(), ClosestHealer->GetActorLocation());
			
			if(CurrentHealerDistance > NewHealerDistance)
			{
				ClosestHealer = *Itr;
			}
		}
	}

	if(!OwnerCharacter->CurrentPath.IsEmpty())
	{
		OwnerCharacter->CurrentPath.Empty();
		OwnerCharacter->CurrentPath.Add(ClosestHealer->GetActorLocation());
	}
}

void UBehaviourComponent::SeekVantagePoint()
{
	if(!OwnerCharacter->CurrentPath.IsEmpty())
	{
		OwnerCharacter->CurrentPath.Empty();
	}
	
	SniperVantageNode = OwnerCharacter->PathfindingSubsystem->FindHighNode(
		OwnerCharacter->GetActorLocation(), 0.75f); // Targeting the top 25%

	OwnerCharacter->CurrentPath = OwnerCharacter->PathfindingSubsystem->GetPath(
		OwnerCharacter->GetActorLocation(), SniperVantageNode->GetActorLocation());

	bSniperInPosition = false;
}

void UBehaviourComponent::SniperTick()
{
	if (OwnerCharacter->HealthComponent->GetCurrentHealthPercentage() < 35.0f)
	{
		SeekHealing(); // Implement logic to find nearby allies for healing.
		return; // Stop further processing if seeking healing.
	}

	if(!bSniperInPosition && !SniperVantageNode)
	{
		SeekVantagePoint(); //Find sniper perch.
		return;
	}
	if(!bSniperInPosition && SniperVantageNode)
	{
		return;
	}

	if (OwnerCharacter->SensedCharacter.IsValid() && bSniperInPosition)
	{
		if (OwnerCharacter->HasWeapon())
		{
			FVector TargetDirection = OwnerCharacter->GetActorLocation()
			- OwnerCharacter->SensedCharacter->GetActorLocation();
			TargetDirection.Normalize();
			OwnerCharacter->SetActorRotation(FMath::Lerp(OwnerCharacter->GetActorRotation(),
				TargetDirection.Rotation(), 0.05f));

			if (OwnerCharacter->WeaponComponent->IsMagazineEmpty())
			{
				OwnerCharacter->WeaponComponent->Reload();
			}
			OwnerCharacter->Fire(OwnerCharacter->SensedCharacter->GetActorLocation());
		}
	}
}


// Called every frame
void UBehaviourComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

