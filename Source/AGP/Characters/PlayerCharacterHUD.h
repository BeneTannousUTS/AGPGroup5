// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerCharacterHUD.generated.h"

/**
 * 
 */
UCLASS()
class AGP_API UPlayerCharacterHUD : public UUserWidget
{
	GENERATED_BODY()
public:
	void SetBalance(int32 Balance) const;

protected:
	UPROPERTY(EditAnywhere, meta=(BindWidget))
	class UTextBlock* BalanceText;
};
