// Fill out your copyright notice in the Description page of Project Settings.


#include "BehaviourComponent.h"

#include "AICharacter.h"
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
	if(!OwnerCharacter->GetCurrentPath()->IsEmpty())
	{
		OwnerCharacter->GetCurrentPath()->Empty();
	}
	if (!OwnerCharacter->GetSquadLeader() || OwnerCharacter->GetSquadMembers().IsEmpty()) return;

	int32 Index = OwnerCharacter->GetSquadLeader()->GetSquadMembers().Find(OwnerCharacter);
	FVector FormationOffset = OwnerCharacter->GetCircleFormationOffset(Index,
		OwnerCharacter->GetSquadMembers().Num());
	FVector TargetLocation = OwnerCharacter->GetSquadLeader()->GetActorLocation() + FormationOffset;
    
	FVector MovementDirection = TargetLocation - OwnerCharacter->GetActorLocation();
	float DistanceToTarget = MovementDirection.Size();

	float MinDistanceThreshold = 10.0f; 
	if (DistanceToTarget > MinDistanceThreshold)
	{
		MovementDirection.Normalize();
		OwnerCharacter->AddMovementInput(MovementDirection);
	}

	if(OwnerCharacter->GetSensedCharacter().Get())
	{
		if (OwnerCharacter->HasWeapon())
		{
			// Check if the weapon's magazine is empty and reload if necessary
			if (OwnerCharacter->GetWeaponComponent()->IsMagazineEmpty())
			{
				OwnerCharacter->GetWeaponComponent()->Reload();
			}
			OwnerCharacter->Fire(OwnerCharacter->GetSensedCharacter()->GetActorLocation());
		}
	}
}

void UBehaviourComponent::TickPatrol()
{
}

void UBehaviourComponent::TickEngage()
{
}

void UBehaviourComponent::TickEvade()
{
}

void UBehaviourComponent::TickCover()
{
}

void UBehaviourComponent::CheckSpecialActions()
{
}


// Called every frame
void UBehaviourComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

