// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseCharacter.h"

#include "HealthComponent.h"

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
}

// Called every frame
void ABaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bHasWeaponEquipped)
	{
		TimeSinceLastShot += DeltaTime;
	}
}

bool ABaseCharacter::HasWeapon()
{
	return bHasWeaponEquipped;
}

void ABaseCharacter::EquipWeapon(bool bEquipWeapon)
{
	bHasWeaponEquipped = bEquipWeapon;
	EquipWeaponGraphical(bEquipWeapon);
	if (bEquipWeapon)
	{
		UE_LOG(LogTemp, Display, TEXT("Player has equipped weapon."))
	}
	else
	{
		UE_LOG(LogTemp, Display, TEXT("Player has unequipped weapon."))
	}
}

// Called to bind functionality to input
void ABaseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

