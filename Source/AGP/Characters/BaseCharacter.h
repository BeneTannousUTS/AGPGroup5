// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
<<<<<<< HEAD
#include "WeaponComponent.h"
=======
>>>>>>> ui
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

<<<<<<< HEAD
	UFUNCTION(BlueprintCallable)
	bool IsCrouching();

	void EquipWeapon(bool bEquipWeapon, const FWeaponStats& WeaponStats);
	// UFUNCTION(BlueprintImplementableEvent)
	// void EquipWeaponGraphical(bool bEquipWeapon);

	/**
	 * Will reload the weapon if the character has a weapon equipped.
	 */
	void Reload();

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
=======
	void EquipWeapon(bool bEquipWeapon);
	UFUNCTION(BlueprintImplementableEvent)
	void EquipWeaponGraphical(bool bEquipWeapon);
>>>>>>> ui

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

<<<<<<< HEAD
=======
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

>>>>>>> ui
	/**
	 * A scene component to store the position that hit scan shots start from. For the enemy character this could
	 * be placed close to the guns position for example and for the player character it can be placed close to the
	 * camera position.
	 */
	UPROPERTY(VisibleAnywhere)
	USceneComponent* BulletStartPosition;

<<<<<<< HEAD
	/**
	 * A component that holds information about the health of the character. This component has functions
	 * for damaging the character and healing the character.
	 */
=======
>>>>>>> ui
	UPROPERTY(VisibleAnywhere)
	UHealthComponent* HealthComponent;

	/**
<<<<<<< HEAD
	 * An actor component that controls the logic for this characters equipped weapon.
	 */
	UPROPERTY(Replicated)
	UWeaponComponent* WeaponComponent = nullptr;

	/**
=======
>>>>>>> ui
	 * Will fire at a specific location and handles the impact of the shot such as determining what it hit and
	 * deducting health if it hit a particular type of actor.
	 * @param FireAtLocation The location that you want to fire at.
	 * @return true if a shot was taken and false otherwise.
	 */
<<<<<<< HEAD
	void Fire(const FVector& FireAtLocation);

	bool bIsCrouching = false;
	bool bHasWeapon= true;
=======
	bool Fire(const FVector& FireAtLocation);
>>>>>>> ui

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

<<<<<<< HEAD
private:
	bool EquipWeaponImplementation(bool bEquipWeapon,
		const FWeaponStats& WeaponStats = FWeaponStats());

	UFUNCTION(NetMulticast, Reliable)
	void MulticastEquipWeapon(bool bEquipWeapon);

	UFUNCTION(Server, Reliable)
	void ServerEquipWeapon(bool bEquipWeapon,
		const FWeaponStats& WeaponStats);
=======
>>>>>>> ui
};
