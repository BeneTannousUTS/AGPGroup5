// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerCharacter.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "AGP/AGPGameInstance.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

class UAGPGameInstance;
// Sets default values
APlayerCharacter::APlayerCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (const APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(InputMappingContext, 0);
		}
	}

	if (IsLocallyControlled() && PlayerHUDClass)
	{
		if (APlayerController*  PlayerController = Cast<APlayerController>(GetController()))
		{
			PlayerHUD = CreateWidget<UPlayerCharacterHUD>(PlayerController, PlayerHUDClass);
			if (PlayerHUD)
			{
				PlayerHUD->AddToPlayerScreen();
				if (UAGPGameInstance* GameInstance = Cast<UAGPGameInstance>(GetGameInstance()))
				{
					GameInstance->HUD = PlayerHUD;
					FInputModeUIOnly InputMode;
					InputMode.SetWidgetToFocus(PlayerHUD->TakeWidget());
					InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
					PlayerController->SetInputMode(InputMode);
					PlayerController->bShowMouseCursor = true;
				}
			}
		}
	}
}

void APlayerCharacter::SpawnAI(EAIType AIType)
{
	UAGPGameInstance* AGPGameInstance = Cast<UAGPGameInstance>(GetGameInstance());

	if(!AGPGameInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to get UAGPGameInstance"));
		return;
	}
	
	if (HasAuthority())
	{
		AISpawnImplementation(ETeam::Team1, AIType);
	}else
	{
		ServerAISpawn(ETeam::Team2, AIType);
	}
}

// Called every frame
void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Move);
		Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Look);
		Input->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ACharacter::Jump);
		Input->BindAction(FireAction, ETriggerEvent::Triggered, this, &APlayerCharacter::FireWeapon);
	}
}

void APlayerCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();
	const FVector ForwardVector = GetActorForwardVector();
	AddMovementInput(ForwardVector, MovementVector.X);
	const FVector RightVector = GetActorRightVector();
	AddMovementInput(RightVector, MovementVector.Y);
}

void APlayerCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookVector = Value.Get<FVector2D>() * LookSensitivity;
	AddControllerYawInput(LookVector.X);
	AddControllerPitchInput(LookVector.Y);
}

void APlayerCharacter::FireWeapon(const FInputActionValue& Value)
{
	FVector CameraPosition;
	FRotator CameraRotation;
	GetWorld()->GetFirstPlayerController()->GetPlayerViewPoint(CameraPosition, CameraRotation);
	const FVector CameraForward = UKismetMathLibrary::GetForwardVector(CameraRotation);
	if (BulletStartPosition)
	{
		Fire(BulletStartPosition->GetComponentLocation() + 10000.0f * CameraForward);
	}
}

void APlayerCharacter::AISpawnImplementation(ETeam AITeam, EAIType AIType)
{
	// Assuming this is now called on the server
	if (UAGPGameInstance* GameInstance =
		GetWorld()->GetGameInstance<UAGPGameInstance>())
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		FName SpawnTag = (AITeam == ETeam::Team1) ? FName("Team1") : FName("Team2");
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), PlayerStarts);
        
		for (AActor* PlayerStart : PlayerStarts)
		{
			if (PlayerStart->ActorHasTag(SpawnTag))
			{
				// Spawn the AI character at the location of the tagged PlayerStart
				FTransform SpawnTransform = PlayerStart->GetActorTransform();
				AAICharacter* SpawnedAI = GetWorld()->SpawnActor<AAICharacter>(GameInstance->GetAIClass(), SpawnTransform, SpawnParams);

				if (SpawnedAI)
				{
					SpawnedAI->AITeam = AITeam;
					SpawnedAI->SetAIType(AIType);
					UE_LOG(LogTemp, Log, TEXT("Spawned AI for Team %s at %s"), *SpawnTag.ToString(), *SpawnTransform.ToString());
				}
				return; // Exit after spawning at the first valid location
			}
		}

		UE_LOG(LogTemp, Warning, TEXT("No PlayerStart found with tag %s for team spawn"), *SpawnTag.ToString());
	}
}


void APlayerCharacter::ServerAISpawn_Implementation(ETeam AITeam, EAIType AIType)
{
	if (HasAuthority())
	{
		UE_LOG(LogTemp, Log, TEXT("ServerAISpawn called on the server"));
		AISpawnImplementation(AITeam, AIType);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ServerAISpawn was called on a client, but should only run on the server."));
	}
}


