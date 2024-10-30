// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WeaponComponent.generated.h"

UENUM()
enum EWeaponType:uint8
{
	Rifle,
	Pistol,
	Sniper
};

USTRUCT(BlueprintType)
struct FWeaponStats
{
	GENERATED_BODY()
public:
	EWeaponType WeaponType = EWeaponType::Rifle;
	float Accuracy = 0.3f;
	float FireRate = 0.7f;
	float BaseDamage = 7.0f;
	int32 MagazineSize = 30;
	float ReloadTime = 2.5f;
};

UENUM(BlueprintType)
enum class EHitObjectType : uint8
{
	BaseCharacter,
	Ground,
	None
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class AGP_API UWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UWeaponComponent();

	void Fire(const FVector& BulletStart, const FVector& FireAtLocation);
	/**
	 * Starts the process of reloading.
	 */
	void Reload();
	void SetWeaponStats(const FWeaponStats& WeaponInfo);

	bool IsMagazineEmpty();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	EWeaponType GetWeaponType();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	UPROPERTY(Replicated)
	FWeaponStats WeaponStats;
	UPROPERTY(Replicated)
	int32 RoundsRemainingInMagazine;
	float TimeSinceLastShot;
	bool bIsReloading = false;
	
	UPROPERTY(Replicated)
	EHitObjectType OutHitObject = EHitObjectType::None;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	int ShotsLeft;

	/**
	 * Called after the reload has been started delayed by the weapon stats reload time.
	 */
	void CompleteReload();
	float CurrentReloadDuration = 0.0f;

	bool FireImplementation(const FVector& BulletStart, const FVector& FireAtLocation, FVector& OutHitLocation, FVector& OutHitNormal);
	void FireVisualImplementation(const FVector& BulletStart, const FVector& HitLocation, const FVector& HitNormal);
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastFire(const FVector& BulletStart, const FVector& HitLocation, const FVector& HitNormal);
	UFUNCTION(Server, Reliable)
	void ServerFire(const FVector& BulletStart, const FVector& FireAtLocation);

	// RELOAD FUNCTIONS
	void ReloadImplementation();
	UFUNCTION(Server, Reliable)
	void ServerReload();
};
