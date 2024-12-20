// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseCharacter.h"
<<<<<<< HEAD
#include "HealthComponent.h"
#include "WeaponComponent.h"
#include "Net/UnrealNetwork.h"
=======

#include "HealthComponent.h"
>>>>>>> ui

// Sets default values
ABaseCharacter::ABaseCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	BulletStartPosition = CreateDefaultSubobject<USceneComponent>("Bullet Start");
	BulletStartPosition->SetupAttachment(GetRootComponent());
	HealthComponent = CreateDefaultSubobject<UHealthComponent>("Health Component");

	
}

// Called when the game starts or when spawned
void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();
<<<<<<< HEAD
}

void ABaseCharacter::Fire(const FVector& FireAtLocation)
{
	if (HasWeapon())
	{
		WeaponComponent->Fire(BulletStartPosition->GetComponentLocation(), FireAtLocation);
	}
}

void ABaseCharacter::Reload()
{
	if (HasWeapon())
	{
		WeaponComponent->Reload();
	}
}

void ABaseCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABaseCharacter, WeaponComponent);
=======
	
}

bool ABaseCharacter::Fire(const FVector& FireAtLocation)
{
	// Determine if the character is able to fire.
	if (TimeSinceLastShot < MinTimeBetweenShots)
	{
		return false;
	}

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	if (GetWorld()->LineTraceSingleByChannel(HitResult, BulletStartPosition->GetComponentLocation(), FireAtLocation, ECC_Pawn, QueryParams))
	{
		if (ABaseCharacter* HitCharacter = Cast<ABaseCharacter>(HitResult.GetActor()))
		{
			if (UHealthComponent* HitCharacterHealth = HitCharacter->GetComponentByClass<UHealthComponent>())
			{
				HitCharacterHealth->ApplyDamage(WeaponDamage);
			}
			DrawDebugLine(GetWorld(), BulletStartPosition->GetComponentLocation(), HitResult.ImpactPoint, FColor::Green, false, 1.0f);
		}
		else
		{
			DrawDebugLine(GetWorld(), BulletStartPosition->GetComponentLocation(), HitResult.ImpactPoint, FColor::Orange, false, 1.0f);
		}
		
	}
	else
	{
		DrawDebugLine(GetWorld(), BulletStartPosition->GetComponentLocation(), FireAtLocation, FColor::Red, false, 1.0f);
	}

	TimeSinceLastShot = 0.0f;
	return true;
>>>>>>> ui
}

// Called every frame
void ABaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

<<<<<<< HEAD
	if(HealthComponent->IsDead())
	{
		Destroy();
=======
	if (bHasWeaponEquipped)
	{
		TimeSinceLastShot += DeltaTime;
>>>>>>> ui
	}
}

bool ABaseCharacter::HasWeapon()
{
<<<<<<< HEAD
	return (WeaponComponent != nullptr);
}

bool ABaseCharacter::IsCrouching()
{
	return bIsCrouching;
}


void ABaseCharacter::EquipWeapon(bool bEquipWeapon, const FWeaponStats& WeaponStats)
{
	if(GetLocalRole() == ROLE_Authority)
	{
		if(EquipWeaponImplementation(bEquipWeapon, WeaponStats))
		{
			MulticastEquipWeapon(bEquipWeapon);
		}
	}else if(GetLocalRole() == ROLE_AutonomousProxy)
	{
		ServerEquipWeapon(bEquipWeapon, WeaponStats);
	}
}

// Called to bind functionality to input
void ABaseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

bool ABaseCharacter::EquipWeaponImplementation(bool bEquipWeapon, const FWeaponStats& WeaponStats)
{
	// Create or remove the weapon component depending on whether we are trying to equip a weapon and we
	// don't already have one. Or if we are trying to unequip a weapon and we do have one.
	if (bEquipWeapon && !HasWeapon())
	{
		WeaponComponent = NewObject<UWeaponComponent>(this);
		WeaponComponent->RegisterComponent();
	}
	else if (!bEquipWeapon && HasWeapon())
	{
		WeaponComponent->UnregisterComponent();
		WeaponComponent = nullptr;
	}

	// At this point we should have a WeaponComponent if we are trying to equip a weapon.
	if (HasWeapon())
	{
		// Set the weapons stats to the given weapon stats.
		WeaponComponent->SetWeaponStats(WeaponStats);
	}
	
	//EquipWeaponGraphical(bEquipWeapon);
=======
	return bHasWeaponEquipped;
}

void ABaseCharacter::EquipWeapon(bool bEquipWeapon)
{
	bHasWeaponEquipped = bEquipWeapon;
	EquipWeaponGraphical(bEquipWeapon);
>>>>>>> ui
	if (bEquipWeapon)
	{
		UE_LOG(LogTemp, Display, TEXT("Player has equipped weapon."))
	}
	else
	{
		UE_LOG(LogTemp, Display, TEXT("Player has unequipped weapon."))
	}
<<<<<<< HEAD

	return true;
}

void ABaseCharacter::ServerEquipWeapon_Implementation(bool bEquipWeapon, const FWeaponStats& WeaponStats)
{
	if(EquipWeaponImplementation(bEquipWeapon, WeaponStats))
	{
		MulticastEquipWeapon(bEquipWeapon);
	}
}

void ABaseCharacter::MulticastEquipWeapon_Implementation(bool bEquipWeapon)
{
	//EquipWeaponGraphical(bEquipWeapon);
=======
}

// Called to bind functionality to input
void ABaseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

>>>>>>> ui
}

