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

    /** Client entry point for inventory drag-and-drop reordering. */
    void RequestReorderItem(bool bContainer, ADeadlandsContainer* Container, int32 SourceIndex, int32 TargetIndex);

    /** Loads this player's item inventory on first use. Returns false if identity is not ready. */
    bool EnsureItemInventory();
    FString GetInventorySaveKey() const;
    TArray<FDeadlandsItemStack>& GetMutableItems() { return Items; }
    void NotifyItemsChanged();
    /** Called by server-side survival vitals when hunger or thirst reaches zero. */
    void ApplySurvivalDamage(float Amount);
    /** Server-authoritative damage shared by survival and melee combat. */
    void ApplyCombatDamage(float Amount);

    UFUNCTION(BlueprintPure, Category="Deadlands|Inventory")
    TArray<FDeadlandsItemStack> GetItems() const { return Items; }

    UFUNCTION(BlueprintCallable, Category="Deadlands|Inventory")
    void RequestItemTransfer(ADeadlandsContainer* Container, FName ItemId, bool bTake, int32 Quantity);
    void RequestUseContainerItem(ADeadlandsContainer* Container, FName ItemId);

    UFUNCTION(BlueprintCallable, Category="Deadlands|Survival") void RequestEquipBackpack();
    UFUNCTION(BlueprintCallable, Category="Deadlands|Survival") void RequestCraft(FName Recipe);
    UFUNCTION(BlueprintCallable, Category="Deadlands|Survival") void RequestRepairTools();
    UFUNCTION(BlueprintPure, Category="Deadlands|Survival") bool HasEquippedBackpack() const { return bBackpackEquipped; }
    UFUNCTION(BlueprintPure, Category="Deadlands|Survival") float GetBleeding() const { return Bleeding; }
    UFUNCTION(BlueprintPure, Category="Deadlands|Survival") float GetInfection() const { return Infection; }
    UFUNCTION(BlueprintPure, Category="Deadlands|Survival") int32 GetToolDurability() const { return ToolCondition; }
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
    int32 GetSupplyCapacity() const { return SupplyCapacity + (bBackpackEquipped ? 20 : 0); }

protected:
    UPROPERTY(ReplicatedUsing=OnRep_Items, VisibleAnywhere, BlueprintReadOnly, Category="Deadlands|Inventory")
    TArray<FDeadlandsItemStack> Items;

    bool bItemsLoaded = false;
    UPROPERTY(Replicated) bool bBackpackEquipped = false;
    UPROPERTY(Replicated) float Bleeding = 0.f;
    UPROPERTY(Replicated) float Infection = 0.f;
    UPROPERTY(Replicated) int32 ToolCondition = 100;
    float ConditionSaveClock = 0.f;
    bool SaveSurvivalAndItems(const TArray<FDeadlandsItemStack>& NewItems, bool NewBackpack, float NewBleeding, float NewInfection, int32 NewCondition);
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

    UPROPERTY(EditDefaultsOnly, Category="Deadlands|Combat")
    bool bAllowFriendlyFire = true; // Prototype setting: enable PvP testing; disable later for co-op.
    UPROPERTY(EditDefaultsOnly, Category="Deadlands|Combat")
    float MeleeDamage = 20.f;
    UPROPERTY(EditDefaultsOnly, Category="Deadlands|Combat")
    float MeleeRange = 180.f;
    UPROPERTY(EditDefaultsOnly, Category="Deadlands|Combat")
    float MeleeCooldown = 0.65f;
    float LastMeleeTime = -100.f;
    FString CombatFeedback;
    float CombatFeedbackUntil = 0.f;
    void TryMeleeAttack();
    void TryHeavyMeleeAttack();
    void ExecuteMeleeAttack(bool bHeavy);
    UFUNCTION(Server, Reliable) void ServerHeavyMeleeAttack();
    UFUNCTION(Server, Reliable) void ServerMeleeAttack();
    UFUNCTION(Client, Reliable) void ClientCombatFeedback(bool bHit);
    UFUNCTION(Client, Reliable) void ClientDetailedCombatFeedback(const FString& Message);

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
    UFUNCTION(Server, Reliable) void ServerUseContainerItem(ADeadlandsContainer* Container, FName ItemId);
    UFUNCTION(Server, Reliable) void ServerEquipBackpack();
    UFUNCTION(Server, Reliable) void ServerCraft(FName Recipe);
    UFUNCTION(Server, Reliable) void ServerRepairTools();
    UFUNCTION(Server, Reliable) void ServerDropItem(FName ItemId);
    void ShowInventory(ADeadlandsContainer* Container);

    UFUNCTION(Server, Reliable) void ServerReorderItem(bool bContainer, ADeadlandsContainer* Container, int32 SourceIndex, int32 TargetIndex);
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

