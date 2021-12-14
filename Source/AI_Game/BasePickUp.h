// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SphereComponent.h"
#include "AI_GameCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "BasePickUp.generated.h"

UCLASS()
class AI_GAME_API ABasePickUp : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABasePickUp();

protected:

	UPROPERTY(EditDefaultsOnly)
	USphereComponent* TriggerSphere;

	UPROPERTY(EditDefaultsOnly, BluePrintReadWrite)
	UStaticMeshComponent* Mesh;

	UPROPERTY(EditDefaultsOnly)
		float SleepTime = 10.0f;

	float SecondsSleeping = 0.0f;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable)
	virtual bool OnPickedUp(AAI_GameCharacter* character);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	bool ValidPickUp = true;

};
