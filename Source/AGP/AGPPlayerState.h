// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Characters/AICharacter.h"
#include "GameFramework/PlayerState.h"
#include "AGPPlayerState.generated.h"

/**
 * 
 */

UCLASS()
class AGP_API AAGPPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	UPROPERTY(Replicated, EditAnywhere)
	int32 Balance;
	UPROPERTY(Replicated, EditAnywhere)
	ETeam PlayerTeam;
	void AdjustBalance(int32 Amount);

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
