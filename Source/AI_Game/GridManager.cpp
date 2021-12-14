// Fill out your copyright notice in the Description page of Project Settings.
// Fill out your copyright notice in the Description page of Project Settings.


#include "GridManager.h"
#include "Engine/Engine.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GenericPlatform/GenericPlatformMath.h"
#include "Game_AIController.h"
#include "DrawDebugHelpers.h"
#include "Algo/Reverse.h"
#include "AI_GameCharacter.h"

#define ECC_GridTracer ECC_GameTraceChannel1

UCell* AGridManager::GetCellFromCoordinates(int32 x, int32 y) const
{
	if (GridCells.Num() == int32(0)) return nullptr;

	if (x >= CellCount.X || y >= CellCount.Y)
	{
		return nullptr;
	}

	int index = 0;
	index += x * CellCount.Y;
	index += y;

	if (index < GridCells.Num())
	{
		return GridCells[index];
	}

	return nullptr;
}

bool AGridManager::GetCellIndexFromGridPosition(int32& index, int32 x, int32 y) const
{
	if (GridCells.Num() == int32(0))
	{
		UE_LOG(LogTemp, Warning, TEXT("GridCells is empty."));
		return false;
	}
	if (x >= CellCount.X || x < 0 || y >= CellCount.Y || y < 0)
	{
		return false;
	}

	int position = 0;
	position += x * CellCount.Y;
	position += y;

	if (position < GridCells.Num())
	{
		index = position;
		return true;
	}

	UE_LOG(LogTemp, Warning, TEXT("position was bigger then GridCells size."));
	return false;
}

void AGridManager::SetAIControllerReferences()
{
	TArray<AActor*> controllers;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGame_AIController::StaticClass(), controllers);
	for (const auto& controller : controllers)
	{
		Cast<AGame_AIController>(controller)->GridManager = this;
	}
}

void AGridManager::DrawCells()
{
	auto world = GetWorld();
	for (auto const& cell : GridCells)
	{
		GetWorld()->ForegroundLineBatcher->DrawPoint(cell->Location, cell->Color, 50, 0.0f);
	}
}

void AGridManager::CalculateCellsHeights()
{
	auto world = GetWorld();
	FHitResult outResult;
	FCollisionQueryParams params;
	FVector start;
	FVector end;
	FVector safetyAjust;

	for (auto const& cell : GridCells)
	{
		start = FVector(cell->Location.X, cell->Location.Y, GetActorLocation().Z + LinceTraceHeight);
		end = FVector(cell->Location.X, cell->Location.Y, GetActorLocation().Z - LinceTraceHeight);
		if (world->LineTraceSingleByChannel(outResult, start, end, ECC_Visibility, params))
		{
			cell->Location = FVector(outResult.ImpactPoint);
			for (float i = -1.0f; i <= 1.0f; i++)
			{
				for (float j = -1.0f; j <= 1.0f; j++)
				{
					if (i == 0 && j == 0) continue;
					safetyAjust = FVector(i * (CellRadius + SafetyRadius), j * (CellRadius + SafetyRadius), 0.0f);
					if (world->LineTraceSingleByChannel(outResult, start + safetyAjust, end + safetyAjust, ECC_Visibility, params))
					{
						if (abs(outResult.ImpactPoint.Z - cell->Location.Z) > MaxTraversableSlope)
						{
							cell->State = BLOCKED;
							cell->SetColorByState();
							i = j = 2.0f;
						}
					}
					else
					{
						cell->State = BLOCKED;
						cell->SetColorByState();
					}
				}
			}
		}
		else
		{
			cell->State = BLOCKED;
			cell->SetColorByState();
		}
	}
}

void AGridManager::SetGridSize(float x, float y, float z)
{
	if (x >= 0) GridSize.X = x;
	if (y >= 0) GridSize.Y = y;
	if (z >= 0) GridSize.Z = z;
}

void AGridManager::SetCellCount(int32 x, int32 y, int32 z)
{
	if (x >= 0) CellCount.X = x;
	if (y >= 0) CellCount.Y = y;
	if (z >= 0) CellCount.Z = z;
}

void AGridManager::SetCellRadius(float radius)
{
	CellRadius = radius;
}

void AGridManager::CreateCells()
{
	FVector SpawnLocation;
	FVector StartLocation = GetActorLocation() - CollisionBox->GetScaledBoxExtent();

	for (int i = 0; i < CellCount.X; i++)
	{
		for (int j = 0; j < CellCount.Y; j++)
		{
			SpawnLocation = StartLocation + FVector(CellRadius * i, CellRadius * j, StartLocation.Z);
			UCell* SpawnedCell = NewObject<UCell>();
			SpawnedCell->Coordinates = FIntVector(i, j, 0);
			SpawnedCell->Index = GridCells.Num();
			SpawnedCell->Location = SpawnLocation;
			GridCells.Add(SpawnedCell);
		}
	}
}

