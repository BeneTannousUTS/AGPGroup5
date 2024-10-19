 // Fill out your copyright notice in the Description page of Project Settings.
#include "AGPGameInstance.h"
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
 }


