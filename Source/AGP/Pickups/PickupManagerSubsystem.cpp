// Fill out your copyright notice in the Description page of Project Settings.


#include "PickupManagerSubsystem.h"

#include "Pickup.h"
#include "AGP/AGPGameInstance.h"
#include "AGP/Pathfinding/PathfindingSubsystem.h"

void UPickupManagerSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// We don't want this pickup manager to do any spawning if it isn't
	// on the server.
	// A value < NM_Client is any type of server. So if it is >=
	// to NM_Client or == NM_Client then we know it is the client
	// and we don't want to spawn.
	if (GetWorld()->GetNetMode() >= NM_Client)
	{
		return;
	}

	if (PossibleSpawnLocations.IsEmpty())
	{
		PopulateSpawnLocations();
	}

	TimeSinceLastSpawn += DeltaTime;
	if (TimeSinceLastSpawn >= PickupSpawnRate)
	{
		SpawnWeaponPickup();
		TimeSinceLastSpawn = 0.0f;
	}
}

void UPickupManagerSubsystem::SpawnWeaponPickup()
{
	// This function will never be called if this PickupManagerSubsystem is not on the server.
	// because the tick function immediately terminates if it isn't on the server as described above.
	// You could alternatively check in here and only spawn if it is on the server.
	if (PossibleSpawnLocations.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("Unable to spawn weapon pickup."))
		return;
	}
	
	if (UAGPGameInstance* GameInstance =
		GetWorld()->GetGameInstance<UAGPGameInstance>())
	{
		FVector SpawnPosition =
				PossibleSpawnLocations[FMath::RandRange(0, PossibleSpawnLocations.Num()-1)];
		SpawnPosition.Z += 50.0f;

		GetWorld()->SpawnActor<APickup>(GameInstance->GetMoneyPickupClass(), SpawnPosition + FVector(0,0,35.0f), FRotator::ZeroRotator);
		
		/*if (GameInstance->Balance < 500)
		{
			GetWorld()->SpawnActor<APickup>(
			GameInstance->GetMoneyPickupClass(), SpawnPosition + FVector(0,0,35.0f), FRotator::ZeroRotator);
			//GameInstance->UpdateBalance(100);
			//UE_LOG(LogTemp, Display, TEXT("Money Pickup Spawned"));
		}
		else
		{
			GetWorld()->SpawnActor<APickup>(
			GameInstance->GetWeaponPickupClass(), SpawnPosition, FRotator::ZeroRotator);
			//UE_LOG(LogTemp, Display, TEXT("Weapon Pickup Spawned"));
		}*/
	}
	else
	{
		UE_LOG(LogTemp, Display, TEXT("Can't find GameInstance"));
	}
}

void UPickupManagerSubsystem::PopulateSpawnLocations()
{
	PossibleSpawnLocations.Empty();
	if (UPathfindingSubsystem* PathfindingSubsystem = GetWorld()->GetSubsystem<UPathfindingSubsystem>())
	{
		PossibleSpawnLocations = PathfindingSubsystem->GetSpawnPositions();
	}
}
