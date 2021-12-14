// Fill out your copyright notice in the Description page of Project Settings.


#include "Game_AIController.h"
#include "Kismet/KismetMathLibrary.h"
#include "Cell.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"

#define VERY_BIG 999999999.9f
#define SMALL 100.0f

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
		CurrentState = Chase();
		break;
	case FLEEING:
		CurrentState = Flee();
		break;
	case WANDERING:
		CurrentState = Wander();
		break;
	case ENGAGE_VIOLENCE:
		CurrentState = Fire();
		break;
	case SEEKING:
		CurrentState = Seek();
		break;
	}
}

float AGame_AIController::GetMissAngle()
{
	static FRandomStream random;
	return FMath::Max(0.0f, random.FRandRange(0.0f, MissAngleRange) - MissAngleRange * Accuracy);
}

void AGame_AIController::GoToLocation()
{
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

AActor* AGame_AIController::FindClosestActor(TArray<AActor*>& actors)
{
	AActor* closest = actors[0];
	float closestDistance = FVector::Distance(Character->GetActorLocation(), closest->GetActorLocation());
	float tempDistance;
	for (const auto& actor : actors)
	{
		tempDistance = FVector::Distance(Character->GetActorLocation(), actor->GetActorLocation());
		if (tempDistance < closestDistance)
		{
			closest = actor;
			closestDistance = tempDistance;
		}
	}

	return closest;
}

AAI_GameCharacter* AGame_AIController::FindClosestEnemy(TSet<AAI_GameCharacter*>& enemies)
{
	if (NearbyEnemies.Num() <= 0) return nullptr;
	AAI_GameCharacter* closest = nullptr;
	for (const auto& first : enemies) { closest = first; break; }
	float closestDistance = FVector::Distance(Character->GetActorLocation(), closest->GetActorLocation());
	float tempDistance;
	for (const auto& enemy : enemies)
	{
		tempDistance = FVector::Distance(Character->GetActorLocation(), enemy->GetActorLocation());
		if (tempDistance < closestDistance)
		{
			closest = enemy;
			closestDistance = tempDistance;
		}
	}

	return closest;
}

TEnumAsByte<AIState> AGame_AIController::Wander()
{
	static FRandomStream random;

	if (NearbyEnemies.Num() > 0)
	{
		return HasEnemyInSight();
	}
	else if (random.RandRange(0, Character->GetMaxHp()) > Character->GetCurrentHp())
	{
		TArray<AActor*> pickUps;
		UGameplayStatics::GetAllActorsWithTag(GetWorld(), "HealthPickUp", pickUps);
		if (pickUps.Num() > 0)
		{
			TargetPickUp = Cast<ABasePickUp>(FindClosestActor(pickUps));
			FindPath(TargetPickUp->GetActorLocation());
			return SEEKING;
		}
	}
	else if(Character->GetCurrentAmmo() <= 0)
	{
		TArray<AActor*> pickUps;
		UGameplayStatics::GetAllActorsWithTag(GetWorld(), "AmmoPickUp", pickUps);
		if (pickUps.Num() > 0)
		{
			TargetPickUp = Cast<ABasePickUp>(FindClosestActor(pickUps));
			FindPath(TargetPickUp->GetActorLocation());
			return SEEKING;
		}
	}

	if (SecondsWandering <= 0 || Path.CellsInPath.Num() <= 0)
	{
		FindPath(TargetLocation);
		TargetLocation = GridManager->GetRandomCell()->Location;
		SecondsWandering = 5.0f;
	}
	else SecondsWandering -= GetWorld()->GetDeltaSeconds();

	FollowPathToTarget();
	return WANDERING;
}

TEnumAsByte<AIState> AGame_AIController::Chase()
{
	if (NearbyEnemies.Num() <= 0 || Character->GetCurrentAmmo() <= 0) return WANDERING;
	if (!TargetEnemy) TargetEnemy = FindClosestEnemy(NearbyEnemies);

	if (FVector::Distance(Character->GetActorLocation(), TargetEnemy->GetActorLocation()) > Character->GetWeaponRange())
	{
		FHitResult outHitResult;
		FCollisionObjectQueryParams objectQuerry;
		FCollisionQueryParams collisionParams;
		if (GetWorld()->LineTraceSingleByObjectType(outHitResult, Character->GetActorLocation() + Character->GetActorForwardVector() * VERY_BIG + FVector(0.0f, 0.0f, 16.0f), (Character->GetActorForwardVector() * VERY_BIG) + Character->GetActorLocation() + Character->GetActorForwardVector() * VERY_BIG + FVector(0.0f, 0.0f, 16.0f), objectQuerry, collisionParams))
		{
			TargetLocation = TargetEnemy->GetActorLocation();
			GoToLocation();
		}
		else
		{
			FindPath(TargetEnemy->GetActorLocation());
			FollowPathToTarget();
		}
		return CHASING;
	}
	else return ENGAGE_VIOLENCE;
}

TEnumAsByte<AIState> AGame_AIController::Flee()
{
	static float flightTime = 0.0f;

	if (NearbyEnemies.Num() <= 0)
	{
		if (flightTime >= SecondsFleeing)
		{
			flightTime = 0.0f;
			return WANDERING;
		}
		else
		{
			flightTime += GetWorld()->GetDeltaSeconds();
			return FLEEING;
		}
	}

	if (!TargetEnemy) TargetEnemy = FindClosestEnemy(NearbyEnemies);
	
	FVector averageEnemyLocation = FVector(0.0f);
	for (const auto& enemy : NearbyEnemies)
	{
		averageEnemyLocation += enemy->GetActorLocation();
	}
	averageEnemyLocation /= NearbyEnemies.Num();
	TargetLocation = (Character->GetActorLocation() - averageEnemyLocation).GetSafeNormal() * SafeFlightDistance;

	FHitResult outHitResult;
	FCollisionObjectQueryParams objectQuerry;
	FCollisionQueryParams collisionParams;
	if (GetWorld()->LineTraceSingleByObjectType(outHitResult, Character->GetActorLocation() + Character->GetActorForwardVector() * SMALL + FVector(0.0f, 0.0f, 16.0f), (Character->GetActorForwardVector() * SMALL) + Character->GetActorLocation() + Character->GetActorForwardVector() * SMALL + FVector(0.0f, 0.0f, 16.0f), objectQuerry, collisionParams))
	{
		MoveAwayFromLocation();
	}
	else
	{
		FindPath(TargetLocation);
		FollowPathToTarget();
	}
	return FLEEING;
}

TEnumAsByte<AIState> AGame_AIController::Fire()
{
	if (NearbyEnemies.Num() < 0 || Character->GetCurrentAmmo() <= 0) return WANDERING;

	if (!TargetEnemy) TargetEnemy = FindClosestEnemy(NearbyEnemies);
	if (!TargetEnemy) return WANDERING;

	if (FVector::Distance(Character->GetActorLocation(), TargetEnemy->GetActorLocation()) > Character->GetWeaponRange()) return CHASING;

	float angle = LookAt(TargetEnemy->GetActorLocation());
	if (tan(abs(angle)) * FVector::Distance(Character->GetActorLocation(), TargetEnemy->GetActorLocation()) <= Character->GetWeaponHitbox()->GetScaledCapsuleRadius() + GetMissAngle())
	{
		Character->Fire();
	}
	return ENGAGE_VIOLENCE;
}

TEnumAsByte<AIState> AGame_AIController::Seek()
{
	if(NearbyEnemies.Num() > 0) return HasEnemyInSight();
	
	if (!TargetPickUp || !TargetPickUp->ValidPickUp)
	{
		return WANDERING;
	}

	if (true)//Path.CellsInPath.Num() <= 0)
	{
		TargetLocation = TargetPickUp->GetActorLocation();
		GoToLocation();
	}
	else FollowPathToTarget();

	return SEEKING;
}

TEnumAsByte<AIState> AGame_AIController::HasEnemyInSight()
{
	if (Character->GetCurrentAmmo() <= 0) return FLEEING;
	for (const auto& enemy : NearbyEnemies)
	{
		if (Character->LowHealth && !enemy->LowHealth) return FLEEING;
		else if (!TargetEnemy) TargetEnemy = FindClosestEnemy(NearbyEnemies);
	}
	if (TargetEnemy && FVector::Distance(TargetEnemy->GetActorLocation(), Character->GetActorLocation()) <= Character->GetWeaponRange()) return ENGAGE_VIOLENCE;
	else return CHASING;
}

void AGame_AIController::BeginPlay()
{
	Super::BeginPlay();

	Character = Cast<AAI_GameCharacter>(GetPawn());
	if (!Character) GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, TEXT("Failed to find suitable pawn!"));

	FAttachmentTransformRules transformRules(EAttachmentRule::SnapToTarget, false);
	AttachToActor(Character, transformRules);
}

