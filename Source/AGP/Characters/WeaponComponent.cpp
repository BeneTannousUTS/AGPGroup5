// Fill out your copyright notice in the Description page of Project Settings.


#include "WeaponComponent.h"

#include "BaseCharacter.h"
#include "HealthComponent.h"


// Sets default values for this component's properties
UWeaponComponent::UWeaponComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	
	// ...
}


// Called when the game starts
void UWeaponComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UWeaponComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	TimeSinceLastShot += DeltaTime;
}

bool UWeaponComponent::Fire(const FVector& BulletStart, const FVector& FireAtLocation)
{
	if(ShotsLeft > 0)
	{
		if (TimeSinceLastShot < WeaponStats.FireRate)
		{
			return false;
		}

		FHitResult HitResult;
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(GetOwner());
		if (GetWorld()->LineTraceSingleByChannel(HitResult, BulletStart, FireAtLocation, ECC_WorldStatic, QueryParams))
		{
			if (ABaseCharacter* HitCharacter = Cast<ABaseCharacter>(HitResult.GetActor()))
			{
				if (UHealthComponent* HitCharacterHealth = HitCharacter->GetComponentByClass<UHealthComponent>())
				{
					HitCharacterHealth->ApplyDamage(WeaponStats.BaseDamage);
				}
				DrawDebugLine(GetWorld(), BulletStart, HitResult.ImpactPoint, FColor::Green, false, 1.0f);
			}
			else
			{
				DrawDebugLine(GetWorld(), BulletStart, HitResult.ImpactPoint, FColor::Orange, false, 1.0f);
			}
		
		}
		else
		{
			DrawDebugLine(GetWorld(), BulletStart, FireAtLocation, FColor::Red, false, 1.0f);
		}

		TimeSinceLastShot = 0.0f;
		ShotsLeft -= 1;
		return true;
	}
	return false;
}

void UWeaponComponent::Reload()
{
	FTimerHandle ReloadTimerHandle;

	GetWorld()->GetTimerManager().SetTimer(ReloadTimerHandle, this, &UWeaponComponent::CompleteReload, WeaponStats.ReloadTime);
}

void UWeaponComponent::CompleteReload()
{
	ShotsLeft = WeaponStats.MagazineSize;
}

int UWeaponComponent::CheckAmmoRemaining() const
{
	return ShotsLeft;
}

void UWeaponComponent::SetWeaponStats(const FWeaponStats& NewStats)
{
	WeaponStats = NewStats;
}

bool UWeaponComponent::IsMagazineEmpty()
{
	if(CheckAmmoRemaining() > 0)
	{
		return false;
	}
	return true;
}

EWeaponType UWeaponComponent::GetWeaponType()
{
	return WeaponStats.WeaponType;
}
