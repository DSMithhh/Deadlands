#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/DeadlandsInteractable.h"
#include "Inventory/DeadlandsItemStack.h"
#include "DeadlandsContainer.generated.h"

UENUM(BlueprintType)
enum class EDeadlandsLootProfile : uint8
{
    House, Store, Garage, Medical
};

class UStaticMeshComponent;
class ADeadlandsCharacter;

/** Persistent container with a finite supply count. */
UCLASS(Blueprintable)
class DEADLANDS_API ADeadlandsContainer : public AActor, public IDeadlandsInteractable
{
    GENERATED_BODY()

public:
    ADeadlandsContainer();
    /** Configure a runtime-spawned container before BeginPlay. */
    void ConfigureLoot(FName StableId, EDeadlandsLootProfile Profile);
    void ConfigureCorpseLoot(FName StableId, const TArray<FDeadlandsItemStack>& Loot, bool bAsBag);
    void RevealCorpseBag();
    bool IsCorpseBag() const { return bCorpseBag; }
    bool HasLootRemaining() const { return FDeadlandsItems::Total(Items) > 0; }
    bool ReorderSlots(int32 Source, int32 Destination);
    bool TransferItemsToCharacter(ADeadlandsCharacter* Character, FName ItemId, bool bTake, int32 Quantity);
    UFUNCTION(BlueprintPure, Category="Deadlands|Loot")
    TArray<FDeadlandsItemStack> GetItems() const { return Items; }

    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual bool CanInteract_Implementation(ACharacter* InteractingCharacter) const override;
    virtual void Interact_Implementation(ACharacter* InteractingCharacter) override;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    TObjectPtr<UStaticMeshComponent> ContainerMesh;

    UPROPERTY(EditInstanceOnly, Category="Deadlands|Persistence")
    FName PersistenceId = NAME_None;

    UPROPERTY(ReplicatedUsing=OnRep_Open, VisibleAnywhere, BlueprintReadOnly, Category="Deadlands|Container")
    bool bOpen = false;

    /** Supplies assigned once when no save exists for this container. */
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Deadlands|Loot", meta=(ClampMin="0"))
    int32 InitialSupplies = 5;

    /** Pack 24: generate typed loot once when no inventory save exists. */
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Deadlands|Loot")
    bool bGenerateLoot = true;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Deadlands|Loot")
    EDeadlandsLootProfile LootProfile = EDeadlandsLootProfile::House;

    /** New item inventory, independent of legacy generic supplies. */
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Deadlands|Loot")
    TArray<FDeadlandsItemStack> InitialItems;

    UPROPERTY(ReplicatedUsing=OnRep_Items, VisibleAnywhere, BlueprintReadOnly, Category="Deadlands|Loot")
    TArray<FDeadlandsItemStack> Items;

    UFUNCTION()
    void OnRep_Items();

    UFUNCTION(BlueprintImplementableEvent, Category="Deadlands|Loot")
    void OnContainerItemsChanged();


    UPROPERTY(ReplicatedUsing=OnRep_Supplies, VisibleAnywhere, BlueprintReadOnly, Category="Deadlands|Loot")
    int32 RemainingSupplies = 0;

    UFUNCTION()
    void OnRep_Open();

    UFUNCTION()
    void OnRep_Supplies();

    UFUNCTION(BlueprintImplementableEvent, Category="Deadlands|Loot")
    void OnRemainingSuppliesChanged(int32 NewAmount);

    UFUNCTION(BlueprintImplementableEvent, Category="Deadlands|Container")
    void OnContainerStateChanged(bool bIsOpen);

private:
    UPROPERTY(ReplicatedUsing=OnRep_BagVisibility) bool bBagVisible = true;
    UPROPERTY(Replicated) bool bCorpseBag = false;
    UFUNCTION() void OnRep_BagVisibility();
    void ApplyBagAppearance();
    FString GetPersistenceKey() const;
    void ApplyState();
    TArray<FDeadlandsItemStack> GenerateInitialLoot() const;
};
