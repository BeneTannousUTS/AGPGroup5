// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseCharacter.h"
#include "AGP/Pathfinding/NavigationNode.h"
#include "GameFramework/Character.h"
#include "AICharacter.generated.h"

class UPawnSensingComponent;
class UPathfindingSubsystem;
class USquadSubsystem;

UENUM(BlueprintType)
enum class EAIType : uint8
{
	Soldier,
	Sniper,
	Medic,
	Scout
};

UENUM(BlueprintType)
enum class EAIState : uint8
{
	Follow,
	Patrol,
	Engage,
	Evade,
	Cover
};

UENUM(BlueprintType)
enum class EMoveState : uint8
{
	WALKING,
	RUNNING,
	CLIMBINGUP,
	CLIMBINGDOWN,
	CRAWLING
};

UENUM(BlueprintType)
enum class ETeam : uint8
{
	Team1,
	Team2
};

/**
 * AI Logic class
 */
UCLASS()
class AGP_API AAICharacter : public ABaseCharacter
{
	GENERATED_BODY()

public:
	// Sets default values
	AAICharacter();

	UFUNCTION(BlueprintCallable)
	ETeam GetTeam();

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ETeam AITeam = ETeam::Team1;
	
protected:
	friend class USquadSubsystem;

	UFUNCTION()
	void OnSensedPawn(APawn* Pawn);
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Logic for different AI states
	void TickFollowLeader();
	void TickPatrol();
	void TickEngage();
	void TickEvade();
	void TickCover();
	
	// AI state management
	UPROPERTY(EditAnywhere)
	EAIState CurrentState = EAIState::Patrol;

	UPROPERTY(EditAnywhere)
	EMoveState MovementState = EMoveState::WALKING;

	UPROPERTY()
	UPathfindingSubsystem* PathfindingSubsystem;

	UPROPERTY()
	USquadSubsystem* SquadSubsystem;
	
	UPROPERTY(VisibleAnywhere)
	UPawnSensingComponent* PawnSensingComponent;
	
	UPROPERTY(VisibleAnywhere)
	TArray<FVector> CurrentPath;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	AAICharacter* SquadLeader;

	UFUNCTION(BlueprintCallable)
	bool IsLeader();

	UPROPERTY(VisibleAnywhere)
	TArray<AAICharacter*> SquadMembers;

	UPROPERTY(VisibleAnywhere)
	TArray<AAICharacter*> NearbyTeamMembers;
	
	UPROPERTY(VisibleAnywhere)
	AAICharacter* SensedCharacter = nullptr;
	
	UPROPERTY(VisibleAnywhere)
	ANavigationNode* TargetNode;
	UPROPERTY(VisibleAnywhere)
	ANavigationNode* NextNode;
	
	UPROPERTY(VisibleAnywhere)
	EMoveState NextMoveState;

	// Movement and pathfinding
	void MoveAlongPath();
	
	// Emotion-related properties
	int FearLevel = 0;
	int AdrenalineLevel = 0;
	int ConfidenceLevel = 50;

	void SenseEnemy();
	
	UFUNCTION()
	void OnSensedCharacterDestroyed(AActor* DestroyedActor);

	/**
	 * Some arbitrary error value for determining how close is close enough before moving onto the next step in the path.
	 */
	UPROPERTY(EditAnywhere)
	float PathfindingError = 150.0f; // 150 cm from target by default.

	bool bIsRunningFromEnemy;

public:
	void UpdateMoveState();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	FVector GetCircleFormationOffset(int32 MemberIndex, int32 SquadSize);

private:
	void UpdateState();
};
