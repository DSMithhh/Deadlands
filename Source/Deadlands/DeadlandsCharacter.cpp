// Copyright Epic Games, Inc. All Rights Reserved.

#include "DeadlandsCharacter.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Deadlands.h"
#include "Interaction/DeadlandsInteractable.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/PlayerState.h"
#include "Persistence/DeadlandsWorldPersistence.h"
#include "Interaction/DeadlandsContainer.h"
#include "Inventory/DeadlandsInventoryWidget.h"
#include "GameFramework/PlayerController.h"
#include "Components/InputComponent.h"
#include "InputCoreTypes.h"
#include "Survival/DeadlandsVitalsComponent.h"
#include "Interaction/DeadlandsInteractionPrompt.h"
#include "Inventory/DeadlandsWorldPickup.h"
#include "Building/DeadlandsBarricade.h"
#include "Misc/Guid.h"
#include "TimerManager.h"
#include "Zombies/DeadlandsZombie.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Controller.h"

ADeadlandsCharacter::ADeadlandsCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    Vitals = CreateDefaultSubobject<UDeadlandsVitalsComponent>(TEXT("SurvivalVitals"));
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

    Flashlight = CreateDefaultSubobject<USpotLightComponent>(TEXT("SurvivalFlashlight"));
    Flashlight->SetupAttachment(FirstPersonCameraComponent);
    Flashlight->SetRelativeLocation(FVector(15.f, 0.f, -8.f));
    Flashlight->SetIntensity(6500.f);
    Flashlight->SetAttenuationRadius(1800.f);
    Flashlight->SetInnerConeAngle(16.f);
    Flashlight->SetOuterConeAngle(32.f);
    Flashlight->SetVisibility(false);

	// configure the character comps
	GetMesh()->SetOwnerNoSee(true);
	GetMesh()->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::WorldSpaceRepresentation;

	GetCapsuleComponent()->SetCapsuleSize(34.0f, 96.0f);

	// Configure character movement
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
	GetCharacterMovement()->AirControl = 0.5f;
    GetCharacterMovement()->MaxWalkSpeed = SurvivalWalkSpeed;
    GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = true;
    GetCharacterMovement()->MaxWalkSpeedCrouched = 220.f;
}

void ADeadlandsCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{	
	// Set up action bindings
	// Bind Tab independently of the Enhanced Input mapping context.
	PlayerInputComponent->BindKey(EKeys::Tab, IE_Pressed, this, &ADeadlandsCharacter::ToggleInventory);
    PlayerInputComponent->BindKey(EKeys::LeftShift, IE_Pressed, this, &ADeadlandsCharacter::BeginSprint);
    PlayerInputComponent->BindKey(EKeys::LeftShift, IE_Released, this, &ADeadlandsCharacter::EndSprint);
    PlayerInputComponent->BindKey(EKeys::L, IE_Pressed, this, &ADeadlandsCharacter::ToggleFlashlight);
    PlayerInputComponent->BindKey(EKeys::B, IE_Pressed, this, &ADeadlandsCharacter::PlaceBarricade);
    PlayerInputComponent->BindKey(EKeys::X, IE_Pressed, this, &ADeadlandsCharacter::DismantleBarricade);
    PlayerInputComponent->BindKey(EKeys::H, IE_Pressed, this, &ADeadlandsCharacter::DebugDamage);
    PlayerInputComponent->BindKey(EKeys::J, IE_Pressed, this, &ADeadlandsCharacter::DebugHeal);
    PlayerInputComponent->BindKey(EKeys::LeftControl, IE_Pressed, this, &ADeadlandsCharacter::ToggleCrouch);
    PlayerInputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &ADeadlandsCharacter::TryMeleeAttack);
    PlayerInputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &ADeadlandsCharacter::TryHeavyMeleeAttack);
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
            if (ADeadlandsZombie* Corpse = Cast<ADeadlandsZombie>(Target))
            {
                if (Corpse->GetZombieHealth() <= 0.f && IsValid(Corpse->GetCorpseLoot()))
                {
                    ShowInventory(Corpse->GetCorpseLoot());
                    return;
                }
            }
            if (ADeadlandsContainer* Container = Cast<ADeadlandsContainer>(Target))
            {
                ShowInventory(Container);
                ServerTryInteract(Target);
                return;
            }
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
    DOREPLIFETIME_CONDITION(ADeadlandsCharacter, Items, COND_OwnerOnly);
    DOREPLIFETIME(ADeadlandsCharacter, bIsSprinting);
    DOREPLIFETIME(ADeadlandsCharacter, Health);
    DOREPLIFETIME(ADeadlandsCharacter, bIsDead);
    DOREPLIFETIME_CONDITION(ADeadlandsCharacter, bBackpackEquipped, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(ADeadlandsCharacter, EquippedGear, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(ADeadlandsCharacter, Bleeding, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(ADeadlandsCharacter, Infection, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(ADeadlandsCharacter, ToolCondition, COND_OwnerOnly);
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

FString ADeadlandsCharacter::GetInventorySaveKey() const
{
    if (GetWorld() && GetWorld()->WorldType == EWorldType::PIE)
    {
        int32 Slot = 0;
        for (auto It = GetWorld()->GetPlayerControllerIterator(); It; ++It, ++Slot)
        {
            if (It->Get() == GetController())
                return FString::Printf(TEXT("Player|PIE|%d"), Slot);
        }
        return FString();
    }
    const APlayerState* PS = GetPlayerState();
    if (!PS) return FString();
    // Stable network identity when online; deterministic player name for PIE.
    // PlayerState numeric IDs are session-scoped and change across restarts.
    if (PS->GetUniqueId().IsValid())
        return FString::Printf(TEXT("Player|Net|%s"), *PS->GetUniqueId()->ToString());
    const FString Name = PS->GetPlayerName();
    if (!Name.IsEmpty())
        return FString::Printf(TEXT("Player|Name|%s"), *Name);
    return FString();
}

bool ADeadlandsCharacter::EnsureItemInventory()
{
    if (!HasAuthority()) return false;
    if (bItemsLoaded) return true;
    const FString Key = GetInventorySaveKey();
    if (Key.IsEmpty()) return false;
    FDeadlandsWorldPersistence::LoadInventory(this, Key, Items);
    FDeadlandsWorldPersistence::LoadSurvivalState(this, Key, bBackpackEquipped, Bleeding, Infection, ToolCondition);
    FDeadlandsWorldPersistence::LoadEquipment(this, Key, EquippedGear);
    EquippedGear.SetNum(3);
    bItemsLoaded = true;
    NotifyItemsChanged();
    return true;
}

void ADeadlandsCharacter::NotifyItemsChanged()
{
    OnItemsChanged();
    ForceNetUpdate();
}

void ADeadlandsCharacter::OnRep_Items()
{
    OnItemsChanged();
}

void ADeadlandsCharacter::ToggleInventory()
{
    if (!IsLocallyControlled()) return;
    if (InventoryWidget && InventoryWidget->IsInViewport())
    {
        CloseInventory();
        return;
    }
    ShowInventory(nullptr);
}

void ADeadlandsCharacter::ShowInventory(ADeadlandsContainer* Container)
{
    if (!IsLocallyControlled()) return;
    if (!InventoryWidget)
        InventoryWidget = CreateWidget<UDeadlandsInventoryWidget>(
            Cast<APlayerController>(GetController()), UDeadlandsInventoryWidget::StaticClass());
    if (!InventoryWidget) return;
    CurrentContainer = Container;
    InventoryWidget->SetInventoryContext(this, Container);
    if (!InventoryWidget->IsInViewport()) InventoryWidget->AddToViewport(20);
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        FInputModeGameAndUI Mode;
        Mode.SetHideCursorDuringCapture(false);
        Mode.SetWidgetToFocus(InventoryWidget->TakeWidget());
        PC->SetInputMode(Mode);
        PC->bShowMouseCursor = true;
    }
}

void ADeadlandsCharacter::RequestItemTransfer(ADeadlandsContainer* Container, FName ItemId, bool bTake, int32 Quantity)
{
    if (!IsLocallyControlled() || !IsValid(Container) || Quantity < 1 || Quantity > 30 ||
        FDeadlandsItems::MaxStack(ItemId) == 0) return;
    ServerTransferItems(Container, ItemId, bTake, Quantity);
}

void ADeadlandsCharacter::RequestReorderItem(bool bContainer,ADeadlandsContainer* Container,int32 SourceIndex,int32 TargetIndex)
{
 if(IsLocallyControlled())ServerReorderItem(bContainer,Container,SourceIndex,TargetIndex);
}
void ADeadlandsCharacter::ServerReorderItem_Implementation(bool bContainer,ADeadlandsContainer* Container,int32 SourceIndex,int32 TargetIndex)
{
 if(bIsDead||SourceIndex<0||TargetIndex<0||TargetIndex>=16||SourceIndex==TargetIndex)return;
 if(bContainer)
 {
  if(!IsValid(Container)||FVector::DistSquared(GetActorLocation(),Container->GetActorLocation())>FMath::Square(300.f))return;
  Container->ReorderSlots(SourceIndex,TargetIndex);
  return;
 }
 if(!EnsureItemInventory()||!Items.IsValidIndex(SourceIndex)||!FDeadlandsItems::Valid(Items[SourceIndex]))return;
 TArray<FDeadlandsItemStack> Updated=Items;
 Updated.SetNum(FMath::Max(Updated.Num(),TargetIndex+1));
 if(Updated[TargetIndex].ItemId==Updated[SourceIndex].ItemId)
 {
  const int32 Space=FDeadlandsItems::MaxStack(Updated[TargetIndex].ItemId)-Updated[TargetIndex].Quantity;
  if(Space>0)
  {
   const int32 Count=FMath::Min(Space,Updated[SourceIndex].Quantity);
   Updated[TargetIndex].Quantity+=Count;Updated[SourceIndex].Quantity-=Count;
   if(Updated[SourceIndex].Quantity==0)Updated[SourceIndex].ItemId=NAME_None;
  }
  else Swap(Updated[SourceIndex],Updated[TargetIndex]);
 }
 else Swap(Updated[SourceIndex],Updated[TargetIndex]);
 while(!Updated.IsEmpty()&&Updated.Last().ItemId.IsNone())Updated.Pop();
 if(!FDeadlandsWorldPersistence::SaveInventory(this,GetInventorySaveKey(),Updated))return;
 Items=MoveTemp(Updated);NotifyItemsChanged();ForceNetUpdate();
}
void ADeadlandsCharacter::ServerTransferItems_Implementation(ADeadlandsContainer* Container, FName ItemId, bool bTake, int32 Quantity)
{
    if (!IsValid(Container) || Quantity < 1 || Quantity > 30 ||
        FDeadlandsItems::MaxStack(ItemId) == 0 || !EnsureItemInventory()) return;
    const FVector Eye = GetPawnViewLocation();
    const FBox Bounds = Container->GetComponentsBoundingBox();
    const FVector ContainerPoint = Bounds.GetClosestPointTo(Eye);
    const bool bNearBag = FVector::DistSquared(Eye, ContainerPoint) <= FMath::Square(250.f);
    ADeadlandsZombie* MatchingCorpse = nullptr;
    if (Container->IsCorpseBag())
    {
        for (TActorIterator<ADeadlandsZombie> It(GetWorld()); It; ++It)
        {
            if (IsValid(*It) && It->GetCorpseLoot() == Container && It->GetZombieHealth() <= 0.f)
            {
                MatchingCorpse = *It;
                break;
            }
        }
    }
    const bool bNearCorpse = MatchingCorpse &&
        FVector::DistSquared(Eye, MatchingCorpse->GetComponentsBoundingBox().GetClosestPointTo(Eye)) <= FMath::Square(250.f);
    if (!bNearBag && !bNearCorpse) return;
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(DeadlandsInventoryTransfer), false, this);
    const FVector TraceTarget = bNearCorpse ? MatchingCorpse->GetComponentsBoundingBox().GetCenter() : Bounds.GetCenter();
    if (!GetWorld()->LineTraceSingleByChannel(Hit, Eye, TraceTarget, ECC_Visibility, Params)) return;
    if (Hit.GetActor() != Container && Hit.GetActor() != MatchingCorpse) return;
    Container->TransferItemsToCharacter(this, ItemId, bTake, Quantity);
}

void ADeadlandsCharacter::CloseInventory()
{
    if (!IsLocallyControlled()) return;
    if (InventoryWidget && InventoryWidget->IsInViewport())
        InventoryWidget->RemoveFromParent();
    CurrentContainer = nullptr;
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        PC->SetInputMode(FInputModeGameOnly());
        PC->bShowMouseCursor = false;
    }
}

void ADeadlandsCharacter::ApplySprintSpeed()
{
    if (GetCharacterMovement())
        GetCharacterMovement()->MaxWalkSpeed = bIsSprinting ? SurvivalSprintSpeed : SurvivalWalkSpeed;
}

void ADeadlandsCharacter::BeginSprint()
{
    if (!IsLocallyControlled()) return;
    if (Vitals && !Vitals->CanSprint()) return;
    bIsSprinting = true;
    if (HasAuthority() && Vitals) Vitals->SetSprinting(true);
    ApplySprintSpeed();
    if (!HasAuthority()) ServerSetSprinting(true);
}

void ADeadlandsCharacter::EndSprint()
{
    if (!IsLocallyControlled()) return;
    bIsSprinting = false;
    if (HasAuthority() && Vitals) Vitals->SetSprinting(false);
    ApplySprintSpeed();
    if (!HasAuthority()) ServerSetSprinting(false);
}

void ADeadlandsCharacter::ServerSetSprinting_Implementation(bool bNewSprinting)
{
    bIsSprinting = bNewSprinting && (!Vitals || Vitals->CanSprint());
    if (Vitals) Vitals->SetSprinting(bIsSprinting);
    ApplySprintSpeed();
    ForceNetUpdate();
}

void ADeadlandsCharacter::OnRep_Sprinting()
{
    ApplySprintSpeed();
}

void ADeadlandsCharacter::ToggleFlashlight()
{
    if (!IsLocallyControlled() || !Flashlight) return;
    bFlashlightOn = !bFlashlightOn;
    Flashlight->SetVisibility(bFlashlightOn);
    UE_LOG(LogDeadlands, Log, TEXT("Flashlight %s"), bFlashlightOn ? TEXT("ON") : TEXT("OFF"));
}

void ADeadlandsCharacter::DebugDamage()
{
    if (IsLocallyControlled()) ServerDebugChangeHealth(-25.f);
}

void ADeadlandsCharacter::DebugHeal()
{
    if (IsLocallyControlled()) ServerDebugChangeHealth(25.f);
}

void ADeadlandsCharacter::ApplyCombatDamage(float Amount)
{
    if (!HasAuthority() || bIsDead || Amount <= 0.f) return;
    EnsureItemInventory();
    ApplySurvivalDamage(Amount * (1.f - GetArmorReduction()));
    if (EnsureItemInventory())
    {
        Bleeding = FMath::Min(100.f, Bleeding + Amount * (1.f - GetArmorReduction()) * 0.35f);
        if (FMath::FRand() < 0.15f)
            Infection = FMath::Min(100.f, Infection + 20.f);
        FDeadlandsWorldPersistence::SaveSurvivalState(this, GetInventorySaveKey(), bBackpackEquipped, Bleeding, Infection, ToolCondition);
    }
}

void ADeadlandsCharacter::TryMeleeAttack()
{
    if (!IsLocallyControlled() || bIsDead || (InventoryWidget && InventoryWidget->IsInViewport())) return;
    ServerMeleeAttack();
}
void ADeadlandsCharacter::TryHeavyMeleeAttack()
{
    if (!IsLocallyControlled() || bIsDead || (InventoryWidget && InventoryWidget->IsInViewport())) return;
    ServerHeavyMeleeAttack();
}
void ADeadlandsCharacter::ServerMeleeAttack_Implementation()
{
    ExecuteMeleeAttack(false);
}
void ADeadlandsCharacter::ServerHeavyMeleeAttack_Implementation()
{
    ExecuteMeleeAttack(true);
}
void ADeadlandsCharacter::ExecuteMeleeAttack(bool bHeavy)
{
    if (!HasAuthority() || bIsDead || !GetWorld()) return;
    const float Now = GetWorld()->GetTimeSeconds();
    const float Cooldown = bHeavy ? 1.35f : MeleeCooldown;
    if (Now - LastMeleeTime < Cooldown) return;
    const float StaminaCost = bHeavy ? 28.f : 10.f;
    if (Vitals && !Vitals->SpendStamina(StaminaCost))
    {
        ClientDetailedCombatFeedback(TEXT("NOT ENOUGH STAMINA"));
        return;
    }
    LastMeleeTime = Now;
    bool bToolEquipped = false;
    if (EnsureItemInventory())
    {
        for (const auto& Stack : Items)
            if (Stack.ItemId == TEXT("Tools") && Stack.Quantity > 0 && ToolCondition > 0)
                bToolEquipped = true;
        if (bToolEquipped)
        {
            ToolCondition = FMath::Max(0, ToolCondition - (bHeavy ? 3 : 1));
            FDeadlandsWorldPersistence::SaveSurvivalState(this, GetInventorySaveKey(), bBackpackEquipped, Bleeding, Infection, ToolCondition);
        }
    }
    const float Damage = (bHeavy ? 38.f : MeleeDamage) * (bToolEquipped ? 1.25f : 1.f);
    const FVector Start = GetPawnViewLocation();
    const FVector Direction = GetController() ? GetController()->GetControlRotation().Vector() : GetActorForwardVector();
    const FVector End = Start + Direction * (bHeavy ? MeleeRange + 30.f : MeleeRange);
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(DeadlandsMelee), false, this);
    const bool bHit = GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity,
        ECC_Pawn, FCollisionShape::MakeSphere(bHeavy ? 32.f : 24.f), Params);
    ADeadlandsCharacter* Victim = bHit ? Cast<ADeadlandsCharacter>(Hit.GetActor()) : nullptr;
    ADeadlandsZombie* Zombie = bHit ? Cast<ADeadlandsZombie>(Hit.GetActor()) : nullptr;
    const bool bPlayerHit = IsValid(Victim) && Victim != this && !Victim->bIsDead && bAllowFriendlyFire;
    const bool bZombieHit = IsValid(Zombie) && Zombie->GetZombieHealth() > 0.f;
    bool bHeadshot = false;
    if (bZombieHit)
    {
        const float HeightAboveFeet = Hit.ImpactPoint.Z - (Zombie->GetActorLocation().Z - Zombie->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
        bHeadshot = HeightAboveFeet >= 140.f;
        Zombie->ReceiveMeleeDamage(Damage, bHeadshot, bHeavy);
    }
    if (bPlayerHit) Victim->ApplyCombatDamage(Damage);
    for (TActorIterator<ADeadlandsZombie> It(GetWorld()); It; ++It)
        if (IsValid(*It)) (*It)->HearCombatNoise(GetActorLocation());
    if (bZombieHit)
        ClientDetailedCombatFeedback(FString::Printf(TEXT("%s%s  -%.0f"), bHeadshot ? TEXT("HEADSHOT!  ") : TEXT("HIT  "), bHeavy ? TEXT("HEAVY") : TEXT("LIGHT"), Damage * (bHeadshot ? 2.5f : 1.f)));
    else if (bPlayerHit)
        ClientDetailedCombatFeedback(FString::Printf(TEXT("PLAYER HIT  -%.0f"), Damage));
    else
        ClientDetailedCombatFeedback(bHeavy ? TEXT("HEAVY SWING - MISS") : TEXT("LIGHT SWING - MISS"));
}
void ADeadlandsCharacter::ClientCombatFeedback_Implementation(bool bHit)
{
    CombatFeedback = bHit ? TEXT("MELEE HIT") : TEXT("MELEE MISS");
    CombatFeedbackUntil = GetWorld() ? GetWorld()->GetTimeSeconds() + 0.8f : 0.f;
}
void ADeadlandsCharacter::ClientDetailedCombatFeedback_Implementation(const FString& Message)
{
    CombatFeedback = Message;
    CombatFeedbackUntil = GetWorld() ? GetWorld()->GetTimeSeconds() + 1.2f : 0.f;
}

