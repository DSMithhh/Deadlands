// Copyright Epic Games, Inc. All Rights Reserved.

#include "DeadlandsCharacter.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Deadlands.h"
#include "Interaction/DeadlandsInteractable.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

ADeadlandsCharacter::ADeadlandsCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
	
	// Create the first person mesh that will be viewed only by this character's owner
	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("First Person Mesh"));

	FirstPersonMesh->SetupAttachment(GetMesh());
	FirstPersonMesh->SetOnlyOwnerSee(true);
	FirstPersonMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
	FirstPersonMesh->SetCollisionProfileName(FName("NoCollision"));

	// Create the Camera Component	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("First Person Camera"));
	FirstPersonCameraComponent->SetupAttachment(FirstPersonMesh, FName("head"));
	FirstPersonCameraComponent->SetRelativeLocationAndRotation(FVector(-2.8f, 5.89f, 0.0f), FRotator(0.0f, 90.0f, -90.0f));
	FirstPersonCameraComponent->bUsePawnControlRotation = true;
	FirstPersonCameraComponent->bEnableFirstPersonFieldOfView = true;
	FirstPersonCameraComponent->bEnableFirstPersonScale = true;
	FirstPersonCameraComponent->FirstPersonFieldOfView = 70.0f;
	FirstPersonCameraComponent->FirstPersonScale = 0.6f;

	// configure the character comps
	GetMesh()->SetOwnerNoSee(true);
	GetMesh()->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::WorldSpaceRepresentation;

	GetCapsuleComponent()->SetCapsuleSize(34.0f, 96.0f);

	// Configure character movement
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
	GetCharacterMovement()->AirControl = 0.5f;
}

void ADeadlandsCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (InteractAction)
		{
			EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &ADeadlandsCharacter::TryInteract);
		}

		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ADeadlandsCharacter::DoJumpStart);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ADeadlandsCharacter::DoJumpEnd);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ADeadlandsCharacter::MoveInput);

		// Looking/Aiming
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ADeadlandsCharacter::LookInput);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ADeadlandsCharacter::LookInput);
	}
	else
	{
		UE_LOG(LogDeadlands, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}


void ADeadlandsCharacter::MoveInput(const FInputActionValue& Value)
{
	// get the Vector2D move axis
	FVector2D MovementVector = Value.Get<FVector2D>();

	// pass the axis values to the move input
	DoMove(MovementVector.X, MovementVector.Y);

}

void ADeadlandsCharacter::LookInput(const FInputActionValue& Value)
{
	// get the Vector2D look axis
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// pass the axis values to the aim input
	DoAim(LookAxisVector.X, LookAxisVector.Y);

}

void ADeadlandsCharacter::DoAim(float Yaw, float Pitch)
{
	if (GetController())
	{
		// pass the rotation inputs
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void ADeadlandsCharacter::DoMove(float Right, float Forward)
{
	if (GetController())
	{
		// pass the move inputs
		AddMovementInput(GetActorRightVector(), Right);
		AddMovementInput(GetActorForwardVector(), Forward);
	}
}

void ADeadlandsCharacter::DoJumpStart()
{
	// pass Jump to the character
	Jump();
}

void ADeadlandsCharacter::DoJumpEnd()
{
	// pass StopJumping to the character
	StopJumping();
}

void ADeadlandsCharacter::TryInteract()
{
    if (!IsLocallyControlled() || !GetWorld() || !FirstPersonCameraComponent)
    {
        return;
    }

    const FVector Start = FirstPersonCameraComponent->GetComponentLocation();
    const FVector End = Start + FirstPersonCameraComponent->GetForwardVector() * 250.f;
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(DeadlandsInteraction), false, this);
    // A small sweep is more forgiving than a single pixel-perfect ray.
    // It also handles narrow targets viewed at oblique angles.
    const FCollisionShape InteractionShape = FCollisionShape::MakeSphere(12.f);
    if (GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity,
        ECC_Visibility, InteractionShape, Params))
    {
        AActor* Target = Hit.GetActor();
        if (IsValid(Target) && Target->GetClass()->ImplementsInterface(UDeadlandsInteractable::StaticClass()))
        {
            ServerTryInteract(Target);
        }
    }
}

void ADeadlandsCharacter::ServerTryInteract_Implementation(AActor* RequestedTarget)
{
    if (!IsValid(RequestedTarget) || !GetWorld() ||
        !RequestedTarget->GetClass()->ImplementsInterface(UDeadlandsInteractable::StaticClass()))
    {
        return;
    }

    // Client traces from its animated first-person camera. On the server the
    // pawn eye position can differ, so validate reach and visibility to the
    // actual target instead of requiring an identical camera ray.
    const FVector EyeLocation = GetPawnViewLocation();
    const FBox TargetBounds = RequestedTarget->GetComponentsBoundingBox();
    const FVector TargetLocation = TargetBounds.GetCenter();
    const FVector ClosestPoint = TargetBounds.GetClosestPointTo(EyeLocation);

    constexpr float MaxInteractionDistance = 250.f;
    if (FVector::DistSquared(EyeLocation, ClosestPoint) >
        FMath::Square(MaxInteractionDistance))
    {
        UE_LOG(LogDeadlands, Verbose, TEXT("Interaction rejected: %s out of range"), *GetNameSafe(RequestedTarget));
        return;
    }

    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(DeadlandsServerInteraction), false, this);
    // Trace toward the target's bounds center, ensuring intervening walls block use.
    if (!GetWorld()->LineTraceSingleByChannel(Hit, EyeLocation, TargetLocation, ECC_Visibility, Params) ||
        Hit.GetActor() != RequestedTarget)
    {
        UE_LOG(LogDeadlands, Verbose, TEXT("Interaction rejected: %s blocked or not visible"), *GetNameSafe(RequestedTarget));
        return;
    }

    if (IDeadlandsInteractable::Execute_CanInteract(RequestedTarget, this))
    {
        UE_LOG(LogDeadlands, Log, TEXT("Server interaction: %s uses %s"), *GetNameSafe(this), *GetNameSafe(RequestedTarget));
        IDeadlandsInteractable::Execute_Interact(RequestedTarget, this);
    }
}

void ADeadlandsCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(ADeadlandsCharacter, Supplies, COND_OwnerOnly);
}

bool ADeadlandsCharacter::AddSupplies(int32 Amount)
{
    if (!HasAuthority() || Amount <= 0 || Supplies > SupplyCapacity - Amount)
    {
        return false;
    }
    Supplies += Amount;
    OnSuppliesChanged(Supplies);
    ForceNetUpdate();
    UE_LOG(LogDeadlands, Log, TEXT("%s now carries %d supplies"), *GetNameSafe(this), Supplies);
    return true;
}

void ADeadlandsCharacter::OnRep_Supplies()
{
    OnSuppliesChanged(Supplies);
}
