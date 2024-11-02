// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ClientController.generated.h"

class AHUD;
class UPlayerCharacterHUD;
/**
 * 
 */
UCLASS()
class AGP_API AClientController : public APlayerController
{
	GENERATED_BODY()

public:

	UFUNCTION(Client, Reliable)
	void UpdateHUDBalance(int32 NewBalance);

	// Function to set HUD reference
	void SetHUDReference(UPlayerCharacterHUD* NewHUD);

protected:
	virtual void BeginPlay() override;

private:
	UPlayerCharacterHUD* HUD;
};
