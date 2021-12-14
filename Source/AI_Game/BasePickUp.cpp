// Fill out your copyright notice in the Description page of Project Settings.


#include "BasePickUp.h"

// Sets default values
ABasePickUp::ABasePickUp()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	TriggerSphere = CreateDefaultSubobject<USphereComponent>(TEXT("TriggerSphere"));
	TriggerSphere->SetVisibility(true);
	TriggerSphere->SetCollisionProfileName("OverlapAll");

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetVisibility(true);
	Mesh->SetHiddenInGame(false);
	Mesh->SetCollisionProfileName("NoCollision");
	RootComponent = Mesh;
}

// Called when the game starts or when spawned
void ABasePickUp::BeginPlay()
{
	Super::BeginPlay();
	
}

bool ABasePickUp::OnPickedUp(AAI_GameCharacter* character)
{
	if (ValidPickUp)
	{
		if (Mesh) Mesh->SetVisibility(false);
		ValidPickUp = false;
	}
	return !ValidPickUp;
}

// Called every frame
void ABasePickUp::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!ValidPickUp)
	{
		if (SecondsSleeping >= SleepTime)
		{
			ValidPickUp = true;
			SecondsSleeping = 0.0f;
			if (Mesh) Mesh->SetVisibility(true);
		}
		else
		{
			SecondsSleeping += GetWorld()->GetDeltaSeconds();
		}
	}
}

