// Fill out your copyright notice in the Description page of Project Settings.


#include "ProceduralLandscape.h"
#include "KismetProceduralMeshLibrary.h"
#include "MultiplayerGameMode.h"
#include "Characters/PlayerCharacter.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Pathfinding/NavigationNode.h"
#include "Pathfinding/PathfindingSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

// Sets default values
AProceduralLandscape::AProceduralLandscape()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	SetReplicates(true);

	ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Procedural Mesh"));
	SetRootComponent(ProceduralMesh);
}

// Called when the game starts or when spawned
void AProceduralLandscape::BeginPlay()
{
	Super::BeginPlay();
	PathfindingSubsystem = GetWorld()->GetSubsystem<UPathfindingSubsystem>();
	GenerateTerrain();
}

void AProceduralLandscape::ClearLandscape()
{
	Vertices.Empty();
	Triangles.Empty();
	UVCoords.Empty();
	HoleIndices.Empty();

	if (Nodes.Num() > 0)
	{
		TArray<AActor*> NodesInScene;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ANavigationNode::StaticClass(), NodesInScene);
		for (AActor* FoundActor : NodesInScene)
		{
			FoundActor->Destroy();
		}
		
		Nodes.Empty();
	}
	
	TArray<AActor*> SafeHouses;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("SafeHouse"), SafeHouses);
	for (AActor* FoundActor : SafeHouses)
	{
		FoundActor->Destroy();
	}

	TArray<AActor*> Rocks;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("RockObstacle"), Rocks);
	for (AActor* FoundActor : Rocks)
	{
		FoundActor->Destroy();
	}

	// Destory old climbing rocks

	if (UWorld* World = GetWorld())
	{
		TArray<AActor*> FoundActors;
		UGameplayStatics::GetAllActorsWithTag(World, FName("ClimbingRock"), FoundActors);
		for (AActor* FoundActor : FoundActors)
		{
			FoundActor->Destroy();
		}
	}
	
	
	if (!PathfindingSubsystem) PathfindingSubsystem = GetWorld()->GetSubsystem<UPathfindingSubsystem>();
	PathfindingSubsystem->UpdatesNodes(Nodes);
	ProceduralMesh->ClearMeshSection(0);
	UKismetSystemLibrary::FlushPersistentDebugLines(GetWorld());
}

void AProceduralLandscape::OnRep_TerrainGenerated()
{
	GenerateMesh();
}

void AProceduralLandscape::RemoveNavNodes()
{
	// removing all obstacle nodes :)

	for (ANavigationNode* Node : Nodes)
	{
		if (Node->NodeType == ENavigationNodeType::OBSTACLE)
		{
			for (ANavigationNode* ConnectedNode : Node->ConnectedNodes)
			{
				ConnectedNode->ConnectedNodes.Remove(Node);
			}
			Node->ConnectedNodes.Empty();
			Node->Destroy();
		}
	}
}

// Called every frame
void AProceduralLandscape::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Timer += DeltaTime;

	if (bShouldRegenerate)
	{
		GenerateTerrain();
		bShouldRegenerate = false;

		/*for (FVector VertexLoc : Vertices)
		{
			DrawDebugSphere(GetWorld(), VertexLoc, 50.0f,4, FColor::Blue, true);
		}*/
	}
}

void AProceduralLandscape::SpawnClimbingRock(FVector SpawnLocation) const
{
	UWorld* World = GetWorld();

	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("World is null."));
		return;
	}

	if (AActor* CubeActor = World->SpawnActor<AActor>(AActor::StaticClass()))
	{
		CubeActor->Tags.Add(FName("ClimbingRock"));

		float Size = 0.75f;

		UStaticMeshComponent* CubeMesh = NewObject<UStaticMeshComponent>(CubeActor);
		if (!CubeMesh)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create CubeMesh."));
			return;
		}

		CubeMesh->RegisterComponent();
		CubeActor->SetRootComponent(CubeMesh);
		CubeMesh->SetWorldLocation(SpawnLocation);

		UStaticMesh* CubeAsset = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		if (!CubeAsset)
		{
			return;
		}
		CubeMesh->SetStaticMesh(CubeAsset);

		UMaterialInterface* CubeMaterial = LoadObject<UMaterialInterface>(
			nullptr, TEXT("/Script/Engine.Material'/Game/Materials/StoneMaterial.StoneMaterial'"));
		if (!CubeMaterial)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to load Cube material."));
			return;
		}
		CubeMesh->SetMaterial(0, CubeMaterial);

		CubeMesh->SetWorldScale3D(FVector(Size, Size, Size));
	}
}

void AProceduralLandscape::SetPlayerSpawns()
{
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), AvailablePlayerStarts);

	if (AvailablePlayerStarts.Num() == 2)
	{
		// Move the two PlayerStarts to new positions
		MovePlayerStarts(
			FVector(VertexSpacing * Width / 2.0f, -500.0f, 8000.0f), FVector(VertexSpacing * Width / 2.0f, VertexSpacing * (Depth - 1) + 500.0f, 8000.0f), 
			FRotator(0.0f, 0, 0.0f), FRotator(0.0f, 0.0f, 0.0f)
		);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Not 2 Player Starts"));
	}
}

void AProceduralLandscape::MovePlayerStarts(const FVector& Position1, const FVector& Position2, const FRotator& Rotation1, const FRotator& Rotation2)
{
	if (AvailablePlayerStarts.Num() < 2)
	{
		UE_LOG(LogTemp, Error, TEXT("Insufficient PlayerStart actors to move!"));
		return;
	}

	// Move the first PlayerStart
	APlayerStart* PlayerStart1 = Cast<APlayerStart>(AvailablePlayerStarts[0]);
	if (PlayerStart1)
	{
		if (PlayerStart1->GetRootComponent())
		{
			PlayerStart1->GetRootComponent()->SetMobility(EComponentMobility::Movable);
		}
		
		PlayerStart1->SetActorLocationAndRotation(Position1, Rotation1);
		UE_LOG(LogTemp, Log, TEXT("Moved PlayerStart 1 to (%s)"), *Position1.ToString());
	}

	// Move the second PlayerStart
	APlayerStart* PlayerStart2 = Cast<APlayerStart>(AvailablePlayerStarts[1]);
	if (PlayerStart2)
	{
		if (PlayerStart2->GetRootComponent())
		{
			PlayerStart2->GetRootComponent()->SetMobility(EComponentMobility::Movable);
		}
		
		PlayerStart2->SetActorLocationAndRotation(Position2, Rotation2);
		UE_LOG(LogTemp, Log, TEXT("Moved PlayerStart 2 to (%s)"), *Position2.ToString());
	}
}

void AProceduralLandscape::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AProceduralLandscape, Vertices);
	DOREPLIFETIME(AProceduralLandscape, Triangles);
	DOREPLIFETIME(AProceduralLandscape, UVCoords);
}