void AGridManager::CheckCellBlocks()
{
	for (auto& cell : GridCells)
	{
		if (CollisionChecker)
		{
			FVector location = cell->Location + FVector(0.0f, 0.0f, CellRadius * 0.5f);
			CollisionChecker->SetWorldLocation(location);
			TArray<UPrimitiveComponent*> overlappingComponents;
			CollisionChecker->GetOverlappingComponents(overlappingComponents);

			for (auto& component : overlappingComponents)
			{
				if (component->GetCollisionObjectType() == ECC_WorldStatic && component->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block)
				{
					cell->State = BLOCKED;
					cell->SetColorByState();
					break;
				}
			}
		}
	}
}

void AGridManager::CalculateSizes()
{
	float gridSizeX = 2 * CollisionBox->GetScaledBoxExtent().X;
	float gridSizeY = 2 * CollisionBox->GetScaledBoxExtent().Y;
	float gridSizeZ = 2 * CollisionBox->GetScaledBoxExtent().Z;

	GridSize = FVector(gridSizeX, gridSizeY, gridSizeZ);

	int32 cellCountX = FGenericPlatformMath::TruncToInt(gridSizeX / CellRadius);
	if (cellCountX < 1) cellCountX = 1;
	int32 cellCountY = FGenericPlatformMath::TruncToInt(gridSizeY / CellRadius);
	if (cellCountY < 1) cellCountY = 1;
	int32 cellCountZ = FGenericPlatformMath::TruncToInt(gridSizeZ / CellRadius);
	if (cellCountZ < 1) cellCountZ = 1;

	CellCount = FIntVector(cellCountX, cellCountY, cellCountZ);
}

void AGridManager::SetCell(int32 cellIndex, TEnumAsByte<ECellState> state, float moveCost, int32 modifierPriority)
{
	GridCells[cellIndex]->SetCellParameters(state, moveCost, modifierPriority);
}

UCell* AGridManager::GetClosestCellFromLocation(const FVector& location) const
{
	FVector relativeLocation = location - GetActorLocation();

	float percentX = FMath::Clamp(((relativeLocation.X + GridSize.X / 2) / GridSize.X), 0.0f, 1.0f);
	float percentY = FMath::Clamp(((relativeLocation.Y + GridSize.Y / 2) / GridSize.Y), 0.0f, 1.0f);

	int x = FMath::RoundToInt((CellCount.X - 1) * percentX);
	int y = FMath::RoundToInt((CellCount.Y - 1) * percentY);


	return GetCellFromCoordinates(x, y);
}

float AGridManager::GetDistanceBetweenCells(const UCell* cellA, const UCell* cellB, const bool& diagonal, const bool& vertical) const
{
	FIntVector distance = cellA->Coordinates - cellB->Coordinates;
	distance.X = abs(distance.X);
	distance.Y = abs(distance.Y);
	distance.Z = abs(distance.Z);

	if (diagonal)
	{
		int32 bigger, middle, smaller;

		if (distance.X > distance.Y)
		{
			if (distance.X > distance.Z)
			{
				bigger = distance.X;
				if (distance.Y > distance.Z)
				{
					middle = distance.Y;
					smaller = distance.Z;
				}
				else
				{
					middle = distance.Z;
					smaller = distance.Y;
				}
			}
			else
			{
				bigger = distance.Z;
				middle = distance.X;
				smaller = distance.Y;
			}
		}
		else
		{
			if (distance.X < distance.Z)
			{
				smaller = distance.X;
				if (distance.Y > distance.Z)
				{
					bigger = distance.Y;
					middle = distance.Z;
				}
				else
				{
					bigger = distance.Z;
					middle = distance.Y;
				}
			}
			else
			{
				bigger = distance.Y;
				middle = distance.X;
				smaller = distance.Z;
			}
		}

		return (smaller * Diagonal3DCostMultiplier) + ((middle - smaller) * DiagonalCostMultiplier) + (bigger - middle);
	}
	else
	{
		return distance.X + distance.Y + distance.Z;
	}
}

void AGridManager::SetCellNeighbors(UCell* cell)
{
	int32 index = 0;

	for (int i = -1; i <= 1; i += 2)
	{
		if (GetCellIndexFromGridPosition(index, cell->Coordinates.X + i, cell->Coordinates.Y))
		{
			if(abs(GridCells[index]->Location.Z - cell->Location.Z) < MaxTraversableSlope) cell->Neighbors.Add(GridCells[index]);
		}
		if (GetCellIndexFromGridPosition(index, cell->Coordinates.X, cell->Coordinates.Y + i))
		{
			if (abs(GridCells[index]->Location.Z - cell->Location.Z) < MaxTraversableSlope) cell->Neighbors.Add(GridCells[index]);
		}

		if (CanMoveOnDiagonals)
		{
			if (GetCellIndexFromGridPosition(index, cell->Coordinates.X + i, cell->Coordinates.Y + i))
			{
				if (abs(GridCells[index]->Location.Z - cell->Location.Z) < MaxTraversableSlope) cell->Neighbors.Add(GridCells[index]);

			}
			if (GetCellIndexFromGridPosition(index, cell->Coordinates.X - i, cell->Coordinates.Y + i))
			{
				if (abs(GridCells[index]->Location.Z - cell->Location.Z) < MaxTraversableSlope) cell->Neighbors.Add(GridCells[index]);
			}
		}
	}
}

