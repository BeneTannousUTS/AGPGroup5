// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseCharacter.h"

#include "AICharacter.h"
#include "HealthComponent.h"
#include "WeaponComponent.h"
#include "AGP/AGPGameInstance.h"
#include "Net/UnrealNetwork.h"

class UAGPGameInstance;
class AAICharacter;
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
}


// Called every frame
void ABaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if(HealthComponent->IsDead())
	{
		Destroy();
	}
}

bool ABaseCharacter::HasWeapon()
{
	return (WeaponComponent != nullptr);
}

bool ABaseCharacter::IsCrouching()
{
	return bIsCrouching;
}

void ABaseCharacter::EquipWeapon(bool bEquipWeapon, const FWeaponStats& WeaponStats)
{
	if (GetLocalRole() == ROLE_Authority)
	{
		EquipWeaponImplementation(bEquipWeapon, WeaponStats);
		MulticastEquipWeapon(bEquipWeapon);
	}
}

// Called to bind functionality to input
void ABaseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void ABaseCharacter::EquipWeaponImplementation(bool bEquipWeapon, const FWeaponStats& WeaponStats)
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
		UE_LOG(LogTemp, Display, TEXT("Equipping weapon"))
		WeaponComponent->SetWeaponStats(WeaponStats);
	}

	if (bEquipWeapon)
	{
		UE_LOG(LogTemp, Display, TEXT("Player has equipped weapon."))
	}
	else
	{
		UE_LOG(LogTemp, Display, TEXT("Player has unequipped weapon."))
	}
}

void ABaseCharacter::MulticastEquipWeapon_Implementation(bool bEquipWeapon)
{
	//EquipWeaponImplementation(bEquipWeapon, WeaponStats);
	//EquipWeaponGraphical(bEquipWeapon);
}