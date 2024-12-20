// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickup.h"

#include "../Characters/PlayerCharacter.h"
#include "AGP/AGPGameInstance.h"
#include "AGP/Characters/AICharacter.h"

void APickup::OnPickupOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                              UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& HitInfo)
{
	//Super::OnPickupOverlap(OverlappedComponent, OtherActor, OtherComponent, OtherBodyIndex, bFromSweep, HitInfo);
	UE_LOG(LogTemp, Display, TEXT("Overlap event occurred on Pickup from Actor: %s"), *OtherActor->GetName());
	if (AAICharacter* AIChar = Cast<AAICharacter>(OtherActor))
	{
		UAGPGameInstance* GameInstance = Cast<UAGPGameInstance>(GetGameInstance());

		// this code need changes, it needs to find the client with the correct player team then add the money to that game instance
		
		if (AIChar->GetTeam() == GameInstance->PlayerTeam)
		{
			GameInstance->UpdateBalance(100);
		}
	}
	
	/*if (ABaseCharacter* Player = Cast<ABaseCharacter>(OtherActor))
	{
		if (!Player->HasWeapon())
		{
<<<<<<< HEAD
			//Player->EquipWeapon(true);
=======
			Player->EquipWeapon(true);
>>>>>>> ui
			Destroy();
		}
	}*/
}