void ADeadlandsCharacter::ApplySurvivalDamage(float Amount)
{
    if (!HasAuthority() || bIsDead || Amount <= 0.f) return;
    Health = FMath::Clamp(Health - Amount, 0.f, MaxHealth);
    OnRep_Health();
    ForceNetUpdate();
    if (Health <= 0.f) Die();
}

void ADeadlandsCharacter::ApplyDeathState()
{
    if (UCharacterMovementComponent* Movement = GetCharacterMovement())
    {
        if (bIsDead) Movement->DisableMovement();
        else Movement->SetMovementMode(MOVE_Walking);
    }
}

void ADeadlandsCharacter::OnRep_Dead()
{
    ApplyDeathState();
}

void ADeadlandsCharacter::Die()
{
    if (!HasAuthority() || bIsDead) return;
    bIsDead = true;
    bIsSprinting = false;
    if (Vitals) Vitals->SetSprinting(false);
    ApplyDeathState();
    ForceNetUpdate();
    GetWorldTimerManager().SetTimer(RespawnTimer, this, &ADeadlandsCharacter::RespawnSurvivor, RespawnDelaySeconds, false);
}

void ADeadlandsCharacter::RespawnSurvivor()
{
    if (!HasAuthority()) return;
    SetActorTransform(InitialSpawnTransform, false, nullptr, ETeleportType::TeleportPhysics);
    Health = MaxHealth;
    if (Vitals) Vitals->RestoreAfterRespawn();
    bIsDead = false;
    ApplyDeathState();
    OnRep_Health();
    ForceNetUpdate();
}

