// Copyright Epic Games, Inc. All Rights Reserved.


#include "AGPGameModeBase.h"

#include "AGPGameInstance.h"
#include "AGPPlayerState.h"


AAGPGameModeBase::AAGPGameModeBase()
{
	PlayerCount = 0;
}

void AAGPGameModeBase::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	
	if (NewPlayer)
	{
		if (UAGPGameInstance* GameInstance = Cast<UAGPGameInstance>(GetGameInstance()))
		{
			if (AAGPPlayerState* PlayerState = NewPlayer->GetPlayerState<AAGPPlayerState>())
			{
				if (NewPlayer->IsLocalController())
				{
					PlayerState->PlayerTeam = ETeam::Team1;
					GameInstance->PlayerTeam = ETeam::Team1;
				}
				else
				{
					PlayerState->PlayerTeam = ETeam::Team2;
					GameInstance->PlayerTeam = ETeam::Team2;
				}
			}
		}
	}
}
