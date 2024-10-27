// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiplayerGameMode.h"

#include "Characters/PlayerCharacter.h"

void AMultiplayerGameMode::RespawnPlayer(AController* Controller, int32 PlayerIndex)
{
	if (!Controller || PlayerIndex < 0 || PlayerIndex >= PlayerStartLocations.Num())
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid PlayerIndex or Controller!"));
		return;
	}

	// Unpossess and destroy the old character
	if (APlayerCharacter* CurrentlyPossessedCharacter = Cast<APlayerCharacter>(Controller->GetPawn()))
	{
		Controller->UnPossess();
		CurrentlyPossessedCharacter->Destroy();
	}

	// Spawn the player at the assigned PlayerStart
	AActor* ChosenStart = PlayerStartLocations[PlayerIndex];
	FTransform SpawnTransform = ChosenStart->GetActorTransform();

	// Rotate the player 90 degrees around the Z-axis (yaw)
	FRotator RotationAdjustment(-40.0f, 90.0f, 0.0f);  // Adjust yaw by 90 degrees
	if (PlayerIndex == 1)
	{
		RotationAdjustment = FRotator(-40.0f,-90.0f,0.0f);
	}
	SpawnTransform.SetRotation((SpawnTransform.GetRotation() * FQuat(RotationAdjustment)).GetNormalized());
	RestartPlayerAtTransform(Controller, SpawnTransform);

	UE_LOG(LogTemp, Log, TEXT("Player %d respawned at: %s"), PlayerIndex, *SpawnTransform.ToString());
}


