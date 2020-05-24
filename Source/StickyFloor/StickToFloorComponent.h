// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "DrawDebugHelpers.h"
#include "StickToFloorComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class STICKYFLOOR_API UStickToFloorComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UStickToFloorComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bSnapToNewSurface;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SurfaceNormalTolerance = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FloorCastLength = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DistanceFromFloor = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<ECollisionChannel> FloorCastChannel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bDebugGizmos;

protected:
	virtual void BeginPlay() override;
	AActor* Owner = nullptr;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void Stick() const;

private:
	void DrawGizmos(FVector HitPoint, FVector FirstHitNormal, float DotDifference) const;
};
