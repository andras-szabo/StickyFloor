// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "StickyFloorPawn.h"
#include "StickyFloorProjectile.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

const FName AStickyFloorPawn::MoveForwardBinding("MoveForward");
const FName AStickyFloorPawn::MoveRightBinding("MoveRight");
const FName AStickyFloorPawn::FireForwardBinding("FireForward");
const FName AStickyFloorPawn::FireRightBinding("FireRight");

AStickyFloorPawn::AStickyFloorPawn()
{	
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ShipMesh(TEXT("/Game/TwinStick/Meshes/TwinStickUFO.TwinStickUFO"));

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// Create the mesh component
	ShipMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShipMesh"));
	ShipMeshComponent->SetupAttachment(RootComponent);
	ShipMeshComponent->SetStaticMesh(ShipMesh.Object);

	// Create stick-to-floor component
	StickToFloor = CreateDefaultSubobject<UStickToFloorComponent>(TEXT("StickToFloor"));
	
	// Cache our sound effect
	static ConstructorHelpers::FObjectFinder<USoundBase> FireAudio(TEXT("/Game/TwinStick/Audio/TwinStickFire.TwinStickFire"));
	FireSound = FireAudio.Object;

	// Create a camera boom...
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->bAbsoluteRotation = false;
	CameraBoom->bUsePawnControlRotation = false;

	CameraBoom->bInheritPitch = true;
	CameraBoom->bInheritYaw = true;
	CameraBoom->bInheritRoll = true;

	CameraBoom->TargetArmLength = 1200.f;
	CameraBoom->RelativeRotation = FRotator(-90.f, 0.f, 0.f);
	CameraBoom->bDoCollisionTest = false; // Don't want to pull camera in when it collides with level

	// Create a camera...
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	CameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	CameraComponent->bUsePawnControlRotation = false;	// Camera does not rotate relative to arm

	// Movement
	MoveSpeed = 1000.0f;
	// Weapon
	FireRate = 0.1f;
	bCanFire = true;
}

void AStickyFloorPawn::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	check(PlayerInputComponent);

	// set up gameplay key bindings
	PlayerInputComponent->BindAxis(MoveForwardBinding);
	PlayerInputComponent->BindAxis(MoveRightBinding);
	PlayerInputComponent->BindAxis(FireForwardBinding);
	PlayerInputComponent->BindAxis(FireRightBinding);
}

void AStickyFloorPawn::BeginPlay()
{
	Super::BeginPlay();
}

void AStickyFloorPawn::Tick(float DeltaSeconds)
{
	// Find input direction
	const float ForwardValue = GetInputAxisValue(MoveForwardBinding);
	const float RightValue = GetInputAxisValue(MoveRightBinding);

	// Calculate _actual_ move direction, based on the camera's orientation
	// (as rotated by 90 degrees)

	const FVector CamForward = CameraComponent->GetUpVector();	
	const FVector CamRight = CameraComponent->GetRightVector();
	const FVector MoveDirection = (CamForward * ForwardValue + CamRight * RightValue).GetClampedToMaxSize(1.0f);
	const FVector Movement = MoveDirection * MoveSpeed * DeltaSeconds;
	const FVector ActorUpVector = GetActorUpVector();

	// If non-zero size, move this actor
	if (Movement.SizeSquared() > 0.0f)
	{
		const FRotator newRotation = GetActorRotation();

		FHitResult Hit(1.f);
		RootComponent->MoveComponent(Movement, newRotation, true, &Hit);
		
		if (Hit.IsValidBlockingHit())
		{
			const FVector Normal2D = Hit.Normal.GetSafeNormal2D();
			const FVector Deflection = FVector::VectorPlaneProject(Movement, Normal2D) * (1.f - Hit.Time);
			RootComponent->MoveComponent(Deflection, newRotation, true);
		}

		// Orient the mesh component to the correct orientation
		const FVector MeshForward = MoveDirection.GetSafeNormal();
		const FVector MeshRightVector = FVector::CrossProduct(-MeshForward, ActorUpVector);
		const FMatrix MeshOrientationMatrix(MeshForward, MeshRightVector, ActorUpVector, FVector::ZeroVector);

		ShipMeshComponent->SetWorldRotation(MeshOrientationMatrix.Rotator(), false, nullptr, ETeleportType::TeleportPhysics);
	}
	
	// Create fire direction vector - again, relative to the camera
	const float FireForwardValue = GetInputAxisValue(FireForwardBinding);
	const float FireRightValue = GetInputAxisValue(FireRightBinding);
	const FVector FireDirection = (CamForward * FireForwardValue + CamRight * FireRightValue).GetClampedToMaxSize(1.0f);

	// Try and fire a shot
	FireShot(FireDirection, ActorUpVector);

	if (bDebugGizmos)
	{
		DrawGizmos();
	}
}

void AStickyFloorPawn::DrawGizmos() const
{
	const UWorld* world = GetWorld();

	const auto meshTr = ShipMeshComponent->GetComponentTransform();
	const FVector meshUp = meshTr.GetUnitAxis(EAxis::Z);
	const FVector meshFw = meshTr.GetUnitAxis(EAxis::X);
	const FVector meshRight = meshTr.GetUnitAxis(EAxis::Y);

	const FVector start = GetActorLocation();

	DrawDebugDirectionalArrow(world, start, start + meshUp * 250.0f, 40.0f, FColor::White);
	DrawDebugDirectionalArrow(world, start, start + meshFw * 250.0f, 40.0f, FColor::Cyan);
	DrawDebugDirectionalArrow(world, start, start + meshRight * 250.0f, 40.f, FColor::Magenta);
}

void AStickyFloorPawn::FireShot(FVector FireDirection, FVector ActorUpVector)
{
	// If it's ok to fire again
	if (bCanFire == true)
	{
		// If we are pressing fire stick in a direction
		if (FireDirection.SizeSquared() > 0.0f)
		{
			// Calculate projectile orientation based on the ship's orientation
			const FVector ProjectileRightVector = FVector::CrossProduct(ActorUpVector, FireDirection);
			const FMatrix ProjectileMatrix(FireDirection, ProjectileRightVector, ActorUpVector, FVector::ZeroVector);
			const FRotator ProjectileRotation = ProjectileMatrix.Rotator();

			// Spawn projectile at an offset from this pawn
			const FVector SpawnLocation = GetActorLocation() + FireDirection * GunOffset;

			UWorld* const World = GetWorld();
			if (World != NULL)
			{
				// spawn the projectile
				auto Projectile = World->SpawnActor<AStickyFloorProjectile>(SpawnLocation, ProjectileRotation);
				Projectile->SetupFloorCast(*StickToFloor);
			}

			bCanFire = false;
			World->GetTimerManager().SetTimer(TimerHandle_ShotTimerExpired, this, &AStickyFloorPawn::ShotTimerExpired, FireRate);

			// try and play the sound if specified
			if (FireSound != nullptr)
			{
				UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
			}

			bCanFire = false;
		}
	}
}

void AStickyFloorPawn::ShotTimerExpired()
{
	bCanFire = true;
}