void AGame_AIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	RotationRate = 0.0f;
	for (const auto& enemy : NearbyEnemies)
	{
		if (FVector::Distance(enemy->GetActorLocation(), Character->GetActorLocation()) > 1000.0) RemoveNearbyEnemy(enemy);
	}
	if (!NearbyEnemies.Contains(TargetEnemy)) TargetEnemy = nullptr;
	Act();
	Character->SetActorRotation(Character->GetActorRotation() + FRotator(0.0f, FMath::Clamp(RotationRate, -1.0f, 1.0f) * Character->BaseTurnRate * DeltaTime, 0.0f));
	PrintData();
}

void AGame_AIController::AddNearbyEnemy(AAI_GameCharacter* enemy)
{
	if (!enemy) return;
	if(enemy != Character) NearbyEnemies.Add(enemy);
}

void AGame_AIController::RemoveNearbyEnemy(AAI_GameCharacter* enemy)
{
	if (!enemy) return;
	if (enemy != Character) NearbyEnemies.Remove(enemy);
}

void AGame_AIController::PrintData()
{
	switch (CurrentState)
	{
	case CHASING:
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor(255, 192, 203), TEXT("CHASING"));
		break;
	case FLEEING:
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor(255, 192, 203), TEXT("FlEEING"));
		break;
	case WANDERING:
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor(255, 192, 203), TEXT("WANDERING"));
		break;
	case ENGAGE_VIOLENCE:
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor(255, 192, 203), TEXT("ENGAGE_VIOLENCE"));
		break;
	case SEEKING:
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor(255, 192, 203), TEXT("SEEKING"));
		break;
	}

	if(TargetEnemy) GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Blue, TargetEnemy->GetName());
	else GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Blue, TEXT("None"));

	if (TargetPickUp) GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Cyan, TargetPickUp->GetName());
	else GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Blue, TEXT("None"));
}

TEnumAsByte<AIState> AGame_AIController::ToggleState()
{
	CurrentState = TEnumAsByte<AIState>(CurrentState + 1);
	if(CurrentState == LAST) CurrentState = TEnumAsByte<AIState>(0);

	return CurrentState;
}
