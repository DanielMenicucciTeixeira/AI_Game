// Copyright Epic Games, Inc. All Rights Reserved.

#include "AI_GameGameMode.h"
#include "AI_GameCharacter.h"
#include "UObject/ConstructorHelpers.h"

AAI_GameGameMode::AAI_GameGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPersonCPP/Blueprints/ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
