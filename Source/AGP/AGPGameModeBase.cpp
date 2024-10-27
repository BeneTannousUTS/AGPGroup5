<<<<<<< HEAD
// Copyright Epic Games, Inc. All Rights Reserved.


#include "AGPGameModeBase.h"

=======
#include "AGPGameModeBase.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

/*AAGPGameModeBase::AAGPGameModeBase()
{
	// No special initialization needed
}

void AAGPGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	// Populate the PlayerStart array
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), AvailablePlayerStarts);

	if (AvailablePlayerStarts.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("No PlayerStart actors found!"));
	}
}

AActor* AAGPGameModeBase::ChoosePlayerStart_Implementation(AController* Player)
{
	// Ensure there are PlayerStarts available
	if (AvailablePlayerStarts.Num() > 0 && PlayerIndex < AvailablePlayerStarts.Num())
	{
		// Assign a unique PlayerStart based on the PlayerIndex
		AActor* SelectedStart = AvailablePlayerStarts[PlayerIndex];
		PlayerIndex++;  // Increment to the next PlayerStart for the next player
		return SelectedStart;
	}

	// Fallback to default behavior
	return Super::ChoosePlayerStart_Implementation(Player);
}

void AAGPGameModeBase::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	// Restart the player at a PlayerStart once connected
	RestartPlayerAtStart(NewPlayer);
}

void AAGPGameModeBase::RestartPlayerAtStart(AController* Player)
{
	if (AvailablePlayerStarts.Num() > 0)
	{
		// Get the next PlayerStart
		AActor* StartPoint = ChoosePlayerStart_Implementation(Player);

		if (StartPoint)
		{
			// Spawn the default pawn at the PlayerStart location
			APawn* NewPawn = SpawnDefaultPawnFor(Player, StartPoint);
			if (NewPawn)
			{
				// Possess the new pawn with the player controller
				Player->Possess(NewPawn);
				UE_LOG(LogTemp, Log, TEXT("Player successfully spawned at PlayerStart."));
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("No available PlayerStart. Using default spawn."));
		RestartPlayer(Player);  // Fallback to default spawn logic
	}
}
*/
>>>>>>> ui
