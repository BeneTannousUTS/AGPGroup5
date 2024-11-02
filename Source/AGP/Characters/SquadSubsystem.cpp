// SquadSubsystem.cpp

#include "SquadSubsystem.h"
#include "AICharacter.h"
#include "HealthComponent.h"

void USquadSubsystem::DetectNearbySquadMembers(AAICharacter* AICharacter)
{
	float DetectionRadius = 1000.0f; 
	TArray<FOverlapResult> Overlaps;
	FCollisionShape CollisionShape = FCollisionShape::MakeSphere(DetectionRadius);

	if (AICharacter->GetWorld()->OverlapMultiByChannel(Overlaps, AICharacter->GetActorLocation(), FQuat::Identity, ECC_Pawn, CollisionShape))
	{
		for (auto& Overlap : Overlaps)
		{
			AAICharacter* DetectedAI = Cast<AAICharacter>(Overlap.GetActor());
			if (DetectedAI && DetectedAI != AICharacter && !DetectedAI->HealthComponent->IsDead()
				&& DetectedAI->GetTeam() == AICharacter->GetTeam())
			{
				AICharacter->SquadMembers.AddUnique(DetectedAI);
				AICharacter->NearbyTeamMembers.Add(DetectedAI);
			}
		}
	}
	
	for (int32 i = AICharacter->SquadMembers.Num() - 1; i >= 0; --i)
	{
		AAICharacter* SquadMember = AICharacter->SquadMembers[i];
		if (!AICharacter->NearbyTeamMembers.Contains(SquadMember) || SquadMember->HealthComponent->IsDead())
		{
			if(SquadMember == AICharacter->SquadLeader)
			{
				AICharacter->SquadLeader = nullptr;
			}
			AICharacter->SquadMembers.RemoveAt(i);
		}
	}
	AICharacter->NearbyTeamMembers.Empty();
}

void USquadSubsystem::AssignSquadLeader(AAICharacter* AICharacter)
{
	// First, check if AICharacter is a valid pointer
	if (!AICharacter) return;

	// If the AI character has squad members
	if (AICharacter->SquadMembers.Num() > 0)
	{
		// Check if the current SquadLeader is valid and still in the squad
		if (!AICharacter->SquadMembers.Contains(AICharacter->SquadLeader))
		{
			// If the current leader is not valid, set it to null and determine a new leader
			AICharacter->SquadLeader = nullptr;
			DetermineNewLeader(AICharacter);
		}
	}
	else if (AICharacter->SquadMembers.Num() == 0)
	{
		// If there are no squad members, set the SquadLeader to null and update state
		AICharacter->SquadLeader = nullptr;
	}
}

void USquadSubsystem::AdjustBehaviorBasedOnSquadSize(AAICharacter* AICharacter)
{
	int32 SquadSize = AICharacter->SquadMembers.Num();
	if (SquadSize == 0)
	{
		if(!AICharacter->SensedCharacter.IsValid()) //If no sensed character, search
		{
			AICharacter->CurrentState = EAIState::Patrol;
		}else if(AICharacter->SensedCharacter->SquadMembers.Num() > 0) //If enemy has squad, evade
		{
			AICharacter->CurrentState = EAIState::Evade;
		}
	}
	else if (SquadSize >= 1)
	{
		if (AICharacter->SquadLeader == AICharacter && AICharacter->SensedCharacter.IsValid())
		{
			AICharacter->CurrentState = EAIState::Engage;
		}
		else if(IsValid(AICharacter->SquadLeader) && AICharacter->SquadLeader == AICharacter)
		{
			AICharacter->CurrentState = EAIState::Patrol;
		}else
		{
			AICharacter->CurrentState = EAIState::Follow;
		}
	}
	else
	{
		AICharacter->CurrentState = EAIState::Patrol;
	}
}

void USquadSubsystem::OnLeaderDeath(AAICharacter* AICharacter)
{
	if (AICharacter->SquadLeader && AICharacter->SquadLeader->HealthComponent->IsDead())
	{
		DetermineNewLeader(AICharacter);
	}
}

void USquadSubsystem::RevokeLeadershipStatus(AAICharacter* AICharacter)
{
	if (AICharacter->SquadLeader == AICharacter)
	{
		AICharacter->SquadLeader = nullptr;
	}
}

void USquadSubsystem::SuggestTargetToLeader(AAICharacter* AICharacter, AAICharacter* PotentialTarget)
{
	// Check if AICharacter and its SquadLeader are valid
	if (!AICharacter || !AICharacter->SquadLeader) return;

	// If AICharacter is the squad leader, communicate the target to the squad
	if (AICharacter->SquadLeader == AICharacter)
	{
		CommunicateTargetToSquad(AICharacter, PotentialTarget);
	}
	// If the squad leader doesn't have a sensed character, set it to the potential target
	else if (!AICharacter->SquadLeader->SensedCharacter.IsValid())
	{
		AICharacter->SquadLeader->SensedCharacter = PotentialTarget;
	}
	//If the player is alone and the sensed character isn't part of a squad (1v1) take the fight
	else if (AICharacter->SquadMembers.Num() == 0 && PotentialTarget->SquadMembers.Num() == 0)
	{
		AICharacter->SensedCharacter = PotentialTarget;
		AICharacter->CurrentState = EAIState::Engage;
	}
}


void USquadSubsystem::CommunicateTargetToSquad(AAICharacter* AICharacter, AAICharacter* PotentialTarget)
{
	for (AAICharacter* SquadMember : AICharacter->SquadMembers)
	{
		SquadMember->SensedCharacter = PotentialTarget;
	}
	AICharacter->CurrentState = EAIState::Engage;
}

void USquadSubsystem::DetermineNewLeader(AAICharacter* AICharacter)
{
	if (!AICharacter) return;

	// Variables to track the new leader and the highest health found
	AAICharacter* NewLeader = AICharacter;
	float HighestHealth = AICharacter->HealthComponent->GetCurrentHealth();

	// Iterate through the squad members to find the healthiest member
	for (AAICharacter* SquadMember : AICharacter->SquadMembers)
	{
		if (SquadMember->HealthComponent->IsDead()) continue; // Skip dead members

		float OtherAIHealth = SquadMember->HealthComponent->GetCurrentHealth();

		// If this squad member has more health than the current highest
		if (OtherAIHealth > HighestHealth)
		{
			NewLeader = SquadMember; // Update the new leader
			HighestHealth = OtherAIHealth; // Update the highest health
		}
	}

	// Set the new leader for AICharacter
	AICharacter->SquadLeader = NewLeader;

	// Ensure all squad members point to the same leader
	for (AAICharacter* SquadMember : AICharacter->SquadMembers)
	{
		if (!SquadMember->HealthComponent->IsDead())
		{
			SquadMember->SquadLeader = NewLeader; // Set the leader for all members
		}
	}

	// Set the squad leader status for the leader itself
	NewLeader->SquadLeader = NewLeader;
}
