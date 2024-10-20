// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerCharacterHUD.h"
#include "Components/TextBlock.h"

void UPlayerCharacterHUD::SetBalance(int32 Balance) const
{
	if (BalanceText)
	{
		BalanceText->SetText(FText::FromString("Balance: $" + FString::FromInt(Balance)));
	}
}
