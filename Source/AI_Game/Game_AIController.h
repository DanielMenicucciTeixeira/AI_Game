// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AI_GameCharacter.h"
#include "GridManager.h"
#include "Game_AIController.generated.h"

/**
 * 
 */
UENUM()
enum AIState
{
	CHASING			UMETA(DisplayName = "Chasing"),
	FLEEING			UMETA(DisplayName = "Fleeing"),
	FOLLOWING_PATH	UMETA(DisplayName = "Following Path"),
	LAST			UMETA(DisplayName = "LastState")
};

UCLASS()
class AI_GAME_API AGame_AIController : public AAIController
{
	GENERATED_BODY()

protected:
	AAI_GameCharacter* Character;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Arrive/Flee");
	AActor* TargetActor;

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
	float StopDistance = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Arrive/Flee")
	float MaxMoveAngle = 0.5;//The maximum absolute angle between the character and the target in which it's still efficient to move forward

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Arrive/Flee")
	float SafeFlightDistance = 1000.0f;//How far from the flee target is considered "safe" to stop fleeing.

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Decision")
	TEnumAsByte<AIState> State = FLEEING;

	UFUNCTION(BlueprintCallable)
	void SetTargetLocation();

	float RotationRate = 0.0f;

	UFUNCTION(BlueprintCallable)
	void Chase();
	
	UFUNCTION(BlueprintCallable)
	void Flee();

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

	void Act();

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		AGridManager* GridManager;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	//For Testing

	UFUNCTION(BlueprintCallable)
		TEnumAsByte<AIState> ToggleState();
};