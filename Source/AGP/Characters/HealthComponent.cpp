// Fill out your copyright notice in the Description page of Project Settings.


#include "HealthComponent.h"

#include "AICharacter.h"
#include "BehaviourComponent.h"

// Sets default values for this component's properties
UHealthComponent::UHealthComponent(): CurrentHealth(0)
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}

float UHealthComponent::GetMaxHealth() const
{
	return MaxHealth;
}

bool UHealthComponent::IsDead()
{
	return bIsDead;
}

float UHealthComponent::GetCurrentHealth() const
{
	
	return CurrentHealth;
}

float UHealthComponent::GetCurrentHealthPercentage() const
{
	UE_LOG(LogTemp, Warning, TEXT("The character health is %f"), CurrentHealth/MaxHealth);
	return CurrentHealth / MaxHealth;
}

void UHealthComponent::ApplyDamage(float DamageAmount, FVector DamageLocation)
{
	if (bIsDead) return;
	CurrentHealth -= DamageAmount;
	if (CurrentHealth <= 0.0f)
	{
		OnDeath();
		CurrentHealth = 0.0f;
	}

	LastKnownDamageLocation = DamageLocation;
	
	if(Cast<AAICharacter>(GetOwner())->GetWeaponType() == Sniper)
	{
		Cast<AAICharacter>(GetOwner())->GetBehaviourComponent()->SeekVantagePoint(); //Change Vantage point if sniper takes damage
	}
}

void UHealthComponent::ApplyHealing(float HealingAmount)
{
	if (bIsDead) return;
	CurrentHealth += HealingAmount;
	if (CurrentHealth > 100.0f)
	{
		CurrentHealth = 100.0f;
	}
}

FVector UHealthComponent::GetLastKnownDamageLocation()
{
	return LastKnownDamageLocation;
}


// Called when the game starts
void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	CurrentHealth = MaxHealth;
}


void UHealthComponent::OnDeath()
{
	UE_LOG(LogTemp, Display, TEXT("The character has died."))
	bIsDead = true;
}

// Called every frame
void UHealthComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