void AProceduralLandscape::Multicast_OnTerrainGenerated_Implementation()
{
	GenerateMesh();

	/*APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (PlayerController && !HasAuthority())  // Ensure it's not the server
	{
		if (AMultiplayerGameMode* GameMode = Cast<AMultiplayerGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
		{
			GameMode->RespawnPlayer(PlayerController, 1);
			UE_LOG(LogTemp, Log, TEXT("Client respawned at index: %d"), 1);
		}
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("CALLED ON SERVER"));
	}*/
}

bool AProceduralLandscape::ShouldTickIfViewportsOnly() const
{
	return true;
}

void AProceduralLandscape::GenerateBase()
{
	int32 HoleCount = 0;
	int32 MaxHoles = 2;

	UE_LOG(LogTemp, Display, TEXT("Width: %i"), Width);
	UE_LOG(LogTemp, Display, TEXT("Depth: %i"), Depth);

	//FVector SpawnLocation = FVector((Width - 1) * VertexSpacing / 2, (Depth - 5) * VertexSpacing, Size * 100.0f / 2);
	
	TSet<int32> SafeHouseIndices;
	TSet<int32> ObstacleIndices;
	int32 BackSafeHouseStartingRowIndex = (Depth  - 6) * Width - 1;
	int32 StartSafeHouseStartingRowIndex = 2 * Width - 1;

	for (int32 j = 0; j < 4; ++j)
	{
		for (int32 i = 0; i < 3; ++i)
		{
			SafeHouseIndices.Append({BackSafeHouseStartingRowIndex + Width / 2 + i + j * Width});
			SafeHouseIndices.Append({StartSafeHouseStartingRowIndex + Width / 2 + i + j * Width});
		}
	}
	if (Width % 2 == 0)
	{
		int32 i = -1;
		for (int32 j = 0; j < 4; ++j)
		{
			SafeHouseIndices.Append({BackSafeHouseStartingRowIndex + Width / 2 + i + j * Width});
			SafeHouseIndices.Append({StartSafeHouseStartingRowIndex + Width / 2 + i + j * Width});
		}
	}

	int32 MidDepth = Depth / 2;
	for (int32 i = 0; i < 1; ++i)
	{
		int RandomEarlyRow = FMath::RandRange(1, MidDepth);
		int RandomCol1 = FMath::RandRange(1, Width - 2);
		int Index1 = RandomEarlyRow * Width + RandomCol1;

		int RandomLateRow = FMath::RandRange(MidDepth, static_cast<int32>(MidDepth * 1.5f));
		int RandomCol2 = FMath::RandRange(1, Width - 2);
		int Index2 = RandomLateRow * Width + RandomCol2;
		
		HoleIndices.Add(Index1);
		HoleIndices.Add(Index2);
	}
	
	
	for (int32 i = 0; i < 7; ++i)
	{
		ObstacleIndices.Add(FMath::RandRange(0,Depth * Width - 1));
	}

	for (int32 Index : SafeHouseIndices)
	{
		ObstacleIndices.Remove(Index);
	}

	for (int32 Index : HoleIndices)
	{
		ObstacleIndices.Remove(Index);
	}
	
	for (int32 DepthIndex = 0; DepthIndex < Depth; ++DepthIndex)
	{
		for (int32 WidthIndex = 0; WidthIndex < Width; ++WidthIndex)
		{
			float XPos = WidthIndex * VertexSpacing;
			float YPos = DepthIndex * VertexSpacing;
			float ZPos = 0.0f;

			FVector VertexLoc = FVector(XPos, YPos, ZPos);

			Vertices.Add(VertexLoc);
			UVCoords.Add(FVector2D(static_cast<float>(WidthIndex), static_cast<float>(DepthIndex)));

			bool bCanGenerateNavNode = true;
			
			if (!(DepthIndex == 0 || WidthIndex == 0 || DepthIndex == Depth - 1 || WidthIndex == Width - 1))
			{
				// Generate Nav Node if it is not a hole

				int32 VertexIndex = Width * DepthIndex + WidthIndex;
				if (ObstacleIndices.Contains(VertexIndex))
				{
					bCanGenerateNavNode = false;

					if (AActor* CubeActor = GetWorld()->SpawnActor<AActor>(AActor::StaticClass()))
					{
						CubeActor->Tags.Add(FName("RockObstacle"));

						int32 Size = 3;
						UStaticMeshComponent* CubeMesh = NewObject<UStaticMeshComponent>(CubeActor);
						CubeMesh->SetupAttachment(CubeActor->GetRootComponent());
						CubeActor->SetRootComponent(CubeMesh);
						CubeMesh->RegisterComponent();
						CubeMesh->SetWorldLocation(VertexLoc + FVector(0.0f,0.0f,Size * 50.0f));

						if (UStaticMesh* CubeAsset = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")))
						{
							CubeMesh->SetStaticMesh(CubeAsset);
						}

						CubeMesh->SetWorldScale3D(FVector(Size, Size, Size * 1.25f));

						UMaterialInterface* CubeMaterial = LoadObject<UMaterialInterface>(
							nullptr, TEXT("/Script/Engine.Material'/Game/Materials/RockMaterial.RockMaterial'"));

						if (CubeMaterial)
						{
							CubeMesh->SetMaterial(0, CubeMaterial);
						}

						ANavigationNode* Node = GetWorld()->SpawnActor<ANavigationNode>();
						Node->SetActorLocation(VertexLoc);
						Node->NodeType = ENavigationNodeType::OBSTACLE;
						Nodes.Add(Node);
					}
				}

				if (HoleIndices.Contains(VertexIndex))
				{
					bCanGenerateNavNode = false;
					ANavigationNode* Node = GetWorld()->SpawnActor<ANavigationNode>();
					Node->SetActorLocation(VertexLoc);
					Node->NodeType = ENavigationNodeType::CLIMBINGDOWN;
					Nodes.Add(Node);
					//DrawDebugSphere(GetWorld(), Vertices[VertexIndex], 75.0f,4, FColor::Magenta, true);
				}

				/*if (HoleCount < MaxHoles && bCanGenerateNavNode)
				{
					if (HoleCount == 0 && DepthIndex < MidDepth * 0.25f) // Early hole
					{
						float EarlyHoleChance = FMath::FRandRange(0.0f, 1.0f);
						if (EarlyHoleChance < 0.05f)
						{
							++HoleCount;
							bCanGenerateNavNode = false;

							ANavigationNode* Node = GetWorld()->SpawnActor<ANavigationNode>();
							Node->SetActorLocation(VertexLoc);
							Node->NodeType = ENavigationNodeType::CLIMBINGDOWN;
							Nodes.Add(Node);
							
							HoleIndices.Add(VertexIndex);
						}
					} else if (HoleCount == 1 && DepthIndex >= MidDepth * 1.5f) // Late hole
					{
						float LateHoleChance = FMath::FRandRange(0.0f, 1.0f);
						if (LateHoleChance < 0.05f)
						{
							++HoleCount;
							bCanGenerateNavNode = false;

							ANavigationNode* Node = GetWorld()->SpawnActor<ANavigationNode>();
							Node->SetActorLocation(VertexLoc);
							Node->NodeType = ENavigationNodeType::CLIMBINGDOWN;
							Nodes.Add(Node);
							
							HoleIndices.Add(VertexIndex);
						}
					}
				}*/

				if (bCanGenerateNavNode) // Not hole so generate nav node yippee!!!
				{
					// Add additional check to make sure nodes do not spawn inside the safe house?

					ANavigationNode* Node = GetWorld()->SpawnActor<ANavigationNode>();
					Node->SetActorLocation(VertexLoc);
					Nodes.Add(Node);
				}
				
			}
		}
	}

	// Adding nav node connections

	int32 NavDepth = Depth - 2;
	int32 NavWidth = Width - 2;
	
	// inside square
	for (int32 Y = 1; Y < NavDepth - 1; ++Y)
	{
		for (int32 X = 1; X < NavWidth - 1; ++X)
		{
			int32 Index = Y * NavWidth + X;
			Nodes[Index]->ConnectedNodes.Append(
				{
					Nodes[Index - 1],
					Nodes[Index - 1 + NavWidth],
					Nodes[Index + NavWidth],
					Nodes[Index + 1 + NavWidth],
					Nodes[Index + 1],
					Nodes[Index + 1 - NavWidth],
					Nodes[Index - NavWidth],
					Nodes[Index - 1 - NavWidth]
				});
		}
	}

	// corners
	Nodes[0]->ConnectedNodes.Append({Nodes[1], Nodes[NavWidth], Nodes[NavWidth + 1]});
	Nodes[NavWidth - 1]->ConnectedNodes.Append({Nodes[NavWidth - 2], Nodes[2 * NavWidth - 1], Nodes[2 * NavWidth - 2]});
	Nodes[(NavDepth - 1) * NavWidth + 0]->ConnectedNodes.Append({
		Nodes[(NavDepth - 1) * NavWidth + 1], Nodes[(NavDepth - 1) * NavWidth + 1 - NavWidth], Nodes[(NavDepth - 1) * NavWidth - NavWidth]
	});
	Nodes[(NavDepth - 1) * NavWidth + (NavWidth - 1)]->ConnectedNodes.Append({
		Nodes[(NavDepth - 1) * NavWidth + (NavWidth - 2)], Nodes[(NavDepth - 1) * NavWidth + (NavWidth - 1) - NavWidth],
		Nodes[(NavDepth - 1) * NavWidth + (NavWidth - 1) - NavWidth - 1]
	});

	// right edge
	int32 RightEdgeX = 0;
	for (int32 Y = 1; Y < NavDepth - 1; ++Y)
	{
		int32 Index = Y * NavWidth + RightEdgeX;
		Nodes[Index]->ConnectedNodes.Append(
			{
				Nodes[Index + NavWidth],
				Nodes[Index + 1 + NavWidth],
				Nodes[Index + 1],
				Nodes[Index + 1 - NavWidth],
				Nodes[Index - NavWidth]
			});
	}

	//left edge
	int32 LeftEdgeX = NavWidth - 1;
	for (int32 Y = 1; Y < NavDepth - 1; ++Y)
	{
		int32 Index = Y * NavWidth + LeftEdgeX;
		Nodes[Index]->ConnectedNodes.Append(
			{
				Nodes[Index + NavWidth],
				Nodes[Index - 1 + NavWidth],
				Nodes[Index - 1],
				Nodes[Index - 1 - NavWidth],
				Nodes[Index - NavWidth]
			});
	}

	//bottom edge
	int32 BottomEdgeY = 0;
	for (int32 X = 1; X < NavWidth - 1; ++X)
	{
		int32 Index = BottomEdgeY * NavWidth + X;
		Nodes[Index]->ConnectedNodes.Append(
			{
				Nodes[Index - 1],
				Nodes[Index - 1 + NavWidth],
				Nodes[Index + NavWidth],
				Nodes[Index + 1 + NavWidth],
				Nodes[Index + 1]
			});
	}

	//top edge
	int32 TopEdgeY = NavDepth - 1;
	for (int32 X = 1; X < NavWidth - 1; ++X)
	{
		int32 Index = TopEdgeY * NavWidth + X;
		Nodes[Index]->ConnectedNodes.Append(
			{
				Nodes[Index - 1],
				Nodes[Index - 1 - NavWidth],
				Nodes[Index - NavWidth],
				Nodes[Index + 1 - NavWidth],
				Nodes[Index + 1]
			});
	}

	// filling in the triangles but skipping the holes
	
	for (int32 Y = 0; Y < Depth - 1; ++Y)
	{
		for (int32 X = 0; X < Width - 1; ++X)
		{
			int32 VertexIndex = Y * Width + X;
			if (HoleIndices.Contains(VertexIndex) || HoleIndices.Contains(VertexIndex + Width) || HoleIndices.Contains(
				VertexIndex + 1))
			{
				continue;
			}
			Triangles.Append({VertexIndex, VertexIndex + Width, VertexIndex + 1});
		}

		for (int32 X = 1; X < Width; ++X)
		{
			int32 VertexIndex = Y * Width + X;
			if (HoleIndices.Contains(VertexIndex) || HoleIndices.Contains(VertexIndex + Width - 1) || HoleIndices.
				Contains(VertexIndex + Width))
			{
				continue;
			}
			Triangles.Append({VertexIndex, VertexIndex + Width - 1, VertexIndex + Width});
		}
	}

	// filling in the missing triangles around the holes

	for (int32 HoleIndex : HoleIndices)
	{
		Triangles.Append({HoleIndex - Width, HoleIndex + 1, HoleIndex - Width + 1});
		Triangles.Append({HoleIndex - 1, HoleIndex + Width - 1, HoleIndex + Width});
	}

	// Holes

	for (int32 HoleIndex : HoleIndices)
	{
		//DrawDebugSphere(GetWorld(), Vertices[HoleIndex], 50.0f,4, FColor::Green, true);

		int32 LayerAboveUpIndex = HoleIndex + Width;
		int32 LayerAboveLeftIndex = HoleIndex + 1;
		int32 LayerAboveDownIndex = HoleIndex - Width;
		int32 LayerAboveRightIndex = HoleIndex - 1;
		FVector UpHolePosition = Vertices[LayerAboveUpIndex];
		FVector LeftHolePosition = Vertices[LayerAboveLeftIndex];
		FVector DownHolePosition = Vertices[LayerAboveDownIndex];
		FVector RightHolePosition = Vertices[LayerAboveRightIndex];


		for (int32 Level = 0; Level <= TunnelLevels; ++Level)
		{
			int32 LayerUpIndex = Vertices.Num();
			int32 LayerLeftIndex = Vertices.Num() + 1;
			int32 LayerDownIndex = Vertices.Num() + 2;
			int32 LayerRightIndex = Vertices.Num() + 3;
			Vertices.Add(UpHolePosition + FVector(0, 0, -1 * Level * VertexSpacing));
			Vertices.Add(LeftHolePosition + FVector(0, 0, -1 * Level * VertexSpacing));
			Vertices.Add(DownHolePosition + FVector(0, 0, -1 * Level * VertexSpacing));
			Vertices.Add(RightHolePosition + FVector(0, 0, -1 * Level * VertexSpacing));

			float UUp = 0.5f;
			float ULeft = 0.0f;
			float UDown = 0.5f;
			float URight = 1.0f;

			float V = static_cast<float>(Level) / TunnelLevels;

			UVCoords.Add(FVector2D(UUp, V));
			UVCoords.Add(FVector2D(ULeft, V));
			UVCoords.Add(FVector2D(UDown, V));
			UVCoords.Add(FVector2D(URight, V));

			Triangles.Append({LayerUpIndex, LayerAboveUpIndex, LayerAboveLeftIndex});
			Triangles.Append({LayerUpIndex, LayerAboveLeftIndex, LayerLeftIndex});
			Triangles.Append({LayerLeftIndex, LayerAboveLeftIndex, LayerAboveDownIndex});
			Triangles.Append({LayerLeftIndex, LayerAboveDownIndex, LayerDownIndex});
			Triangles.Append({LayerDownIndex, LayerAboveDownIndex, LayerAboveRightIndex});
			Triangles.Append({LayerDownIndex, LayerAboveRightIndex, LayerRightIndex});
			Triangles.Append({LayerRightIndex, LayerAboveRightIndex, LayerAboveUpIndex});
			Triangles.Append({LayerRightIndex, LayerAboveUpIndex, LayerUpIndex});
		}
	}
	
	
}

void AProceduralLandscape::GenerateTunnels()
{
	int32 LastHoleUpIndex = Vertices.Num() - 4;
	int32 FirstHoleUpIndex = Vertices.Num() - 4 * (TunnelLevels + 2);
	int32 FirstTunnelVertex = Vertices.Num();

	//DrawDebugSphere(GetWorld(), Vertices[FirstHoleUpIndex], 75.0f,4, FColor::Magenta, true);
	
	int32 StartOfHoleNavIndices = Nodes.Num();

	// Generate Climb Up Nav Nodes for Tunnels n Connect them to above Node

	for (int32 HoleIndex : HoleIndices)
	{
		// get loc
		FVector ClimbUpHoleLoc = Vertices[HoleIndex] - FVector(0.0f,0.0f, VertexSpacing * 1.5f);
		// make nav node
		ANavigationNode* HoleNavNode = GetWorld()->SpawnActor<ANavigationNode>();
		HoleNavNode->SetActorLocation(ClimbUpHoleLoc);
		HoleNavNode->NodeType = ENavigationNodeType::CLIMBINGUP;
		Nodes.Add(HoleNavNode);
		// connect to above
		int32 AboveHoleNavNodeIndex = (HoleIndex - Width) - 2 * ((HoleIndex - Width) / Width) - 1;
		ANavigationNode* AboveHoleNavNode = Nodes[AboveHoleNavNodeIndex];
		AboveHoleNavNode->ConnectedNodes.Add(HoleNavNode);
		HoleNavNode->ConnectedNodes.Add(AboveHoleNavNode);
	}

	//DrawDebugSphere(GetWorld(), Vertices[LastHoleUpIndex], 75.0f,4, FColor::Red, true);

	TSet<int32> ConnectedHoles;
	float TunnelRadius = VertexSpacing;
	float StepSize = VertexSpacing;

	TArray<TPair<int32, int32>> HolePairs = {
		{HoleIndices.Array()[0], HoleIndices.Array()[1]}
	};

	for (int32 HoleIndex : HoleIndices)
	{
		int32 TargetHoleIndex;
		int32 Attempts = 0;
		do
		{
			TargetHoleIndex = HoleIndices.Array()[FMath::RandRange(0, HoleIndices.Num() - 1)];
			Attempts++;
		}
		while (TargetHoleIndex == HoleIndex && Attempts < 50);

		if (Attempts >= 50)
		{
			UE_LOG(LogTemp, Warning, TEXT("Two unique holes couldn't be found in 50 attempts. Ending tunnel generation process."));
			return;
		}

		if (ConnectedHoles.Contains(HoleIndex) || ConnectedHoles.Contains(TargetHoleIndex))
		{
			continue;
		}

		ConnectedHoles.Add(HoleIndex);
		ConnectedHoles.Add(TargetHoleIndex);

		//int32 StartHoleIndex = HolePair.Key;
		//int32 TargetHoleIndex = HolePair.Value;

		FVector StartPos = Vertices[HoleIndex] - FVector(0.0f, 0.0f, VertexSpacing * (TunnelLevels + 1.0f));
		FVector EndPos = Vertices[TargetHoleIndex] - FVector(0.0f, 0.0f, VertexSpacing * (TunnelLevels + 0.5f));

		FVector TunnelDirection = (EndPos - StartPos).GetSafeNormal();
		float TunnelLength = (EndPos - StartPos).Size();

		int32 PreviousRingStartIndex = -1;
		ANavigationNode* PrevNavNode = nullptr;

		for (float Step = 0.0f; Step < TunnelLength - VertexSpacing; Step += StepSize)
		{
			FVector CurrentPos = StartPos + TunnelDirection * Step + FVector(FMath::RandRange(-0.25f,0.25f) * VertexSpacing,FMath::RandRange(-0.1f,0.1f) * VertexSpacing, 0.0f);
			int32 CurrentRingStartIndex = Vertices.Num();

			if (Step != 0)
			{
				FVector NavLoc = CurrentPos - FVector(0,0,TunnelRadius * 0.8f);

				ANavigationNode* Node = GetWorld()->SpawnActor<ANavigationNode>();
				Node->SetActorLocation(NavLoc);
				Node->NodeType = ENavigationNodeType::CRAWLING;
				Nodes.Add(Node);

				// ADD CONNECTIONS
				if (PrevNavNode)
				{
					PrevNavNode->ConnectedNodes.Add(Node);
					Node->ConnectedNodes.Add(PrevNavNode);
				}

				PrevNavNode = Node;

				if (Step == StepSize) // aka the first step with a node
				{
					Node->ConnectedNodes.Add(Nodes[StartOfHoleNavIndices]);
					Nodes[StartOfHoleNavIndices]->ConnectedNodes.Add(Node);
				}
			}
			
			for (int32 Vertex = 0; Vertex < 6; ++Vertex)
			{
				FVector VertexVector = CurrentPos + FVector(TunnelRadius * FMath::Cos(Vertex * PI / 3.0f), 0.0f,
				                                            TunnelRadius * FMath::Sin(Vertex * PI / 3.0f)) +
																TunnelDirection * StepSize + FVector(FMath::RandRange(-0.2f,0.2f) * VertexSpacing);
				Vertices.Add(VertexVector);
				UVCoords.Add(FVector2D(0.0f, 0.0f));
			}

			if (PreviousRingStartIndex != -1)
			{
				for (int32 i = 0; i < 6; ++i)
				{
					int32 CurrentVertex1 = CurrentRingStartIndex + i;
					int32 CurrentVertex2 = CurrentRingStartIndex + ((i + 1) % 6);
					int32 PreviousVertex1 = PreviousRingStartIndex + i;
					int32 PreviousVertex2 = PreviousRingStartIndex + ((i + 1) % 6);

					Triangles.Append({PreviousVertex1, CurrentVertex1, CurrentVertex2});
					Triangles.Append({PreviousVertex1, CurrentVertex2, PreviousVertex2});
				}
			}
			PreviousRingStartIndex = CurrentRingStartIndex;
		}
		PrevNavNode->ConnectedNodes.Add(Nodes[StartOfHoleNavIndices + 1]);
		Nodes[StartOfHoleNavIndices + 1]->ConnectedNodes.Add(PrevNavNode);
	}

	int32 LastTunnelVertex = Vertices.Num() - 6;

	//DrawDebugSphere(GetWorld(), Vertices[LastTunnelVertex], 75.0f,4, FColor::Green, true);

	Triangles.Append({FirstTunnelVertex + 5, FirstHoleUpIndex + 1, FirstHoleUpIndex + 2});
	Triangles.Append({FirstTunnelVertex + 4, FirstTunnelVertex + 5, FirstHoleUpIndex + 2});
	Triangles.Append({FirstTunnelVertex + 4, FirstHoleUpIndex + 2, FirstHoleUpIndex + 3});
	Triangles.Append({FirstTunnelVertex + 4, FirstHoleUpIndex + 3, FirstTunnelVertex + 3});
	Triangles.Append({FirstTunnelVertex + 3, FirstHoleUpIndex + 3, FirstHoleUpIndex});
	Triangles.Append({FirstTunnelVertex + 5, FirstTunnelVertex, FirstHoleUpIndex + 1});
	Triangles.Append({FirstTunnelVertex, FirstHoleUpIndex, FirstHoleUpIndex + 1});

	Triangles.Append({LastTunnelVertex + 5, LastHoleUpIndex, LastHoleUpIndex + 1});
	Triangles.Append({LastTunnelVertex + 4, LastHoleUpIndex, LastTunnelVertex + 5});
	Triangles.Append({LastTunnelVertex + 4, LastHoleUpIndex + 3, LastHoleUpIndex});
	Triangles.Append({LastTunnelVertex + 5, LastHoleUpIndex + 1, LastHoleUpIndex + 2});
	Triangles.Append({LastTunnelVertex, LastHoleUpIndex + 1, LastHoleUpIndex + 2});
	Triangles.Append({LastTunnelVertex + 4, LastTunnelVertex + 3, LastHoleUpIndex + 3});
	Triangles.Append({LastTunnelVertex + 3, LastHoleUpIndex + 2, LastHoleUpIndex + 3});

	//Triangles.Append({FirstTunnelVertex + 5, FirstTunnelVertex, FirstHoleUpIndex + 1});
	//Triangles.Append({FirstTunnelVertex, FirstTunnelVertex + 1, FirstHoleUpIndex + 1});
}

void AProceduralLandscape::GenerateCliffs()
{
	// RIGHT CLIFF BOTTOM FACE

	int32 RightCliffVertex = Vertices.Num();

	for (int32 HeightIndex = 0; HeightIndex <= Height; ++HeightIndex)
	{
		//float V = static_cast<float>(HeightIndex);
		FVector StartingHeightPosition = FVector(0, 0, HeightIndex * VertexSpacing);
		for (int32 DepthIndex = 0; DepthIndex < Depth; ++DepthIndex)
		{
			if (HeightIndex == 0 || DepthIndex == 0 || DepthIndex == Depth - 1)
			{
				Vertices.Add(StartingHeightPosition + FVector(0, DepthIndex * VertexSpacing, 0));
			}
			else
			{
				Vertices.Add(StartingHeightPosition + FVector(FMath::RandRange(-0.4f, 0.4f) * VertexSpacing,
				                                              DepthIndex * VertexSpacing, 0));
			}

			float U = static_cast<float>(Depth - DepthIndex);
			float V = static_cast<float>(Height - HeightIndex);

			UVCoords.Add(FVector2D(U, V));
		}
	}

	for (int32 HeightIndex = 0; HeightIndex < Height; ++HeightIndex)
	{
		for (int32 DepthIndex = 0; DepthIndex < Depth - 1; ++DepthIndex)
		{
			Triangles.Append({
				RightCliffVertex + DepthIndex + HeightIndex * Depth,
				RightCliffVertex + DepthIndex + (HeightIndex + 1) * Depth,
				RightCliffVertex + DepthIndex + HeightIndex * Depth + 1
			});
			Triangles.Append({
				RightCliffVertex + DepthIndex + HeightIndex * Depth + 1,
				RightCliffVertex + DepthIndex + (HeightIndex + 1) * Depth,
				RightCliffVertex + DepthIndex + (HeightIndex + 1) * Depth + 1
			});
		}
	}

	// GENERATE RIGHT CLIMBING ROCKS

	int32 RandRightBottomVertex = FMath::RandRange(RightCliffVertex + 1, RightCliffVertex + Depth - 2);
	for (int32 HeightIndex = 0; HeightIndex < Height; ++HeightIndex)
	{
		if (HeightIndex != 0)
		{
			SpawnClimbingRock(FMath::Lerp(Vertices[RandRightBottomVertex + Depth * HeightIndex],
			                              Vertices[RandRightBottomVertex + 1 + Depth * HeightIndex],
			                              FMath::RandRange(0.25f, 0.5f)));
		}
		SpawnClimbingRock(FMath::Lerp(Vertices[RandRightBottomVertex + Depth * HeightIndex],
		                              Vertices[RandRightBottomVertex - 1 + Depth * (HeightIndex + 1)],
		                              FMath::RandRange(0.25f, 0.6f)));
	}

		// bottom nav node

	ANavigationNode* RightCliffBottomNode = GetWorld()->SpawnActor<ANavigationNode>();
	RightCliffBottomNode->NodeType = ENavigationNodeType::CLIMBINGUP;
	RightCliffBottomNode->SetActorLocation(Vertices[RandRightBottomVertex] + FVector(VertexSpacing * 0.25f, 0.0f, 0.0f));
	Nodes.Add(RightCliffBottomNode);
	int32 RightDepthIndexOffset = static_cast<int32>(Vertices[RandRightBottomVertex].Y / VertexSpacing);
	ANavigationNode* RCB_ConnectedNode = Nodes[(RightDepthIndexOffset - 1) * (Width - 2)];
	RCB_ConnectedNode->ConnectedNodes.Add(RightCliffBottomNode);
	RightCliffBottomNode->ConnectedNodes.Add(RCB_ConnectedNode);
	

	// RIGHT CLIFF GROUND

	RightCliffVertex = Vertices.Num() - Depth;
	ANavigationNode* PrevNavNode = nullptr;

	for (int32 DepthIndex = 0; DepthIndex < Depth; ++DepthIndex)
	{
		Vertices.Add(FVector(0, 0, Height * VertexSpacing) + FVector(-2 * VertexSpacing, DepthIndex * VertexSpacing, 0));

		float U = static_cast<float>(DepthIndex);
		float V = static_cast<float>(0);

		UVCoords.Add(FVector2D(U, V));

		// generate nav nodes !!!

		FVector NavLoc = FVector(FVector(0, 0, Height * VertexSpacing) + FVector(-1 * VertexSpacing, DepthIndex * VertexSpacing, 0));

		ANavigationNode* Node = GetWorld()->SpawnActor<ANavigationNode>();
		Node->SetActorLocation(NavLoc);

		Nodes.Add(Node);

		// ADD CONNECTIONS !!!!
		if (PrevNavNode)
		{
			PrevNavNode->ConnectedNodes.Add(Node);
			Node->ConnectedNodes.Add(PrevNavNode);
		}

		PrevNavNode = Node;

		if (DepthIndex == RightDepthIndexOffset)
		{
			ANavigationNode* RightCliffTopNode = GetWorld()->SpawnActor<ANavigationNode>();
			RightCliffTopNode->NodeType = ENavigationNodeType::CLIMBINGDOWN;
			RightCliffTopNode->SetActorLocation(Vertices[RightCliffVertex + RightDepthIndexOffset] + FVector(VertexSpacing * 0.25f, 0.0f, 0.0f));
			Nodes.Add(RightCliffTopNode);
			
			Node->ConnectedNodes.Add(RightCliffTopNode);
			RightCliffTopNode->ConnectedNodes.Add(Node);
			RightCliffTopNode->ConnectedNodes.Add(RightCliffBottomNode);
			RightCliffBottomNode->ConnectedNodes.Add(RightCliffTopNode);
		}
	}
	PrevNavNode = nullptr;

	for (int32 DepthIndex = 0; DepthIndex < Depth - 1; ++DepthIndex)
	{
		Triangles.Append({
			RightCliffVertex + DepthIndex,
			RightCliffVertex + Depth + DepthIndex,
			RightCliffVertex + 1 + DepthIndex});
		Triangles.Append({
			RightCliffVertex + 1 + DepthIndex,
			RightCliffVertex + Depth + DepthIndex,
			RightCliffVertex + Depth + DepthIndex + 1});
	}

	// RIGHT CLIFF TOP FACE

	RightCliffVertex = Vertices.Num();

	for (int32 HeightIndex = 0; HeightIndex <= Height; ++HeightIndex)
	{
		float V = static_cast<float>(HeightIndex);
		FVector StartingHeightPosition = FVector(-2 * VertexSpacing, 0,
		                                         Height * VertexSpacing + HeightIndex * VertexSpacing);
		for (int32 DepthIndex = 0; DepthIndex < Depth; ++DepthIndex)
		{
			if (HeightIndex == 0 || DepthIndex == 0 || DepthIndex == Depth - 1)
			{
				Vertices.Add(StartingHeightPosition + FVector(0, DepthIndex * VertexSpacing, 0));
			}
			else
			{
				Vertices.Add(StartingHeightPosition + FVector(FMath::RandRange(-0.4f, 0.4f) * VertexSpacing,
				                                              DepthIndex * VertexSpacing, 0));
			}

			float U = static_cast<float>(DepthIndex);

			UVCoords.Add(FVector2D(U, V));
		}
	}

	for (int32 HeightIndex = 0; HeightIndex < Height; ++HeightIndex)
	{
		for (int32 DepthIndex = 0; DepthIndex < Depth - 1; ++DepthIndex)
		{
			Triangles.Append({
				RightCliffVertex + DepthIndex + HeightIndex * Depth,
				RightCliffVertex + DepthIndex + (HeightIndex + 1) * Depth,
				RightCliffVertex + DepthIndex + HeightIndex * Depth + 1
			});
			Triangles.Append({
				RightCliffVertex + DepthIndex + HeightIndex * Depth + 1,
				RightCliffVertex + DepthIndex + (HeightIndex + 1) * Depth,
				RightCliffVertex + DepthIndex + (HeightIndex + 1) * Depth + 1
			});
		}
	}

	/*
	
	// CONNECTION BETWEEN RIGHT AND BACK

	int32 RConnectionIndex = Vertices.Num();

	FVector RConnectionStartingPos = FVector(0, (Depth - 1) * VertexSpacing, Height * VertexSpacing);

	Vertices.Add(RConnectionStartingPos);
	Vertices.Add(RConnectionStartingPos + FVector(-2 * VertexSpacing, 0, 0));
	Vertices.Add(RConnectionStartingPos + FVector(-2 * VertexSpacing, 0, Height * VertexSpacing));
	Vertices.Add(RConnectionStartingPos + FVector(0, 0, Height * VertexSpacing));

	UVCoords.Add(FVector2D(0.0f, 0.0f));
	UVCoords.Add(FVector2D(static_cast<float>(Height), 0.0f));
	UVCoords.Add(FVector2D(static_cast<float>(Height), static_cast<float>(Height)));
	UVCoords.Add(FVector2D(0.0f, static_cast<float>(Height)));

	Triangles.Append({RConnectionIndex, RConnectionIndex + 1, RConnectionIndex + 2});
	Triangles.Append({RConnectionIndex, RConnectionIndex + 2, RConnectionIndex + 3});

	// BACK CLIFF FACE

	int32 BackCliffVertex = Vertices.Num();

	for (int32 HeightIndex = 0; HeightIndex <= Height * 2; ++HeightIndex)
	{
		float V = static_cast<float>(HeightIndex);
		FVector StartingHeightPosition = FVector(0, (Depth - 1) * VertexSpacing, HeightIndex * VertexSpacing);
		for (int32 WidthIndex = 0; WidthIndex < Width; ++WidthIndex)
		{
			if (HeightIndex == 0 || WidthIndex == 0 || WidthIndex == Width - 1)
			{
				Vertices.Add(StartingHeightPosition + FVector(WidthIndex * VertexSpacing, 0, 0));
			}
			else
			{
				Vertices.Add(StartingHeightPosition + FVector(WidthIndex * VertexSpacing,
				                                              FMath::RandRange(-0.4f, 0.4f) * VertexSpacing, 0));
			}

			float U = static_cast<float>(WidthIndex);

			UVCoords.Add(FVector2D(U, V));
		}
	}

	for (int32 HeightIndex = 0; HeightIndex < Height * 2; ++HeightIndex)
	{
		for (int32 WidthIndex = 0; WidthIndex < Width - 1; ++WidthIndex)
		{
			Triangles.Append({
				BackCliffVertex + WidthIndex + HeightIndex * Width,
				BackCliffVertex + WidthIndex + (HeightIndex + 1) * Width,
				BackCliffVertex + WidthIndex + HeightIndex * Width + 1
			});
			Triangles.Append({
				BackCliffVertex + WidthIndex + HeightIndex * Width + 1,
				BackCliffVertex + WidthIndex + (HeightIndex + 1) * Width,
				BackCliffVertex + WidthIndex + (HeightIndex + 1) * Width + 1
			});
		}
	}

	*/

	// LEFT BOTTOM CLIFF FACE

	int32 LeftCliffVertex = Vertices.Num();

	for (int32 HeightIndex = 0; HeightIndex <= Height; ++HeightIndex)
	{
		float V = static_cast<float>(HeightIndex);
		FVector StartingHeightPosition = FVector((Width - 1) * VertexSpacing, (Depth - 1) * VertexSpacing,
		                                         HeightIndex * VertexSpacing);
		for (int32 DepthIndex = 0; DepthIndex < Depth; ++DepthIndex)
		{
			if (HeightIndex == 0 || DepthIndex == 0 || DepthIndex == Depth - 1)
			{
				Vertices.Add(StartingHeightPosition - FVector(0, DepthIndex * VertexSpacing, 0));
			}
			else
			{
				Vertices.Add(StartingHeightPosition - FVector(FMath::RandRange(-0.4f, 0.4f) * VertexSpacing,
				                                              DepthIndex * VertexSpacing, 0));
			}

			float U = static_cast<float>(DepthIndex);

			UVCoords.Add(FVector2D(U, V));
		}
	}

	for (int32 HeightIndex = 0; HeightIndex < Height; ++HeightIndex)
	{
		for (int32 DepthIndex = 0; DepthIndex < Depth - 1; ++DepthIndex)
		{
			Triangles.Append({
				LeftCliffVertex + DepthIndex + HeightIndex * Depth,
				LeftCliffVertex + DepthIndex + (HeightIndex + 1) * Depth,
				LeftCliffVertex + DepthIndex + HeightIndex * Depth + 1
			});
			Triangles.Append({
				LeftCliffVertex + DepthIndex + HeightIndex * Depth + 1,
				LeftCliffVertex + DepthIndex + (HeightIndex + 1) * Depth,
				LeftCliffVertex + DepthIndex + (HeightIndex + 1) * Depth + 1
			});
		}
	}

	// GENERATE LEFT CLIMBING ROCKS

	int32 RandLeftBottomVertex = FMath::RandRange(LeftCliffVertex + 1, LeftCliffVertex + Depth - 2);
	for (int32 HeightIndex = 0; HeightIndex < Height; ++HeightIndex)
	{
		if (HeightIndex != 0)
		{
			SpawnClimbingRock(FMath::Lerp(Vertices[RandLeftBottomVertex + Depth * HeightIndex],
			                              Vertices[RandLeftBottomVertex + 1 + Depth * HeightIndex],
			                              FMath::RandRange(0.25f, 0.5f)));
		}
		SpawnClimbingRock(FMath::Lerp(Vertices[RandLeftBottomVertex + Depth * HeightIndex],
		                              Vertices[RandLeftBottomVertex - 1 + Depth * (HeightIndex + 1)],
		                              FMath::RandRange(0.25f, 0.6f)));
	}

	// bottom nav node

	ANavigationNode* LeftCliffBottomNode = GetWorld()->SpawnActor<ANavigationNode>();
	LeftCliffBottomNode->NodeType = ENavigationNodeType::CLIMBINGUP;
	LeftCliffBottomNode->SetActorLocation(Vertices[RandLeftBottomVertex] - FVector(VertexSpacing * 0.25f, 0.0f, 0.0f));
	Nodes.Add(LeftCliffBottomNode);
	int32 LeftDepthIndexOffset = static_cast<int32>(Vertices[RandLeftBottomVertex].Y / VertexSpacing);
	ANavigationNode* LCB_ConnectedNode = Nodes[(LeftDepthIndexOffset - 1) * (Width - 2) + Width - 3];
	LCB_ConnectedNode->ConnectedNodes.Add(LeftCliffBottomNode);
	LeftCliffBottomNode->ConnectedNodes.Add(LCB_ConnectedNode);

	// LEFT CLIFF GROUND

	LeftCliffVertex = Vertices.Num() - Depth;

	for (int32 DepthIndex = 0; DepthIndex < Depth; ++DepthIndex)
	{
		Vertices.Add(FVector((Width - 1) * VertexSpacing + 2 * VertexSpacing, (Depth - 1) * VertexSpacing,Height * VertexSpacing) - FVector(0, DepthIndex * VertexSpacing, 0));

		float U = static_cast<float>(DepthIndex);
		float V = static_cast<float>(0);

		UVCoords.Add(FVector2D(U, V));

		// generate nav nodes !!!

		FVector NavLoc = FVector(FVector(Width * VertexSpacing, (Depth - 1) * VertexSpacing, Height * VertexSpacing) - FVector(0, DepthIndex * VertexSpacing, 0));

		ANavigationNode* Node = GetWorld()->SpawnActor<ANavigationNode>();
		Node->SetActorLocation(NavLoc);

		Nodes.Add(Node);

		// ADD CONNECTIONS !!!!
		if (PrevNavNode)
		{
			PrevNavNode->ConnectedNodes.Add(Node);
			Node->ConnectedNodes.Add(PrevNavNode);
		}

		PrevNavNode = Node;

		if (DepthIndex == Depth - LeftDepthIndexOffset - 1)
		{
			ANavigationNode* LeftCliffTopNode = GetWorld()->SpawnActor<ANavigationNode>();
			LeftCliffTopNode->NodeType = ENavigationNodeType::CLIMBINGDOWN;
			LeftCliffTopNode->SetActorLocation(Vertices[LeftCliffVertex + Depth - LeftDepthIndexOffset - 1] - FVector(VertexSpacing * 0.25f, 0.0f, 0.0f));
			Nodes.Add(LeftCliffTopNode);
			
			Node->ConnectedNodes.Add(LeftCliffTopNode);
			LeftCliffTopNode->ConnectedNodes.Add(Node);
			LeftCliffTopNode->ConnectedNodes.Add(LeftCliffBottomNode);
			LeftCliffBottomNode->ConnectedNodes.Add(LeftCliffTopNode);
		}
	}

	for (int32 DepthIndex = 0; DepthIndex < Depth - 1; ++DepthIndex)
	{
		Triangles.Append({
			LeftCliffVertex + DepthIndex, LeftCliffVertex + Depth + DepthIndex, LeftCliffVertex + 1 + DepthIndex
		});
		Triangles.Append({
			LeftCliffVertex + 1 + DepthIndex, LeftCliffVertex + Depth + DepthIndex,
			LeftCliffVertex + Depth + DepthIndex + 1
		});
	}

	// LEFT TOP CLIFF FACE

	LeftCliffVertex = Vertices.Num();

	for (int32 HeightIndex = 0; HeightIndex <= Height; ++HeightIndex)
	{
		float V = static_cast<float>(HeightIndex);
		FVector StartingHeightPosition = FVector((Width - 1) * VertexSpacing + 2 * VertexSpacing,
		                                         (Depth - 1) * VertexSpacing,
		                                         Height * VertexSpacing + HeightIndex * VertexSpacing);
		for (int32 DepthIndex = 0; DepthIndex < Depth; ++DepthIndex)
		{
			if (HeightIndex == 0 || DepthIndex == 0 || DepthIndex == Depth - 1)
			{
				Vertices.Add(StartingHeightPosition - FVector(0, DepthIndex * VertexSpacing, 0));
			}
			else
			{
				Vertices.Add(StartingHeightPosition - FVector(FMath::RandRange(-0.4f, 0.4f) * VertexSpacing,
				                                              DepthIndex * VertexSpacing, 0));
			}

			float U = static_cast<float>(DepthIndex);

			UVCoords.Add(FVector2D(U, V));
		}
	}

	for (int32 HeightIndex = 0; HeightIndex < Height; ++HeightIndex)
	{
		for (int32 DepthIndex = 0; DepthIndex < Depth - 1; ++DepthIndex)
		{
			Triangles.Append({
				LeftCliffVertex + DepthIndex + HeightIndex * Depth,
				LeftCliffVertex + DepthIndex + (HeightIndex + 1) * Depth,
				LeftCliffVertex + DepthIndex + HeightIndex * Depth + 1
			});
			Triangles.Append({
				LeftCliffVertex + DepthIndex + HeightIndex * Depth + 1,
				LeftCliffVertex + DepthIndex + (HeightIndex + 1) * Depth,
				LeftCliffVertex + DepthIndex + (HeightIndex + 1) * Depth + 1
			});
		}
	}

	/*
	
	// CONNECTION BETWEEN LEFT AND BACK

	int32 LConnectionIndex = Vertices.Num();

	FVector LConnectionStartingPos = FVector((Width - 1) * VertexSpacing, (Depth - 1) * VertexSpacing,
	                                         Height * VertexSpacing);

	Vertices.Add(LConnectionStartingPos);
	Vertices.Add(LConnectionStartingPos + FVector(2 * VertexSpacing, 0, 0));
	Vertices.Add(LConnectionStartingPos + FVector(2 * VertexSpacing, 0, Height * VertexSpacing));
	Vertices.Add(LConnectionStartingPos + FVector(0, 0, Height * VertexSpacing));

	UVCoords.Add(FVector2D(0.0f, 0.0f));
	UVCoords.Add(FVector2D(static_cast<float>(Height), 0.0f));
	UVCoords.Add(FVector2D(static_cast<float>(Height), static_cast<float>(Height)));
	UVCoords.Add(FVector2D(0.0f, static_cast<float>(Height)));

	Triangles.Append({LConnectionIndex + 1, LConnectionIndex, LConnectionIndex + 3});
	Triangles.Append({LConnectionIndex + 1, LConnectionIndex + 3, LConnectionIndex + 2});
	*/
}

void AProceduralLandscape::GenerateMesh() const
{
	if (ProceduralMesh)
	{
		TArray<FVector> Normals;
		TArray<FProcMeshTangent> Tangents;

		UKismetProceduralMeshLibrary::CalculateTangentsForMesh(Vertices, Triangles, UVCoords, Normals, Tangents);
		ProceduralMesh->CreateMeshSection(0, Vertices, Triangles, Normals, UVCoords, TArray<FColor>(), Tangents, true);
	}
}

void AProceduralLandscape::GenerateSafeHouse(FVector SpawnLocation) const
{
	if (UWorld* World = GetWorld())
	{
		// Check if the SafeHouseBlueprintClass is set
		if (SafeHouseBlueprintClass)
		{
			TArray<AActor*> ExistingSafeHouses;
			UGameplayStatics::GetAllActorsWithTag(World, FName("SafeHouse"), ExistingSafeHouses);
			
			//FRotator SpawnRotation = (!ExistingSafeHouses.IsEmpty()) ? FRotator(0, 180, 0) : FRotator::ZeroRotator;
			
			if (AActor* SafeHouseActor = World->SpawnActor<AActor>(SafeHouseBlueprintClass, SpawnLocation, FRotator(0,0,0)))
			{
				SafeHouseActor->Tags.Add(FName("SafeHouse"));
				
				int32 Size = 15;
				FVector AdjustedLocation = SpawnLocation + FVector(0, 0, Size * 100.0f / 2);
				SafeHouseActor->SetActorLocation(AdjustedLocation);
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("SafeHouseBlueprintClass is not set. Cannot spawn SafeHouse."));
		}
	}
}

void AProceduralLandscape::RespawnServerPlayer()
{
	if (!HasAuthority()) return;  // Ensure only the server executes this

	// Get the server's player controller (usually the first controller)
	APlayerController* ServerController = GetWorld()->GetFirstPlayerController();
	if (!ServerController) 
	{
		UE_LOG(LogTemp, Warning, TEXT("No server player controller found!"));
		return;
	}

	// Get the GameMode and call the respawn logic
	if (AMultiplayerGameMode* GameMode = Cast<AMultiplayerGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
	{
		GameMode->RespawnPlayer(ServerController, 0);
		UE_LOG(LogTemp, Log, TEXT("Server player respawned!"));
	}
}

void AProceduralLandscape::GenerateTerrain()
{
	if (!HasAuthority()) return;
	
	Width = FMath::RandRange(MinWidth, MaxWidth);
	Depth = FMath::RandRange(MinDepth, MaxDepth);
	Height = FMath::RandRange(MinHeight, MaxHeight);

	SetPlayerSpawns();
	ClearLandscape();
	GenerateBase();
	GenerateTunnels();
	GenerateCliffs();
	GenerateMesh();
	GenerateSafeHouse(FVector((Width - 1) * VertexSpacing / 2, (Depth - 5) * VertexSpacing + VertexSpacing / 2.0f, 0));
	GenerateSafeHouse(FVector((Width - 1) * VertexSpacing / 2, 3 * VertexSpacing + VertexSpacing / 2.0f, 0));
	RemoveNavNodes();
	PathfindingSubsystem->UpdatesNodes(Nodes);

	if (AMultiplayerGameMode* GameMode = Cast<AMultiplayerGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
	{
		GameMode->PlayerStartLocations = AvailablePlayerStarts;
		RespawnServerPlayer();
	}

	Multicast_OnTerrainGenerated();
}
