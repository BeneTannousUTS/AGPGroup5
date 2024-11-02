// Fill out your copyright notice in the Description page of Project Settings.


#include "ClientController.h"

#include "PlayerCharacterHUD.h"

void AClientController::UpdateHUDBalance_Implementation(int32 NewBalance)
{
	if (HUD)
	{
		HUD->SetBalance(NewBalance);
	}
}

void AClientController::SetHUDReference(UPlayerCharacterHUD* NewHUD)
{
	HUD = NewHUD;
}

void AClientController::BeginPlay()
{
	Super::BeginPlay();

	HUD = Cast<UPlayerCharacterHUD>(GetHUD());
}
