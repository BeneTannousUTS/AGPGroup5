 // Fill out your copyright notice in the Description page of Project Settings.
#include "AGPGameInstance.h"

#include "Characters/AICharacter.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "Pickups/Pickup.h"

 UClass* UAGPGameInstance::GetWeaponPickupClass() const
{
 return WeaponPickupClass.Get();
}

 UClass* UAGPGameInstance::GetMoneyPickupClass() const
 {
 return  MoneyPickupClass.Get();
 }

void UAGPGameInstance::UpdateBalance(int32 Change)
 {
  Balance += Change;
  if (HUD) // Check to ensure the HUD exists before updating
  {
   HUD->SetBalance(Balance);
  }
 }

 UClass* UAGPGameInstance::GetAIClass() const
 {
  return AIClass.Get();
 }