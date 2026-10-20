#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/DeadlandsInteractable.h"
#include "Inventory/DeadlandsItemStack.h"
#include "DeadlandsContainer.generated.h"

class UStaticMeshComponent;
class ADeadlandsCharacter;

/** Persistent container with a finite supply count. */
UCLASS(Blueprintable)
class DEADLANDS_API ADeadlandsContainer : public AActor, public IDeadlandsInteractable
{
    GENERATED_BODY()

public:
    ADeadlandsContainer();
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
    FString GetPersistenceKey() const;
    void ApplyState();
};
