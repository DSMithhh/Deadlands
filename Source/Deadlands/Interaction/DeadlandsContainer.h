#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/DeadlandsInteractable.h"
#include "DeadlandsContainer.generated.h"

class UStaticMeshComponent;

/** Replicated container shell; inventory/finite loot comes in a later pack. */
UCLASS(Blueprintable)
class DEADLANDS_API ADeadlandsContainer : public AActor, public IDeadlandsInteractable
{
    GENERATED_BODY()

public:
    ADeadlandsContainer();
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

    UFUNCTION()
    void OnRep_Open();

    UFUNCTION(BlueprintImplementableEvent, Category="Deadlands|Container")
    void OnContainerStateChanged(bool bIsOpen);

private:
    FString GetPersistenceKey() const;
    void ApplyState();
};
