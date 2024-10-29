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

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class AGP_API UWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UWeaponComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	FWeaponStats WeaponStats;
	int32 RoundsRemainingInMagazine;
	float TimeSinceLastShot = 0.0f;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	bool Fire(const FVector& BulletStart, const FVector& FireAtLocation);

	void Reload();

	void CompleteReload();

	int CheckAmmoRemaining() const;

	void SetWeaponStats(const FWeaponStats& NewStats);

	bool IsMagazineEmpty();

private:
	int ShotsLeft;
		
};
