// Fill out your copyright notice in the Description page of Project Settings.

#include "StickToFloorComponent.h"


UStickToFloorComponent::UStickToFloorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void UStickToFloorComponent::BeginPlay()
{
	Super::BeginPlay();
	Owner = GetOwner();
}

void UStickToFloorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	Stick();
}

void UStickToFloorComponent::Stick() const
{
	if (Owner == nullptr)
	{
		return;
	}

	const FVector ActorUp = Owner->GetActorUpVector();
	const FVector ActorForward = Owner->GetActorForwardVector();
	const FVector ActorRight = Owner->GetActorRightVector();

	FHitResult DirectHitResult;
	const FVector FloorCastStart = Owner->GetActorLocation();
	const FVector FloorCastEnd = FloorCastStart - ActorUp * FloorCastLength;

	const auto World = GetWorld();
	bool bShouldAdjustRotation = false;

	if (World != nullptr && World->LineTraceSingleByChannel(DirectHitResult, FloorCastStart, FloorCastEnd, FloorCastChannel))
	{
		// Based on the line trace, we get the current surface's orientation.
		// Because of the edge gaps, we can only accept it if casting a ray
		// from the new orientation using the current "down" would get us
		// a ray normal approximately the same.

		const FVector FirstHitNormal = DirectHitResult.Normal;
		FVector SecondHitNormal = FirstHitNormal;
		float DotDifference = 0.0f;

		FVector NewUp = ActorUp;

		if (bSnapToNewSurface)
		{
			NewUp = FirstHitNormal;
			bShouldAdjustRotation = true;
		}
		else
		{
			FHitResult SecondHitResult;

			if (World->LineTraceSingleByChannel(SecondHitResult, FloorCastStart, FloorCastStart - FirstHitNormal * FloorCastLength, FloorCastChannel))
			{
				SecondHitNormal = SecondHitResult.Normal;
				DotDifference = 1.0f - FVector::DotProduct(FirstHitNormal, SecondHitNormal);

				if (DotDifference <= SurfaceNormalTolerance)
				{
					NewUp = SecondHitNormal;
					bShouldAdjustRotation = true;
				}
			}
		}

		FRotator NewRotation = Owner->GetActorRotation();

		if (bShouldAdjustRotation)
		{
			const FVector NewForward = FVector::CrossProduct(ActorRight, NewUp);
			const FVector NewRight = FVector::CrossProduct(-NewForward, NewUp);
			const FMatrix NewMatrix(NewForward, NewRight, NewUp, FVector::ZeroVector);
			NewRotation = NewMatrix.Rotator();
		}

		const FVector UpVectorRelativeToSurface = bSnapToNewSurface ? NewUp : (FloorCastStart - DirectHitResult.ImpactPoint).GetSafeNormal();
		const FVector NewLocation = DirectHitResult.ImpactPoint + UpVectorRelativeToSurface * DistanceFromFloor;

		Owner->SetActorLocationAndRotation(NewLocation, NewRotation);

		if (bDebugGizmos)
		{
			DrawGizmos(DirectHitResult.ImpactPoint, FirstHitNormal, DotDifference);
		}
	}
}

void UStickToFloorComponent::DrawGizmos(FVector HitPoint, FVector FirstNormal, float DotDifference) const
{
	if (Owner == nullptr)
	{
		return;
	}

	const FVector ActorUp = Owner->GetActorUpVector();
	const FVector ActorForward = Owner->GetActorForwardVector();
	const FVector ActorRight = Owner->GetActorRightVector();
	const FVector FloorCastStart = Owner->GetActorLocation();
	const UWorld* world = GetWorld();

	DrawDebugDirectionalArrow(world, FloorCastStart, FloorCastStart + ActorForward * 250.0f, 40.0f, FColor::Blue);
	DrawDebugDirectionalArrow(world, FloorCastStart, FloorCastStart + ActorRight * 250.0f, 40.0f, FColor::Red);
	DrawDebugDirectionalArrow(world, FloorCastStart, FloorCastStart + ActorUp * 250.0f, 40.0f, FColor::Green);

	DrawDebugDirectionalArrow(world, HitPoint, HitPoint + ActorUp * 250.0f, 40.0f, FColor::White);
	DrawDebugDirectionalArrow(world, HitPoint, HitPoint + FirstNormal * 250.0f, 40.0f, FColor::Cyan);

	GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, FString::Printf(TEXT("Dot difference: %f"), DotDifference));
}
