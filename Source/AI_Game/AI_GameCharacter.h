// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Perception/AIPerceptionComponent.h"
#include "AI_GameCharacter.generated.h"

UCLASS(config=Game)
class AAI_GameCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class USceneComponent* CharacterRoot;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	class USceneComponent* GunStartPosition;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
		class UCapsuleComponent* WeaponHitbox;
public:
	AAI_GameCharacter();

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseLookUpRate;

	/** Resets HMD orientation in VR. */
	void OnResetVR();

	/** Called for forwards/backward input */
	void MoveForward(float Value);

	/** Called for side to side input */
	void MoveRight(float Value);

	/**
	 * Called via input to turn at a given rate.
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void TurnAtRate(float Rate);

	/**
	 * Called via input to turn look up/down at a given rate.
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAtRate(float Rate);

	UFUNCTION(BlueprintCallable)
		void AddHP(int hp);

	UFUNCTION(BlueprintCallable)
		void DealDamage(int damage);

	UFUNCTION(BlueprintCallable)
		void AddAmmo(int ammo);

	UFUNCTION(BlueprintCallable)
		bool Fire();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stats")
		int32 MaxHP = 10;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stats")
		int32 CurrentHP = 10;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stats")
		int32 MaxAmmo = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
		int32 CurrentAmmo = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stats")
		int32 WeaponDamage = 3;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stats")
		float WeaponRange = 1200.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stats")
		float FireRange = 30.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stats")
		float FireDealy = 0.7f;

		float LastFireTime = 0.0f;

	/** Handler for when a touch input begins. */
	void TouchStarted(ETouchIndex::Type FingerIndex, FVector Location);

	/** Handler for when a touch input stops. */
	void TouchStopped(ETouchIndex::Type FingerIndex, FVector Location);

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	// End of APawn interface

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Decision")
		bool LowHealth = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Decision")
		int32 LowHealthThreshold = 3;

	inline int32 GetMaxHp() { return MaxHP; }
	inline int32 GetCurrentHp() { return CurrentHP; }
	inline int32 GetMaxAmmo() { return MaxAmmo; }
	inline int32 GetCurrentAmmo() { return CurrentAmmo; }
	inline float GetWeaponRange() { return WeaponRange; }
	inline UCapsuleComponent* GetWeaponHitbox() { return WeaponHitbox; }

};

