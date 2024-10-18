// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BaseCharacter.generated.h"

class UHealthComponent;

UCLASS()
class AGP_API ABaseCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ABaseCharacter();

	UFUNCTION(BlueprintCallable)
	bool HasWeapon();

	void EquipWeapon(bool bEquipWeapon);
	UFUNCTION(BlueprintImplementableEvent)
	void EquipWeaponGraphical(bool bEquipWeapon);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	bool bHasWeaponEquipped = false;

	/**
	 * Will be updated each frame and be used to determine if a shot can be taken.
	 */
	float TimeSinceLastShot = 0.0f;
	/**
	 * Is the minimum time that needs to have occured between shots.
	 */
	float MinTimeBetweenShots = 0.2f;
	/**
	 * The damage that will be applied to characters that are hit with this weapon.
	 */
	float WeaponDamage = 10.0f;

	/**
	 * A scene component to store the position that hit scan shots start from. For the enemy character this could
	 * be placed close to the guns position for example and for the player character it can be placed close to the
	 * camera position.
	 */
	UPROPERTY(VisibleAnywhere)
	USceneComponent* BulletStartPosition;

	UPROPERTY(VisibleAnywhere)
	UHealthComponent* HealthComponent;

	/**
	 * Will fire at a specific location and handles the impact of the shot such as determining what it hit and
	 * deducting health if it hit a particular type of actor.
	 * @param FireAtLocation The location that you want to fire at.
	 * @return true if a shot was taken and false otherwise.
	 */
	bool Fire(const FVector& FireAtLocation);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
