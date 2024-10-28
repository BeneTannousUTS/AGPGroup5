// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickup.h"

#include "../Characters/PlayerCharacter.h"
#include "AGP/AGPGameInstance.h"
#include "AGP/Characters/AICharacter.h"

void APickup::OnPickupOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                              UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& HitInfo)
{
	//Super::OnPickupOverlap(OverlappedComponent, OtherActor, OtherComponent, OtherBodyIndex, bFromSweep, HitInfo);

	if (AAICharacter* AIChar = Cast<AAICharacter>(OtherActor))
	{
		UE_LOG(LogTemp, Display, TEXT("AI Overlap event occurred on Pickup"))
		UAGPGameInstance* GameInstance = Cast<UAGPGameInstance>(GetGameInstance());
		if (AIChar->GetTeam() == GameInstance->PlayerTeam)
		{
			GameInstance->UpdateBalance(100);
		}
	}
	
	/*if (ABaseCharacter* Player = Cast<ABaseCharacter>(OtherActor))
	{
		if (!Player->HasWeapon())
		{
			//Player->EquipWeapon(true);
			Destroy();
		}
	}*/
}