void ADeadlandsCharacter::ServerDebugChangeHealth_Implementation(float Delta)
{
    // Temporary testing-only controls; remove before gameplay release.
    if (!FMath::IsFinite(Delta) || (Delta != 25.f && Delta != -25.f)) return;
    if (bIsDead) return;
    Health = FMath::Clamp(Health + Delta, 0.f, MaxHealth);
    if (Health <= 0.f) Die();
    OnRep_Health();
    ForceNetUpdate();
}

void ADeadlandsCharacter::OnRep_Health()
{
    OnHealthChanged(Health, MaxHealth);
    UE_LOG(LogDeadlands, Log, TEXT("%s health: %.0f / %.0f"), *GetNameSafe(this), Health, MaxHealth);
}

void ADeadlandsCharacter::BeginPlay()
{
    Super::BeginPlay();
    if (HasAuthority())
    {
        InitialSpawnTransform = GetActorTransform();
        EnsureItemInventory(); // PlayerState may arrive later; Tick retries.
    }
    if (FirstPersonCameraComponent)
    {
        StandingCameraRelativeLocation = FirstPersonCameraComponent->GetRelativeLocation();
        bCameraBaseCaptured = true;
    }
    if (IsLocallyControlled())
    {
        if (APlayerController* PC = Cast<APlayerController>(GetController()))
        {
            InteractionPrompt = CreateWidget<UDeadlandsInteractionPrompt>(PC, UDeadlandsInteractionPrompt::StaticClass());
            if (InteractionPrompt) InteractionPrompt->AddToViewport(5);
        }
    }
}

void ADeadlandsCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (HasAuthority() && !bItemsLoaded) EnsureItemInventory();
    if (HasAuthority() && bItemsLoaded && !bIsDead && (Bleeding > 0.f || Infection > 0.f))
    {
        ApplySurvivalDamage((Bleeding * 0.018f + Infection * 0.006f) * DeltaSeconds);
        ConditionSaveClock += DeltaSeconds;
        if (ConditionSaveClock >= 10.f)
        {
            ConditionSaveClock = 0.f;
            FDeadlandsWorldPersistence::SaveSurvivalState(this, GetInventorySaveKey(), bBackpackEquipped, Bleeding, Infection, ToolCondition);
        }
    }
    // The first-person camera is attached to the head mesh rather than the capsule.
    // Move it locally so crouch is visible even before crouch animations exist.
    if (IsLocallyControlled() && FirstPersonCameraComponent && bCameraBaseCaptured)
    {
        // The head socket is rotated, so camera-relative Z is not world up/down.
        // Convert a vertical world-space offset into the parent socket's local space.
        const USceneComponent* CameraParent = FirstPersonCameraComponent->GetAttachParent();
        const FVector WorldDrop(0.f, 0.f, bIsCrouched ? -40.f : 0.f);
        const FVector LocalDrop = CameraParent
            ? CameraParent->GetSocketTransform(FirstPersonCameraComponent->GetAttachSocketName(), RTS_World).InverseTransformVectorNoScale(WorldDrop)
            : WorldDrop;
        const FVector Target = StandingCameraRelativeLocation + LocalDrop;
        FirstPersonCameraComponent->SetRelativeLocation(FMath::VInterpTo(
            FirstPersonCameraComponent->GetRelativeLocation(), Target, DeltaSeconds, 10.f));
    }
    if (HasAuthority() && bIsSprinting && Vitals && !Vitals->CanSprint())
    {
        bIsSprinting = false;
        Vitals->SetSprinting(false);
        ApplySprintSpeed();
        ForceNetUpdate();
    }
    if (IsLocallyControlled() && bIsSprinting && Vitals && !Vitals->CanSprint())
    {
        bIsSprinting = false;
        ApplySprintSpeed();
    }
    if (!IsLocallyControlled() || !InteractionPrompt || !FirstPersonCameraComponent || !GetWorld()) return;
    if (Vitals)
    {
        auto Bar = [](float Value) -> FString
        {
            const int32 Filled = FMath::Clamp(FMath::RoundToInt(Value / 10.f), 0, 10);
            return FString::ChrN(Filled, TEXT('|')) + FString::ChrN(10 - Filled, TEXT('.'));
        };
        InteractionPrompt->SetVitalsDebug(FString::Printf(
            TEXT("HEALTH   [%s] %.0f\nSTAMINA  [%s] %.0f\nHUNGER   [%s] %.0f\nTHIRST   [%s] %.0f\nCROUCH: %s (requested: %s)\n%s\n%s"),
            *Bar(Health), Health, *Bar(Vitals->GetStamina()), Vitals->GetStamina(),
            *Bar(Vitals->GetHunger()), Vitals->GetHunger(),
            *Bar(Vitals->GetThirst()), Vitals->GetThirst(),
            bIsCrouched ? TEXT("YES") : TEXT("NO"),
            GetCharacterMovement() && GetCharacterMovement()->bWantsToCrouch ? TEXT("YES") : TEXT("NO"),
            bIsDead ? TEXT("DEAD - RESPAWNING SOON") : TEXT(""),
            GetWorld()->GetTimeSeconds() < CombatFeedbackUntil ? *CombatFeedback : TEXT("")));
    }
    InteractionPromptCountdown -= DeltaSeconds;
    if (InteractionPromptCountdown > 0.f) return;
    InteractionPromptCountdown = 0.1f;
    if (InventoryWidget && InventoryWidget->IsInViewport())
    {
        InteractionPrompt->SetPrompt(TEXT(""));
        return;
    }
    const FVector Start = FirstPersonCameraComponent->GetComponentLocation();
    const FVector End = Start + FirstPersonCameraComponent->GetForwardVector() * 250.f;
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(DeadlandsPrompt), false, this);
    const bool bHit = GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity,
        ECC_Visibility, FCollisionShape::MakeSphere(12.f), Params);
    AActor* Target = bHit ? Hit.GetActor() : nullptr;
    const bool bInteractable = IsValid(Target) &&
        Target->GetClass()->ImplementsInterface(UDeadlandsInteractable::StaticClass()) &&
        IDeadlandsInteractable::Execute_CanInteract(Target, this);
    InteractionPrompt->SetPrompt(bInteractable ? TEXT("[F] Interact") : TEXT(""));
}

