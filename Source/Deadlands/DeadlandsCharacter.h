// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "TimerManager.h"
#include "Inventory/DeadlandsItemStack.h"
#include "DeadlandsCharacter.generated.h"

class UInputComponent;
class UDeadlandsInventoryWidget;
class ADeadlandsContainer;
class USkeletalMeshComponent;
class UCameraComponent;
class USpotLightComponent;
class UDeadlandsVitalsComponent;
class UDeadlandsInteractionPrompt;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  A basic first person character
 */
UCLASS(abstract)
class ADeadlandsCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Pawn mesh: first person view (arms; seen only by self) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* FirstPersonMesh;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCameraComponent;

protected:

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	class UInputAction* LookAction;

	/** F-key interaction action (assign IA_Interact in the character Blueprint). */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* InteractAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	class UInputAction* MouseLookAction;
	
public:
    ADeadlandsCharacter();

    /** Loads this player's item inventory on first use. Returns false if identity is not ready. */
    bool EnsureItemInventory();
    FString GetInventorySaveKey() const;
    TArray<FDeadlandsItemStack>& GetMutableItems() { return Items; }
    void NotifyItemsChanged();
    /** Called by server-side survival vitals when hunger or thirst reaches zero. */
    void ApplySurvivalDamage(float Amount);

    UFUNCTION(BlueprintPure, Category="Deadlands|Inventory")
    TArray<FDeadlandsItemStack> GetItems() const { return Items; }

    UFUNCTION(BlueprintCallable, Category="Deadlands|Inventory")
    void RequestItemTransfer(ADeadlandsContainer* Container, FName ItemId, bool bTake, int32 Quantity);

    void CloseInventory();
    UFUNCTION(BlueprintCallable, Category="Deadlands|Inventory")
    void RequestConsumeItem(FName ItemId);
    UFUNCTION(BlueprintCallable, Category="Deadlands|Inventory")
    void RequestDropItem(FName ItemId);
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    UFUNCTION(BlueprintPure, Category="Deadlands|Vitals")
    UDeadlandsVitalsComponent* GetVitals() const { return Vitals; }

    // Pack 05: independent sprint, local flashlight and replicated health prototypes.
    UFUNCTION(BlueprintPure, Category="Deadlands|Survival")
    float GetHealth() const { return Health; }

    UFUNCTION(BlueprintPure, Category="Deadlands|Survival")
    float GetMaxHealth() const { return MaxHealth; }

    UFUNCTION(BlueprintPure, Category="Deadlands|Survival")
    bool IsSprinting() const { return bIsSprinting; }

    UFUNCTION(BlueprintPure, Category="Deadlands|Survival")
    bool IsFlashlightOn() const { return bFlashlightOn; }


    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    /** Server-only transfer into this pawn's prototype inventory. */
    bool AddSupplies(int32 Amount);

    UFUNCTION(BlueprintPure, Category="Deadlands|Inventory")
    int32 GetSupplies() const { return Supplies; }

    UFUNCTION(BlueprintPure, Category="Deadlands|Inventory")
    int32 GetSupplyCapacity() const { return SupplyCapacity; }

protected:
    UPROPERTY(ReplicatedUsing=OnRep_Items, VisibleAnywhere, BlueprintReadOnly, Category="Deadlands|Inventory")
    TArray<FDeadlandsItemStack> Items;

    bool bItemsLoaded = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Deadlands|Vitals")
    TObjectPtr<UDeadlandsVitalsComponent> Vitals;
    UPROPERTY(Transient)
    TObjectPtr<UDeadlandsInteractionPrompt> InteractionPrompt;
    float InteractionPromptCountdown = 0.f;
    FVector StandingCameraRelativeLocation = FVector::ZeroVector;
    bool bCameraBaseCaptured = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Deadlands|Survival")
    TObjectPtr<USpotLightComponent> Flashlight;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Deadlands|Survival")
    float SurvivalWalkSpeed = 500.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Deadlands|Survival")
    float SurvivalSprintSpeed = 800.f;

    UPROPERTY(ReplicatedUsing=OnRep_Sprinting, VisibleAnywhere, BlueprintReadOnly, Category="Deadlands|Survival")
    bool bIsSprinting = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Deadlands|Survival")
    bool bFlashlightOn = false;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Deadlands|Survival")
    float MaxHealth = 100.f;

    UPROPERTY(ReplicatedUsing=OnRep_Health, VisibleAnywhere, BlueprintReadOnly, Category="Deadlands|Survival")
    float Health = 100.f;

    UPROPERTY(ReplicatedUsing=OnRep_Dead, VisibleAnywhere, BlueprintReadOnly, Category="Deadlands|Survival")
    bool bIsDead = false;
    UPROPERTY(EditDefaultsOnly, Category="Deadlands|Survival")
    float RespawnDelaySeconds = 5.f;
    FTransform InitialSpawnTransform;
    FTimerHandle RespawnTimer;
    void Die();
    void RespawnSurvivor();
    void ApplyDeathState();
    UFUNCTION() void OnRep_Dead();

    void BeginSprint();
    void EndSprint();
    void ToggleFlashlight();
    void DebugDamage();
    void DebugHeal();
    void ApplySprintSpeed();

    UFUNCTION(Server, Reliable)
    void ServerSetSprinting(bool bNewSprinting);

    UFUNCTION(Server, Reliable)
    void ServerDebugChangeHealth(float Delta);

    UFUNCTION()
    void OnRep_Sprinting();

    UFUNCTION()
    void OnRep_Health();

    UFUNCTION(BlueprintImplementableEvent, Category="Deadlands|Survival")
    void OnHealthChanged(float NewHealth, float NewMaxHealth);


    UPROPERTY(Transient)
    TObjectPtr<UDeadlandsInventoryWidget> InventoryWidget;

    UPROPERTY(Transient)
    TObjectPtr<ADeadlandsContainer> CurrentContainer;

    void ToggleInventory();
    void ToggleCrouch();
    UFUNCTION(Server, Reliable) void ServerConsumeItem(FName ItemId);
    UFUNCTION(Server, Reliable) void ServerDropItem(FName ItemId);
    void ShowInventory(ADeadlandsContainer* Container);

    UFUNCTION(Server, Reliable)
    void ServerTransferItems(ADeadlandsContainer* Container, FName ItemId, bool bTake, int32 Quantity);

    UFUNCTION()
    void OnRep_Items();

    UFUNCTION(BlueprintImplementableEvent, Category="Deadlands|Inventory")
    void OnItemsChanged();

    UPROPERTY(ReplicatedUsing=OnRep_Supplies, VisibleAnywhere, BlueprintReadOnly, Category="Deadlands|Inventory")
    int32 Supplies = 0;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Deadlands|Inventory", meta=(ClampMin="1"))
    int32 SupplyCapacity = 20;

    UFUNCTION()
    void OnRep_Supplies();

    UFUNCTION(BlueprintImplementableEvent, Category="Deadlands|Inventory")
    void OnSuppliesChanged(int32 NewAmount);

public:

protected:

	/** Find an interactable directly ahead and request a server interaction. */
	void TryInteract();

	UFUNCTION(Server, Reliable)
	void ServerTryInteract(AActor* RequestedTarget);

	/** Called from Input Actions for movement input */
	void MoveInput(const FInputActionValue& Value);

	/** Called from Input Actions for looking input */
	void LookInput(const FInputActionValue& Value);

	/** Handles aim inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoAim(float Yaw, float Pitch);

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles jump start inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump end inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

protected:

	/** Set up input action bindings */
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	

public:

	/** Returns the first person mesh **/
	USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; }

	/** Returns first person camera component **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

};

