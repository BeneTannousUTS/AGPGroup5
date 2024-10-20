// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickup.h"

#include "../Characters/PlayerCharacter.h"

void APickup::OnPickupOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                    UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& HitInfo)
{
	//Super::OnPickupOverlap(OverlappedComponent, OtherActor, OtherComponent, OtherBodyIndex, bFromSweep, HitInfo);
	UE_LOG(LogTemp, Display, TEXT("Overlap event occurred on WeaponPickup"))

	if (ABaseCharacter* Player = Cast<ABaseCharacter>(OtherActor))
	{
		if (!Player->HasWeapon())
		{
			//Player->EquipWeapon(true);
			Destroy();
		}
	}
}
