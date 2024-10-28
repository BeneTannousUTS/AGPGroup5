// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "CoreMinimal.h"
#include "Characters/AICharacter.h"
#include "Characters/PlayerCharacterHUD.h"
#include "Engine/GameInstance.h"
#include "AGPGameInstance.generated.h"

class APickup;
/**
 * 
 */
UCLASS()
class AGP_API UAGPGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:

	UClass* GetWeaponPickupClass() const;
	UClass* GetMoneyPickupClass() const;
	int32 Balance = 0;
	ETeam PlayerTeam;
	void UpdateBalance(int32 Change);
	UPROPERTY()
	UPlayerCharacterHUD* HUD;

	UPROPERTY(EditDefaultsOnly, Category="Pickup Classes")
	TSubclassOf<AAICharacter> AIClass;

protected:

	UPROPERTY(EditDefaultsOnly, Category="Pickup Classes")
	TSubclassOf<APickup> WeaponPickupClass;
	UPROPERTY(EditDefaultsOnly, Category="Pickup Classes")
	TSubclassOf<APickup> MoneyPickupClass;

	UClass* GetAIClass() const;
};
