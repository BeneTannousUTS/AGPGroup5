// Fill out your copyright notice in the Description page of Project Settings.


#include "AGPPlayerState.h"

#include "Net/UnrealNetwork.h"

void AAGPPlayerState::AdjustBalance(int32 Amount)
{
	if (HasAuthority()) // Only allow server to modify Balance
    {
        Balance += Amount;
    }
}



void AAGPPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AAGPPlayerState, Balance);
	DOREPLIFETIME(AAGPPlayerState, PlayerTeam);
}
