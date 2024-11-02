// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AICharacter.h"
#include "Components/ActorComponent.h"
#include "BehaviourComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class AGP_API UBehaviourComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UBehaviourComponent();

protected:
	friend class AAICharacter;

	UPROPERTY()
	AAICharacter* OwnerCharacter = nullptr;
	
	// Called when the game starts
	virtual void BeginPlay() override;

	// Logic for different AI states
	void TickFollowLeader();
	void TickPatrol();
	void TickEngage();
	void TickEvade();
	void TickCover();

	//Logic for any exceptions to AI logic, i.e. looking for money, healing squad members, etc
	void CheckSpecialActions();

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};
