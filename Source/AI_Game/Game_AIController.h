// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AI_GameCharacter.h"
#include "GridManager.h"
#include "Components/SphereComponent.h"
#include "BasePickUp.h"
#include "Game_AIController.generated.h"

/**
 * 
 */
UENUM()
enum AIState
{
	CHASING			UMETA(DisplayName = "Chasing"),
	FLEEING			UMETA(DisplayName = "Fleeing"),
	WANDERING		UMETA(DisplayName = "Wandering"),
	ENGAGE_VIOLENCE	UMETA(DisplayName = "Engage Violence"),
	SEEKING			UMETA(DisplayName = "Seeking"),
	LAST			UMETA(DisplayName = "LastState")
};

UCLASS()
class AI_GAME_API AGame_AIController : public AAIController
{
	GENERATED_BODY()

protected:
	AAI_GameCharacter* Character;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Arrive/Flee");
	ABasePickUp* TargetPickUp;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Arrive/Flee");
	AAI_GameCharacter* TargetEnemy;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Arrive/Flee");
	FVector TargetLocation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Arrive/Flee")
	float AlignmentTolerance = 0.087f;//In Radians (1 degree ~ 0.0174533 radians)

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Arrive/Flee")
	float SafeTurningDistance = 300.0f;//In Radians (1 degree ~ 0.0174533 radians)

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Arrive/Flee")
	float SlowDownAngle = 0.1740f;//In Radians (1 degree ~ 0.0174533 radians)

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Arrive/Flee")
	float SlowDownDistance = 500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Arrive/Flee")
	float StopDistance = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Arrive/Flee")
	float MaxMoveAngle = 0.5;//The maximum absolute angle between the character and the target in which it's still efficient to move forward

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Arrive/Flee")
	float SafeFlightDistance = 1000.0f;//How far from the flee target is considered "safe" to stop fleeing.

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Fire")
		float MissAngleRange = 0.349066;//In Radians

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Fire")
		float Accuracy = 0.5f;//Between 0 and 1

	UFUNCTION(BlueprintCallable)
		float GetMissAngle();

	float RotationRate = 0.0f;

	UFUNCTION(BlueprintCallable)
	void GoToLocation();
	
	UFUNCTION(BlueprintCallable)
	void MoveAwayFromLocation();

	UFUNCTION(BlueprintCallable)
	float LookAt(FVector target);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		FPath Path;

	UFUNCTION(BlueprintCallable)
		bool FindPath(FVector destination);

	UFUNCTION(BlueprintCallable)
		void FollowPathToTarget();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
		float CellReachDistance = 80.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
		float PathfindMaxMoveAngle = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
		float SecondsWandering = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
		float SecondsFleeing = 2.0f;

	void Act();

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Decision")
		TSet<AAI_GameCharacter*> NearbyEnemies;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Decision")
		float NearbyEnemyRange = 10000;

	UFUNCTION(BlueprintCallable)
		AActor* FindClosestActor(TArray<AActor*>& actors);

	UFUNCTION(BlueprintCallable)
		AAI_GameCharacter* FindClosestEnemy(TSet<AAI_GameCharacter*>& enemies);

	//Behaviour methods

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Decision")
		TEnumAsByte<AIState> CurrentState = WANDERING;

	UFUNCTION(BlueprintCallable)
		TEnumAsByte<AIState> Wander();
	
	UFUNCTION(BlueprintCallable)
		TEnumAsByte<AIState> Chase();

	UFUNCTION(BlueprintCallable)
		TEnumAsByte<AIState> Flee();

	UFUNCTION(BlueprintCallable)
		TEnumAsByte<AIState> Fire();

	UFUNCTION(BlueprintCallable)
		TEnumAsByte<AIState> Seek();

	UFUNCTION(BlueprintCallable)
		TEnumAsByte<AIState> HasEnemyInSight();

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		AGridManager* GridManager;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
	void AddNearbyEnemy(AAI_GameCharacter* enemy);

	UFUNCTION(BlueprintCallable)
	void RemoveNearbyEnemy(AAI_GameCharacter* enemy);

	void PrintData();

	//For Testing

	UFUNCTION(BlueprintCallable)
		TEnumAsByte<AIState> ToggleState();
};
