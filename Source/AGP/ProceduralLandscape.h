// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
<<<<<<< HEAD
=======
#include "GameFramework/PlayerStart.h"
>>>>>>> ui
#include "Pathfinding/NavigationNode.h"
#include "ProceduralLandscape.generated.h"

UCLASS()
class AGP_API AProceduralLandscape : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AProceduralLandscape();
	virtual bool ShouldTickIfViewportsOnly() const override;
	void GenerateBase();
	void GenerateTunnels();
	void GenerateCliffs();
	void GenerateMesh() const;
	void GenerateSafeHouse(FVector SpawnLocation) const;

protected:
<<<<<<< HEAD
=======
	void RespawnServerPlayer();
>>>>>>> ui
	void GenerateTerrain();
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	UPROPERTY(VisibleAnywhere)
	UProceduralMeshComponent* ProceduralMesh;

	void ClearLandscape();
<<<<<<< HEAD
    UPROPERTY()
    TArray<FVector> Vertices;
    UPROPERTY()
    TArray<int32> Triangles;
    UPROPERTY()
=======
    UPROPERTY(ReplicatedUsing = OnRep_TerrainGenerated)
    TArray<FVector> Vertices;
    UPROPERTY(ReplicatedUsing = OnRep_TerrainGenerated)
    TArray<int32> Triangles;
    UPROPERTY(ReplicatedUsing = OnRep_TerrainGenerated)
>>>>>>> ui
    TArray<FVector2D> UVCoords;
	UPROPERTY()
	TArray<ANavigationNode*> Nodes;
	UPROPERTY()
	UPathfindingSubsystem* PathfindingSubsystem;

<<<<<<< HEAD
=======
	UFUNCTION()
	void OnRep_TerrainGenerated();
	void TellClientsToRespawn();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnTerrainGenerated();

>>>>>>> ui
	UPROPERTY(EditAnywhere)
	bool bShouldRegenerate = false;
	float Timer;
	TSet<int32> HoleIndices;
	UPROPERTY(EditAnywhere)
	int32 TunnelLevels = 1;

	UPROPERTY(EditAnywhere)
	int32 MinWidth = 10;
	UPROPERTY(EditAnywhere)
	int32 MinDepth = 10;
	UPROPERTY(EditAnywhere)
	int32 MinHeight = 3;
	UPROPERTY(EditAnywhere)
	int32 MaxWidth = 10;
	UPROPERTY(EditAnywhere)
	int32 MaxDepth = 10;
	UPROPERTY(EditAnywhere)
	int32 MaxHeight = 3;
	UPROPERTY(EditAnywhere)
	float VertexSpacing = 1000.0f;
<<<<<<< HEAD
=======
	UPROPERTY(EditAnywhere)
	TSubclassOf<AActor> SafeHouseBlueprintClass;
>>>>>>> ui

	int32 Width;
	int32 Depth;
	int32 Height;
<<<<<<< HEAD

=======
	
>>>>>>> ui
	/*UPROPERTY(EditAnywhere)
	float PerlinScale = 1000.0f;
	UPROPERTY(EditAnywhere)
	float PerlinRoughness = 0.00012f;
	UPROPERTY(EditAnywhere)
	float NoiseThreshold = 0.1f;
	UPROPERTY(VisibleAnywhere)
	float PerlinOffset;*/

public:
	void RemoveNavNodes();
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:
	void SpawnClimbingRock(FVector SpawnLocation) const;
<<<<<<< HEAD

=======
	void SetPlayerSpawns();
	void MovePlayerStarts(const FVector& Position1, const FVector& Position2, const FRotator& Rotation1,
	                      const FRotator& Rotation2);
	TArray<AActor*> AvailablePlayerStarts;
>>>>>>> ui
};