void AGridManager::SetAllCellNeighbors()
{
	for (auto& cell : GridCells)
	{
		SetCellNeighbors(cell);
	}
}

bool AGridManager::FindPathByCell(FPath& outPath, UCell* startCell, UCell* targetCell)
{
	TSet<UCell*> openSet;
	TSet<UCell*> closedSet;
	TSet<UCell*> searchedCells;

	openSet.Add(startCell);

	while (openSet.Num() > 0)
	{
		UCell* currentCell = NewObject<UCell>();
		for (const auto& cell : openSet) { currentCell = cell; break; }

		for (const auto& cell : openSet)
		{
			if (cell->FCost() < currentCell->FCost() || (cell->FCost() == currentCell->FCost() && cell->HCost < currentCell->HCost))
			{
				currentCell = cell;
			}
		}

		openSet.Remove(currentCell);
		closedSet.Add(currentCell);

		if (currentCell == targetCell)
		{
			while (currentCell != startCell)
			{
				outPath.CellsInPath.Add(currentCell);
				outPath.CellCosts.Add(currentCell->GCost);
				currentCell = GridCells[currentCell->ParentIndex];
			}

			Algo::Reverse(outPath.CellsInPath);
			Algo::Reverse(outPath.CellCosts);
			for (auto& cell : searchedCells) cell->GCost = 0;
			return true;
		}

		for (auto& cell : currentCell->GetNeighbors(CanMoveOnDiagonals, CanMoveVertically))
		{
			if (cell->State == ECellState::BLOCKED || closedSet.Contains(cell)) continue;

			float newGCost = currentCell->GCost + GetDistanceBetweenCells(currentCell, cell) + cell->MoveCost;
			if (!openSet.Contains(cell) || cell->GCost > newGCost)
			{
				cell->GCost = newGCost;
				cell->HCost = GetDistanceBetweenCells(cell, targetCell);
				cell->ParentIndex = currentCell->Index;

				if (!openSet.Contains(cell))
				{
					openSet.Add(cell);
					searchedCells.Add(cell);
				}
			}
		}
	}

	return false;
}

bool AGridManager::FindPathByCoordinate(FPath& outPath, const FIntVector& start, const FIntVector& end)
{

	UCell* startCell = GetCellFromCoordinates(start.X, start.Y);
	if (!startCell)
	{
		UE_LOG(LogTemp, Warning, TEXT("Start location not valid!"));
		return false;
	}

	UCell* targetCell = GetCellFromCoordinates(end.X, end.Y);
	if (!targetCell)
	{
		UE_LOG(LogTemp, Warning, TEXT("End location not valid!"));
		return false;
	}

	return FindPathByCell(outPath, startCell, targetCell);
}

bool AGridManager::FindPathByLocation(FPath& outPath, const FVector& start, const FVector& end)
{
	UCell* startCell = GetClosestCellFromLocation(start);
	if (!startCell)
	{
		UE_LOG(LogTemp, Warning, TEXT("Start location not valid!"));
		return false;
	}

	UCell* targetCell = GetClosestCellFromLocation(end);
	if (!targetCell)
	{
		UE_LOG(LogTemp, Warning, TEXT("End location not valid!"));
		return false;
	}
	return FindPathByCell(outPath, startCell, targetCell);
}

bool AGridManager::SetCellColor(int32 index, FColor color)
{
	if (!GridCells[index])
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, FString::Printf(TEXT("Invalid index: %f"), index));
		return false;
	}

	//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, FString::Printf(TEXT("Valid index: %f"), index));
	GridCells[index]->Color = color;
	return true;
}

UCell* AGridManager::GetRandomCell()
{
	UCell* cell;
	static FRandomStream random;
	do { cell = GridCells[random.RandRange(0, GridCells.Num() - 1)]; } while (cell->State != ECellState::FREE);
	return cell;
}


// Sets default values
AGridManager::AGridManager()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision Box"));
	CollisionBox->SetRelativeLocation(FVector(0));
	CollisionBox->SetCollisionProfileName("NoCollision");
	SetRootComponent(CollisionBox);

	CollisionChecker = CreateDefaultSubobject<USphereComponent>(TEXT("Collision Checker"));
	CollisionChecker->SetCollisionProfileName("OverlapAll");
}

// Called when the game starts or when spawned
void AGridManager::BeginPlay()
{
	Super::BeginPlay();

	CalculateSizes();
	CreateCells();
	CalculateCellsHeights();
	SetAllCellNeighbors();
	SetAIControllerReferences();
}

// Called every frame
void AGridManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	//DrawCells();
}

