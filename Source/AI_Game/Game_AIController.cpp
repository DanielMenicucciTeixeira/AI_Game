// Fill out your copyright notice in the Description page of Project Settings.


#include "Game_AIController.h"
#include "Kismet/KismetMathLibrary.h"
#include "Cell.h"

void AGame_AIController::SetTargetLocation()
{
	if (TargetActor) TargetLocation = TargetActor->GetActorLocation();
}

float AGame_AIController::LookAt(FVector target)
{
	FVector2D currentDirection(Character->GetActorForwardVector());
	FVector2D targetDirection((target - Character->GetActorLocation()).GetSafeNormal());
	float angle = UKismetMathLibrary::Acos(FVector2D::DotProduct(currentDirection, targetDirection));
	if (FVector2D::CrossProduct(currentDirection, targetDirection) < 0) angle = -angle;

	float distance = FVector2D::Distance(FVector2D(Character->GetActorLocation()), FVector2D(target));
	//Try to look at the target
	if (abs(angle) > AlignmentTolerance)
	{
		//RotationRate = angle / abs(angle);
		float sign = angle / abs(angle);
		if (abs(angle) > SlowDownAngle) RotationRate = sign * (0.3f + 0.7f *(distance/SafeTurningDistance));
		else RotationRate = sign * (0.3f + (0.7f *((abs(angle) - AlignmentTolerance) / (SlowDownAngle - AlignmentTolerance)) * (distance / SafeTurningDistance)));
	}
	else
	{
		RotationRate = 0.0f;
	}

	return angle;
}

bool AGame_AIController::FindPath(FVector destination)
{
	for (auto& cell : Path.CellsInPath)
	{
		GridManager->SetCellColor(cell->Index, FColor::Blue);
	}

	return GridManager->FindPathByLocation(Path, Character->GetActorLocation(), destination);
}

void AGame_AIController::FollowPathToTarget()
{
	if(Path.CellsInPath.Num() == 0) return;

	float distance = FVector2D::Distance(FVector2D(Path.CellsInPath[0]->Location), FVector2D(Character->GetActorLocation()));
	while (distance < CellReachDistance)
	{
		GridManager->SetCellColor(Path.CellsInPath[0]->Index, FColor::Green);
		Path.CellsInPath.RemoveAt(0);
		if (Path.CellsInPath.Num() == 0) return;
		distance = FVector2D::Distance(FVector2D(Path.CellsInPath[0]->Location), FVector2D(Character->GetActorLocation()));
	}

	float angle = LookAt(Path.CellsInPath[0]->Location);
	if ( abs(angle) < PathfindMaxMoveAngle)
	{
		//float input = (1 - (abs(angle) / MaxMoveAngle));// *(0.5 + 0.5 * (1 - distance / CellReachDistance));
		//Character->MoveForward((1 - angle/MaxMoveAngle) * (0.5 + 0.5 * (distance/CellReachDistance)));
		Character->MoveForward(1.0f);
	}
}

void AGame_AIController::Act()
{
	switch (CurrentState)
	{
	case CHASING:
		GoToLocation();
		break;
	case FLEEING:
		MoveAwayFromLocation();
		break;
	case WANDER:
		CurrentState = Wander();
		break;
	}
}

void AGame_AIController::GoToLocation()
{
	SetTargetLocation();

	float angle = LookAt(TargetLocation);
	float distance = FVector::Distance(Character->GetActorLocation(), TargetLocation);

	//If the charater is looking roughtly in the direction of the target and is not too close...
	if (distance > StopDistance && abs(angle) < MaxMoveAngle)
	{
		//If the character is getting close, slow down
		if (distance < SlowDownDistance)
		{
			Character->MoveForward((distance - StopDistance) / (SlowDownDistance - StopDistance));
		}
		else Character->MoveForward(1.0f);//Otherwise move at max speed
	}
}

void AGame_AIController::MoveAwayFromLocation()
{
	SetTargetLocation();

	FVector fleeLocation;
	FVector charLocation = Character->GetActorLocation();
	fleeLocation = charLocation + ((charLocation - TargetLocation).GetSafeNormal() * SafeFlightDistance);

	float angle = LookAt(fleeLocation);
	float distance = FVector::Distance(Character->GetActorLocation(), TargetLocation);

	//If the charater is looking roughtly in the direction of the target and is not too close...
	if (distance < SafeFlightDistance && abs(angle) < MaxMoveAngle)
	{
		//If the character is getting close, slow down
		if (distance > SlowDownDistance)
		{
			Character->MoveForward((SafeFlightDistance - distance) / (SafeFlightDistance - SlowDownDistance));
		}
		else Character->MoveForward(1.0f);//Otherwise move at max speed
		//Character->MoveForward(1.0f);
	}
}

TEnumAsByte<AIState> AGame_AIController::Wander()
{
	if (SecondsWandering <= 0 || Path.CellsInPath.Num() <= 0)
	{
		FindPath(TargetLocation);
		TargetLocation = GridManager->GetRandomCell()->Location;
		SecondsWandering = 5.0f;
	}
	else SecondsWandering -= GetWorld()->GetDeltaSeconds();

	FollowPathToTarget();
	GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Orange, FString::Printf(TEXT("State number: %f"), SecondsWandering));
	return WANDER;
}

void AGame_AIController::BeginPlay()
{
	Super::BeginPlay();

	Character = Cast<AAI_GameCharacter>(GetPawn());
	if (!Character) GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, TEXT("Failed to find suitable pawn!"));

	FAttachmentTransformRules transformRules(EAttachmentRule::SnapToTarget, false);
	AttachToActor(Character, transformRules);

	TrackingSphere->AttachTo(GetRootComponent(), NAME_None, EAttachLocation::SnapToTarget);
}

void AGame_AIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	RotationRate = 0.0f;

	GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Orange, FString::Printf(TEXT("State number: %d"), CurrentState.GetValue()));
	GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Orange, TargetLocation.ToString());


	if (!TargetActor) TargetActor = GetWorld()->GetFirstPlayerController()->GetPawn();
	Act();
	Character->SetActorRotation(Character->GetActorRotation() + FRotator(0.0f, FMath::Clamp(RotationRate, -1.0f, 1.0f) * Character->BaseTurnRate * DeltaTime, 0.0f));
}

AGame_AIController::AGame_AIController() : AAIController()
{
	TrackingSphere = CreateDefaultSubobject<USphereComponent>(TEXT("Tracking Sphere"));
	TrackingSphere->SetVisibility(true);
	TrackingSphere->SetHiddenInGame(false);
}

TEnumAsByte<AIState> AGame_AIController::ToggleState()
{
	CurrentState = TEnumAsByte<AIState>(CurrentState + 1);
	if(CurrentState == LAST) CurrentState = TEnumAsByte<AIState>(0);

	return CurrentState;
}
