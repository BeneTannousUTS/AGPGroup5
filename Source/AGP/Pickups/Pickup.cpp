// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickup.h"

#include "../Characters/PlayerCharacter.h"
#include "AGP/AGPGameInstance.h"
#include "AGP/AGPPlayerState.h"
#include "AGP/Characters/AICharacter.h"
#include "Kismet/GameplayStatics.h"

APickup::APickup()
{
	bReplicates = true;
}

void APickup::OnPickupOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                              UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& HitInfo)
{
	if (AAICharacter* AIChar = Cast<AAICharacter>(OtherActor))
	{
		UE_LOG(LogTemp, Display, TEXT("Overlap event occurred on Pickup from Actor: %s"), *OtherActor->GetName());

		ETeam AITeam = AIChar->GetTeam();
		if (AITeam == ETeam::Team1)
		{
			UE_LOG(LogTemp, Error, TEXT("AI ON TEAM 1"))
		} else if (AITeam == ETeam::Team2)
		{
			UE_LOG(LogTemp, Error, TEXT("AI ON TEAM 2"))
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("AI HAS NO TEAM"))
		}

		if (GetNetMode() < NM_Client)
		{
			ServerPickupCollide(AIChar->GetTeam());
		}
		else
		{
			HudUpdate(AIChar->GetTeam());
		}
		
		
	}
}

void APickup::HudUpdate(ETeam AITeam)
{
	if (UAGPGameInstance* AGPGameInstance = Cast<UAGPGameInstance>(GetGameInstance()))
	{
		ETeam GameTeam = AGPGameInstance->PlayerTeam;
		if (GameTeam == ETeam::Team1)
		{
			UE_LOG(LogTemp, Error, TEXT("Game ON TEAM 1"))
		} else if (GameTeam == ETeam::Team2)
		{
			UE_LOG(LogTemp, Error, TEXT("Game ON TEAM 2"))
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Game HAS NO TEAM"))
		}
		
		if (AGPGameInstance->PlayerTeam == AITeam)
		{
			UE_LOG(LogTemp, Display, TEXT("GAME INSTANCE TEAM IS SAME AS AI TEAM"))
			for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
			{
				if (APlayerController* PlayerController = Iterator->Get())
				{
					if (PlayerController->IsLocalController())
					{
						if (AAGPPlayerState* PlayerState = PlayerController->GetPlayerState<AAGPPlayerState>())
						{
							UE_LOG(LogTemp, Display, TEXT("Player %s is getting HUD updated"), *PlayerState->GetUniqueId().ToString())
							// Update the player's balance in their PlayerState
							PlayerState->AdjustBalance(100);
							AGPGameInstance->HUD->SetBalance(PlayerState->Balance);
							break;
						}
					}
					/*// Get the PlayerState and check if it matches the AIChar's team
					if (AAGPPlayerState* PlayerState = PlayerController->GetPlayerState<AAGPPlayerState>(); PlayerState->PlayerTeam == AITeam)
					{
						UE_LOG(LogTemp, Display, TEXT("Player %s is getting HUD updated"), *PlayerState->GetUniqueId().ToString())
						// Update the player's balance in their PlayerState
						PlayerState->AdjustBalance(100);
						AGPGameInstance->HUD->SetBalance(PlayerState->Balance);
						break;
					}*/
				}
			}
		}
	}
}

void APickup::ServerPickupCollide_Implementation(ETeam AITeam)
{
	MulticastHudUpdate(AITeam);
	Destroy();
}

void APickup::MulticastHudUpdate_Implementation(ETeam AITeam)
{
	UE_LOG(LogTemp, Display, TEXT("CALLED THE MULTICAST %d"), GetWorld()->GetNetMode());
	HudUpdate(AITeam);
}

