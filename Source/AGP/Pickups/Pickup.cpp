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
		UE_LOG(LogTemp, Display, TEXT("Overlap event occurred on Pickup from Actor: %s"), *OtherActor->GetName());
		if (UAGPGameInstance* GameInstance = Cast<UAGPGameInstance>(GetGameInstance()); AIChar->GetTeam() == GameInstance->PlayerTeam)
		{
			GameInstance->UpdateBalance(100);
		}
	}
	Destroy();
}

