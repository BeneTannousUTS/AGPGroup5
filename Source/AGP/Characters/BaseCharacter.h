// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WeaponComponent.h"
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

	UFUNCTION(BlueprintCallable)
	bool IsCrouching();

	void EquipWeapon(bool bEquipWeapon, const FWeaponStats& WeaponStats);

	void Reload();

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void Fire(const FVector& FireAtLocation);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere)
	USceneComponent* BulletStartPosition;

	UPROPERTY(VisibleAnywhere)
	UHealthComponent* HealthComponent;

	UPROPERTY(Replicated)
	UWeaponComponent* WeaponComponent = nullptr;

	bool bIsCrouching = false;
	bool bHasWeapon= true;

public:    
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	void EquipWeaponImplementation(bool bEquipWeapon, const FWeaponStats& WeaponStats = FWeaponStats());

	UFUNCTION(NetMulticast, Reliable)
	void MulticastEquipWeapon(bool bEquipWeapon);
};