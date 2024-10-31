// Copyright Epic Games, Inc. All Rights Reserved.


#include "AGPGameModeBase.h"

#include "AGPGameInstance.h"


AAGPGameModeBase::AAGPGameModeBase()
{
	PlayerCount = 0;
}

void AAGPGameModeBase::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	UAGPGameInstance* GameInstance = Cast<UAGPGameInstance>(GetGameInstance());
	if (GameInstance)
	{
		if (PlayerCount % 2 == 0)
		{
			GameInstance->PlayerTeam = ETeam::Team1;
		}
		else
		{
			GameInstance->PlayerTeam = ETeam::Team2;
		}
		PlayerCount++;
	}
}