void ADeadlandsCharacter::ToggleCrouch()
{
    if (!IsLocallyControlled()) return;
    if (GetCharacterMovement() && GetCharacterMovement()->bWantsToCrouch) UnCrouch();
    else
    {
        EndSprint();
        Crouch();
    }
}
void ADeadlandsCharacter::RequestConsumeItem(FName ItemId)
{
    if (IsLocallyControlled() && (ItemId == TEXT("Food") || ItemId == TEXT("Water") || ItemId == TEXT("Medicine") || ItemId == TEXT("Bandage") || ItemId == TEXT("Antibiotics")))
        ServerConsumeItem(ItemId);
}
void ADeadlandsCharacter::RequestUseContainerItem(ADeadlandsContainer* Container, FName ItemId)
{
    if (IsLocallyControlled() && IsValid(Container)) ServerUseContainerItem(Container, ItemId);
}
void ADeadlandsCharacter::ServerUseContainerItem_Implementation(ADeadlandsContainer* Container, FName ItemId)
{
    if (bIsDead || !Vitals || !IsValid(Container) || !EnsureItemInventory()) return;
    if (ItemId != TEXT("Food") && ItemId != TEXT("Water") && ItemId != TEXT("Medicine") &&
        ItemId != TEXT("Bandage") && ItemId != TEXT("Antibiotics") && ItemId != TEXT("Backpack") &&
        ItemId != TEXT("Jacket") && ItemId != TEXT("Pants") && ItemId != TEXT("ArmorVest")) return;
    if ((ItemId == TEXT("Food") && Vitals->GetHunger() >= 100.f) ||
        (ItemId == TEXT("Water") && Vitals->GetThirst() >= 100.f) ||
        (ItemId == TEXT("Medicine") && Health >= MaxHealth) ||
        (ItemId == TEXT("Bandage") && Bleeding <= 0.f) ||
        (ItemId == TEXT("Antibiotics") && Infection <= 0.f) ||
        (ItemId == TEXT("Backpack") && bBackpackEquipped) ||
        ((ItemId == TEXT("Jacket") || ItemId == TEXT("Pants") || ItemId == TEXT("ArmorVest")) && IsGearEquipped(ItemId))) return;
    // Reuse the same server-side reach and obstruction checks as manual transfers.
    // No client-side consumption is trusted.
    const FVector Eye = GetPawnViewLocation();
    ADeadlandsZombie* MatchingCorpse = nullptr;
    if (Container->IsCorpseBag())
        for (TActorIterator<ADeadlandsZombie> It(GetWorld()); It; ++It)
            if (IsValid(*It) && It->GetCorpseLoot() == Container && It->GetZombieHealth() <= 0.f)
            { MatchingCorpse = *It; break; }
    const FBox Bounds = Container->GetComponentsBoundingBox();
    const bool bNearBag = FVector::DistSquared(Eye, Bounds.GetClosestPointTo(Eye)) <= FMath::Square(250.f);
    const bool bNearCorpse = MatchingCorpse &&
        FVector::DistSquared(Eye, MatchingCorpse->GetComponentsBoundingBox().GetClosestPointTo(Eye)) <= FMath::Square(250.f);
    if (!bNearBag && !bNearCorpse) return;
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(DeadlandsUseContainerItem), false, this);
    const FVector Target = bNearCorpse ? MatchingCorpse->GetComponentsBoundingBox().GetCenter() : Bounds.GetCenter();
    if (!GetWorld()->LineTraceSingleByChannel(Hit, Eye, Target, ECC_Visibility, Params) ||
        (Hit.GetActor() != Container && Hit.GetActor() != MatchingCorpse)) return;
    if (!Container->TransferItemsToCharacter(this, ItemId, true, 1)) return;
    if (ItemId == TEXT("Backpack")) ServerEquipBackpack_Implementation();
    else if (ItemId == TEXT("Jacket") || ItemId == TEXT("Pants") || ItemId == TEXT("ArmorVest")) ServerToggleEquipment_Implementation(ItemId);
    else ServerConsumeItem_Implementation(ItemId);
}
void ADeadlandsCharacter::ServerConsumeItem_Implementation(FName ItemId)
{
    if (bIsDead || !Vitals || !EnsureItemInventory()) return;
    if (ItemId != TEXT("Food") && ItemId != TEXT("Water") && ItemId != TEXT("Medicine") && ItemId != TEXT("Bandage") && ItemId != TEXT("Antibiotics")) return;
    TArray<FDeadlandsItemStack> NewItems = Items;
    if (!FDeadlandsItems::RemoveOne(NewItems, ItemId)) return;
    // Only consume when the corresponding vital needs replenishment.
    if ((ItemId == TEXT("Food") && Vitals->GetHunger() >= 100.f) ||
        (ItemId == TEXT("Water") && Vitals->GetThirst() >= 100.f) ||
        (ItemId == TEXT("Medicine") && Health >= MaxHealth) ||
        (ItemId == TEXT("Bandage") && Bleeding <= 0.f) ||
        (ItemId == TEXT("Antibiotics") && Infection <= 0.f)) return;
    if (!FDeadlandsWorldPersistence::SaveInventory(this, GetInventorySaveKey(), NewItems)) return;
    Items = MoveTemp(NewItems);
    if (ItemId == TEXT("Medicine"))
    {
        Health = FMath::Min(MaxHealth, Health + 40.f);
        OnRep_Health();
    }
    else if (ItemId == TEXT("Bandage")) Bleeding = FMath::Max(0.f, Bleeding - 50.f);
    else if (ItemId == TEXT("Antibiotics")) Infection = FMath::Max(0.f, Infection - 60.f);
    else Vitals->Consume(ItemId);
    FDeadlandsWorldPersistence::SaveSurvivalState(this, GetInventorySaveKey(), bBackpackEquipped, Bleeding, Infection, ToolCondition);
    NotifyItemsChanged();
    ForceNetUpdate();
}
void ADeadlandsCharacter::RequestDropItem(FName ItemId)
{
    if (IsLocallyControlled() && FDeadlandsItems::MaxStack(ItemId) > 0)
        ServerDropItem(ItemId);
}
void ADeadlandsCharacter::ServerDropItem_Implementation(FName ItemId)
{
    if (bIsDead || !GetWorld() || !EnsureItemInventory() || FDeadlandsItems::MaxStack(ItemId) == 0) return;
    TArray<FDeadlandsItemStack> NewItems = Items;
    if (!FDeadlandsItems::RemoveOne(NewItems, ItemId)) return;
    const FVector SpawnLocation = GetActorLocation() + GetActorForwardVector() * 110.f + FVector(0.f, 0.f, 35.f);
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    ADeadlandsWorldPickup* Pickup = GetWorld()->SpawnActor<ADeadlandsWorldPickup>(ADeadlandsWorldPickup::StaticClass(), SpawnLocation, FRotator::ZeroRotator, Params);
    if (!Pickup) return;
    Pickup->InitializePickup(ItemId);
    if (!FDeadlandsWorldPersistence::SaveInventory(this, GetInventorySaveKey(), NewItems))
    {
        Pickup->Destroy();
        return;
    }
    Items = MoveTemp(NewItems);
    NotifyItemsChanged();
}

