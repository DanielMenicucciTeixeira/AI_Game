// Copyright Epic Games, Inc. All Rights Reserved.

#include "AI_GameCharacter.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "DrawDebugHelpers.h"

#define COLLISION_WEAPON ECC_GameTraceChannel1

//////////////////////////////////////////////////////////////////////////
// AAI_GameCharacter

AAI_GameCharacter::AAI_GameCharacter()
{
	RootComponent = CharacterRoot;
	GunStartPosition = CreateDefaultSubobject<USceneComponent>(TEXT("Gun Position"));
	WeaponHitbox = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Weapon Hitbox"));

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
	GetCapsuleComponent()->SetRelativeLocation(FVector(0.0f, 0.0f, 96.0f));

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetCapsuleComponent());
	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)
}

//////////////////////////////////////////////////////////////////////////
// Input

void AAI_GameCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAxis("MoveForward", this, &AAI_GameCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AAI_GameCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AAI_GameCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AAI_GameCharacter::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &AAI_GameCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &AAI_GameCharacter::TouchStopped);

	// VR headset functionality
	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &AAI_GameCharacter::OnResetVR);
}


void AAI_GameCharacter::OnResetVR()
{
	// If AI_Game is added to a project via 'Add Feature' in the Unreal Editor the dependency on HeadMountedDisplay in AI_Game.Build.cs is not automatically propagated
	// and a linker error will result.
	// You will need to either:
	//		Add "HeadMountedDisplay" to [YourProject].Build.cs PublicDependencyModuleNames in order to build successfully (appropriate if supporting VR).
	// or:
	//		Comment or delete the call to ResetOrientationAndPosition below (appropriate if not supporting VR)
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void AAI_GameCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
		Jump();
}

void AAI_GameCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
		StopJumping();
}

void AAI_GameCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AAI_GameCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AAI_GameCharacter::AddHP(int hp)
{
	CurrentHP += hp;
	if (CurrentHP >= LowHealthThreshold) LowHealth = false;
	if (CurrentHP > MaxHP) CurrentHP = MaxHP;
}

void AAI_GameCharacter::DealDamage(int damage)
{
	CurrentHP -= damage;
	if (CurrentHP <= LowHealthThreshold) LowHealth = true;
	if (CurrentHP <= 0) Destroy();
}

void AAI_GameCharacter::AddAmmo(int ammo)
{
	CurrentAmmo += ammo;
	if (CurrentAmmo > MaxAmmo) CurrentAmmo = MaxAmmo;
}

bool AAI_GameCharacter::Fire()
{
	if (GetWorld()->GetTimeSeconds() - LastFireTime < FireDealy) return false;
	if (CurrentAmmo <= 0) return false;

	CurrentAmmo--;
	LastFireTime = GetWorld()->GetTimeSeconds();
	FHitResult outHitResult;
	FCollisionQueryParams params;
	params.AddIgnoredActor(this);

	DrawDebugLine
	(
		GetWorld(),
		GetActorLocation() + GetActorForwardVector() * FireRange + FVector(0.0f, 0.0f, 16.0f),
		(GetActorForwardVector() * WeaponRange) + GetActorLocation() + GetActorForwardVector() * FireRange + FVector(0.0f, 0.0f, 16.0f),
		FColor(255, 0, 0),
		false, 0.3f, 10,
		12.333 
	);
	if (GetWorld()->LineTraceSingleByChannel(outHitResult, GetActorLocation() + GetActorForwardVector() * FireRange + FVector(0.0f, 0.0f, 16.0f), (GetActorForwardVector() * WeaponRange) + GetActorLocation() + GetActorForwardVector() * FireRange + FVector(0.0f, 0.0f, 16.0f), COLLISION_WEAPON, params))
	{
		if (Cast<AAI_GameCharacter>(outHitResult.Actor)) Cast<AAI_GameCharacter>(outHitResult.Actor)->DealDamage(WeaponDamage);
		return true;
	}
	else return false;
}

void AAI_GameCharacter::MoveForward(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void AAI_GameCharacter::MoveRight(float Value)
{
	if ( (Controller != nullptr) && (Value != 0.0f) )
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}
