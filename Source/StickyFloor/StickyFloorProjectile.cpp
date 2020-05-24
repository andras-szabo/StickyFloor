// Copyright 1998-2019 Epic Games, Inc. All Rights Reserve

#include "StickyFloorProjectile.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Engine/StaticMesh.h"

AStickyFloorProjectile::AStickyFloorProjectile() 
{
	// Static reference to the mesh to use for the projectile
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ProjectileMeshAsset(TEXT("/Game/TwinStick/Meshes/TwinStickProjectile.TwinStickProjectile"));

	// Create mesh component for the projectile sphere
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh0"));
	ProjectileMesh->SetStaticMesh(ProjectileMeshAsset.Object);
	ProjectileMesh->SetupAttachment(RootComponent);
	ProjectileMesh->BodyInstance.SetCollisionProfileName(TEXT("Projectile"));
	ProjectileMesh->OnComponentHit.AddDynamic(this, &AStickyFloorProjectile::OnHit);		// set up a notification for when this component hits something
	RootComponent = ProjectileMesh;

	// Stick to the floor
	StickToFloor = CreateDefaultSubobject<UStickToFloorComponent>(TEXT("StickToFloor"));
	StickToFloor->bSnapToNewSurface = false;
	StickToFloor->Activate();

	// Die after 3 seconds by default
	PrimaryActorTick.bCanEverTick = true;
	InitialLifeSpan = 3.0f;
}

void AStickyFloorProjectile::SetupFloorCast(const UStickToFloorComponent& BaseStickToFloorComponent)
{
	if (StickToFloor != nullptr)
	{
		StickToFloor->FloorCastChannel = BaseStickToFloorComponent.FloorCastChannel;
		StickToFloor->FloorCastLength = BaseStickToFloorComponent.FloorCastLength;
		StickToFloor->DistanceFromFloor = BaseStickToFloorComponent.DistanceFromFloor;
	}
}

void AStickyFloorProjectile::Tick(float DeltaSeconds)
{
	const FVector MoveDirection = GetActorForwardVector();
	const FVector Movement = MoveDirection * DeltaSeconds * 1500.0f;
	FHitResult hit(1.0f);
	RootComponent->MoveComponent(Movement, GetActorRotation(), true, &hit);
}

void AStickyFloorProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// Only add impulse and destroy projectile if we hit a physics
	if ((OtherActor != NULL) && (OtherActor != this) && (OtherComp != NULL) && OtherComp->IsSimulatingPhysics())
	{
		OtherComp->AddImpulseAtLocation(GetVelocity() * 20.0f, GetActorLocation());
	}

	//GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, FString::Printf(TEXT("Other: %s"), *OtherActor->GetName()));
	Destroy();
}