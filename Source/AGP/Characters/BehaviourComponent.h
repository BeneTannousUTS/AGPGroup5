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
	bool IsCoverPositionValid(const FVector& CoverPosition, const FVector& ObstacleLocation);

	float CoverOffsetDistance = 50.0f;
	FVector CoverVector;
	bool bIsTakingCover = false;

	//Logic for any exceptions to AI logic, i.e. looking for money, healing squad members, etc
	void CheckSpecialActions();

	void ScoutTick();
	void MedicTick();

	void SeekHealing();
	//Since Snipers operate independently they will need a few helper functions
	void SniperTick();

	UPROPERTY()
	ANavigationNode* SniperVantageNode = nullptr;
	bool bSniperInPosition = false;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	//Helper function to be called when sniper takes damage
	void SeekVantagePoint();

private:
	TWeakObjectPtr<AAICharacter> ClosestHealer;
};
