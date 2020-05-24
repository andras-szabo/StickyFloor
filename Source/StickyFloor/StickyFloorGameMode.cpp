// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "StickyFloorGameMode.h"
#include "StickyFloorPawn.h"

AStickyFloorGameMode::AStickyFloorGameMode()
{
	// set default pawn class to our character class
	DefaultPawnClass = AStickyFloorPawn::StaticClass();
}