bool ADeadlandsCharacter::SaveSurvivalAndItems(const TArray<FDeadlandsItemStack>& NewItems, bool NewBackpack, float NewBleeding, float NewInfection, int32 NewCondition)
{
    if (!HasAuthority() || !EnsureItemInventory()) return false;
    if (!FDeadlandsWorldPersistence::SaveInventoryAndSurvivalState(this, GetInventorySaveKey(), NewItems, NewBackpack, NewBleeding, NewInfection, NewCondition)) return false;
    Items = NewItems;
    bBackpackEquipped = NewBackpack;
    Bleeding = NewBleeding;
    Infection = NewInfection;
    ToolCondition = NewCondition;
    NotifyItemsChanged();
    return true;
}
bool ADeadlandsCharacter::IsGearEquipped(FName ItemId) const
{
    const int32 Slot = ItemId == TEXT("Jacket") ? 0 : ItemId == TEXT("Pants") ? 1 : ItemId == TEXT("ArmorVest") ? 2 : INDEX_NONE;
    return Slot != INDEX_NONE && EquippedGear.IsValidIndex(Slot) && EquippedGear[Slot].ItemId == ItemId && EquippedGear[Slot].Quantity > 0;
}
float ADeadlandsCharacter::GetArmorReduction() const
{
    return IsGearEquipped(TEXT("ArmorVest")) ? 0.30f : 0.f;
}
void ADeadlandsCharacter::RequestToggleEquipment(FName ItemId)
{
    if (IsLocallyControlled()) ServerToggleEquipment(ItemId);
}
void ADeadlandsCharacter::ServerToggleEquipment_Implementation(FName ItemId)
{
    if (bIsDead || !EnsureItemInventory()) return;
    const int32 Slot = ItemId == TEXT("Jacket") ? 0 : ItemId == TEXT("Pants") ? 1 : ItemId == TEXT("ArmorVest") ? 2 : INDEX_NONE;
    if (Slot == INDEX_NONE) return;
    TArray<FDeadlandsItemStack> UpdatedItems = Items;
    TArray<FDeadlandsItemStack> UpdatedGear = EquippedGear;
    UpdatedGear.SetNum(3);
    if (UpdatedGear[Slot].ItemId == ItemId && UpdatedGear[Slot].Quantity > 0)
    {
        const int32 NewCapacity = GetSupplyCapacity() - (ItemId == TEXT("Jacket") ? 4 : ItemId == TEXT("Pants") ? 2 : 0);
        if (!FDeadlandsItems::AddOne(UpdatedItems, ItemId, NewCapacity)) return;
        UpdatedGear[Slot] = FDeadlandsItemStack();
    }
    else
    {
        if (!FDeadlandsItems::RemoveOne(UpdatedItems, ItemId)) return;
        UpdatedGear[Slot].ItemId = ItemId;
        UpdatedGear[Slot].Quantity = 1;
    }
    if (!FDeadlandsWorldPersistence::SaveEquipmentAndInventory(this, GetInventorySaveKey(), UpdatedGear, UpdatedItems)) return;
    EquippedGear = MoveTemp(UpdatedGear);
    Items = MoveTemp(UpdatedItems);
    NotifyItemsChanged();
}
void ADeadlandsCharacter::RequestEquipBackpack()
{
    if (IsLocallyControlled()) ServerEquipBackpack();
}
void ADeadlandsCharacter::ServerEquipBackpack_Implementation()
{
    if (bIsDead || !EnsureItemInventory()) return;
    TArray<FDeadlandsItemStack> Updated = Items;
    if (!bBackpackEquipped)
    {
        if (!FDeadlandsItems::RemoveOne(Updated, TEXT("Backpack"))) return;
        SaveSurvivalAndItems(Updated, true, Bleeding, Infection, ToolCondition);
    }
    else
    {
        if (!FDeadlandsItems::AddOne(Updated, TEXT("Backpack"), SupplyCapacity)) return;
        if (FDeadlandsItems::TotalWeight(Updated) > SupplyCapacity) return;
        SaveSurvivalAndItems(Updated, false, Bleeding, Infection, ToolCondition);
    }
}
void ADeadlandsCharacter::RequestCraft(FName Recipe)
{
    if (IsLocallyControlled()) ServerCraft(Recipe);
}
void ADeadlandsCharacter::ServerCraft_Implementation(FName Recipe)
{
    if (bIsDead || !EnsureItemInventory()) return;
    FName Output = NAME_None;
    FName IngredientA = NAME_None, IngredientB = NAME_None;
    int32 CountA = 0, CountB = 0;
    if (Recipe == TEXT("Bandage")) { Output = TEXT("Bandage"); IngredientA = TEXT("Cloth"); CountA = 2; }
    else if (Recipe == TEXT("Tools")) { Output = TEXT("Tools"); IngredientA = TEXT("Scrap"); CountA = 3; IngredientB = TEXT("Materials"); CountB = 2; }
    else if (Recipe == TEXT("Medicine")) { Output = TEXT("Medicine"); IngredientA = TEXT("Bandage"); CountA = 2; IngredientB = TEXT("Antibiotics"); CountB = 1; }
    else return;
    TArray<FDeadlandsItemStack> Updated = Items;
    for (int32 I = 0; I < CountA; ++I)
        if (!FDeadlandsItems::RemoveOne(Updated, IngredientA)) return;
    for (int32 I = 0; I < CountB; ++I)
        if (!FDeadlandsItems::RemoveOne(Updated, IngredientB)) return;
    if (!FDeadlandsItems::AddOne(Updated, Output, GetSupplyCapacity())) return;
    SaveSurvivalAndItems(Updated, bBackpackEquipped, Bleeding, Infection, ToolCondition);
}
void ADeadlandsCharacter::RequestRepairTools()
{
    if (IsLocallyControlled()) ServerRepairTools();
}
void ADeadlandsCharacter::ServerRepairTools_Implementation()
{
    if (bIsDead || !EnsureItemInventory() || ToolCondition >= 100) return;
    TArray<FDeadlandsItemStack> Updated = Items;
    bool HasTools = false;
    for (const auto& Stack : Items)
        if (Stack.ItemId == TEXT("Tools") && Stack.Quantity > 0) HasTools = true;
    if (!HasTools || !FDeadlandsItems::RemoveOne(Updated, TEXT("Scrap")) ||
        !FDeadlandsItems::RemoveOne(Updated, TEXT("Materials"))) return;
    SaveSurvivalAndItems(Updated, bBackpackEquipped, Bleeding, Infection, 100);
}

