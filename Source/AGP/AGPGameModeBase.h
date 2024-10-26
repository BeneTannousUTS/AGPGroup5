#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AGPGameModeBase.generated.h"

/**
 * Custom GameMode for player spawning.
 */
UCLASS()
class AGP_API AAGPGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

/*public:
	AAGPGameModeBase();

	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	virtual void BeginPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;

private:
	TArray<AActor*> AvailablePlayerStarts;
	int32 PlayerIndex = 0;  // Track which PlayerStart to use next

	void RestartPlayerAtStart(AController* Player);*/
};
