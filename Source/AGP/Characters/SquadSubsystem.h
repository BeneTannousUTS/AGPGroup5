// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AICharacter.h"
#include "SquadSubsystem.generated.h"

UCLASS()
class AGP_API USquadSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
	// Detect nearby squad members for the given AI character
	void DetectNearbySquadMembers(AAICharacter* AICharacter);

	// Assign a squad leader based on the nearby members
	void AssignSquadLeader(AAICharacter* AICharacter);

	// Adjust behavior based on the squad size
	void AdjustBehaviorBasedOnSquadSize(AAICharacter* AICharacter);

	// Handle the event where a squad leader dies
	void OnLeaderDeath(AAICharacter* AICharacter);
	
	// Revoke leadership from the current leader
	void RevokeLeadershipStatus(AAICharacter* AICharacter);

	void SuggestTargetToLeader(AAICharacter* AICharacter, AAICharacter* PotentialTarget);

	void CommunicateTargetToSquad(AAICharacter* AICharacter, AAICharacter* PotentialTarget);

private:
	// Helper function to determine if the AI should be the squad leader
	void DetermineNewLeader(AAICharacter* AICharacter);
};