void ADeadlandsCharacter::PlaceBarricade()
{
 if(IsLocallyControlled())ServerPlaceBarricade();
}
void ADeadlandsCharacter::ServerPlaceBarricade_Implementation()
{
 if(bIsDead||!GetWorld()||!EnsureItemInventory())return;
 TArray<FDeadlandsItemStack> Updated=Items;
 for(int32 i=0;i<3;++i)if(!FDeadlandsItems::RemoveOne(Updated,TEXT("Materials")))return;
 for(int32 i=0;i<2;++i)if(!FDeadlandsItems::RemoveOne(Updated,TEXT("Scrap")))return;
 const FVector Eye=GetPawnViewLocation();
 const FVector Direction=GetBaseAimRotation().Vector();
 FHitResult Hit;
 FCollisionQueryParams Params(SCENE_QUERY_STAT(DeadlandsBuildPlacement),false,this);
 // Building requires an unobstructed view of the intended ground placement area.
 // Place on ground in front of the player, not inside the surface being aimed at.
 const FVector Candidate=GetActorLocation()+FVector(Direction.X,Direction.Y,0.f).GetSafeNormal()*210.f;
 FHitResult Ground;
 FCollisionQueryParams GroundParams(SCENE_QUERY_STAT(DeadlandsBuildGround),false,this);
 if(!GetWorld()->LineTraceSingleByChannel(Ground,Candidate+FVector(0.f,0.f,150.f),Candidate-FVector(0.f,0.f,240.f),ECC_Visibility,GroundParams))return;
 if(Ground.ImpactNormal.Z<0.65f)return;
 const FRotator Rotation(0.f,GetBaseAimRotation().Yaw+90.f,0.f);
 const FVector Location=Ground.ImpactPoint+FVector(0.f,0.f,75.f);
 const FCollisionShape Shape=FCollisionShape::MakeBox(FVector(14.f,110.f,76.f));
 FCollisionQueryParams BlockParams(SCENE_QUERY_STAT(DeadlandsBuildObstruction),false,this);
 if(GetWorld()->OverlapBlockingTestByChannel(Location,Rotation.Quaternion(),ECC_WorldStatic,Shape,BlockParams))return;
 if(GetWorld()->OverlapBlockingTestByChannel(Location,Rotation.Quaternion(),ECC_WorldDynamic,Shape,BlockParams))return;
 const FTransform Transform(Rotation,Location);
 const FString Id=FGuid::NewGuid().ToString();
 if(!FDeadlandsWorldPersistence::SaveBarricade(this,Id,Transform,200.f))return;
 ADeadlandsBarricade* Barricade=GetWorld()->SpawnActorDeferred<ADeadlandsBarricade>(
  ADeadlandsBarricade::StaticClass(),Transform,this,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
 if(!Barricade){FDeadlandsWorldPersistence::RemoveBarricade(this,Id);return;}
 if(!FDeadlandsWorldPersistence::SaveInventory(this,GetInventorySaveKey(),Updated))
 {
  Barricade->Destroy();FDeadlandsWorldPersistence::RemoveBarricade(this,Id);return;
 }
 Barricade->InitializeSaved(Id,200.f);
 Barricade->FinishSpawning(Transform);
 Items=MoveTemp(Updated);NotifyItemsChanged();
}
void ADeadlandsCharacter::DismantleBarricade()
{
 if(IsLocallyControlled())ServerDismantleBarricade();
}
void ADeadlandsCharacter::ServerDismantleBarricade_Implementation()
{
 if(bIsDead||!GetWorld())return;
 const FVector Eye=GetPawnViewLocation();
 FHitResult Hit;
 FCollisionQueryParams Params(SCENE_QUERY_STAT(DeadlandsDismantle),false,this);
 if(!GetWorld()->LineTraceSingleByChannel(Hit,Eye,Eye+GetBaseAimRotation().Vector()*260.f,ECC_Visibility,Params))return;
 if(ADeadlandsBarricade* Barricade=Cast<ADeadlandsBarricade>(Hit.GetActor()))
  Barricade->Dismantle(this);
}
