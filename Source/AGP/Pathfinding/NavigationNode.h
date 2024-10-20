// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NavigationNode.generated.h"

UENUM()
enum class ENavigationNodeType : uint8
{
	WALKING,
	CLIMBINGUP,
	CLIMBINGDOWN,
	CRAWLING,
	OBSTACLE
};

UCLASS()
class AGP_API ANavigationNode : public AActor
{
	GENERATED_BODY()

	friend class UPathfindingSubsystem;
	
public:	
	// Sets default values for this actor's properties
	ANavigationNode();

	virtual bool ShouldTickIfViewportsOnly() const override;
	UPROPERTY(EditAnywhere)
	TArray<ANavigationNode*> ConnectedNodes;
	UPROPERTY(VisibleAnywhere)
	USceneComponent* LocationComponent;
	UPROPERTY(VisibleAnywhere)
	ENavigationNodeType NodeType = ENavigationNodeType::WALKING;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
