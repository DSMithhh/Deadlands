#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/DeadlandsInteractable.h"
#include "DeadlandsDoor.generated.h"

class UStaticMeshComponent;
class USceneComponent;
class UBoxComponent;

/** Replicated, persistent, interactable prototype door. */
UCLASS(Blueprintable)
class DEADLANDS_API ADeadlandsDoor : public AActor, public IDeadlandsInteractable
{
    GENERATED_BODY()

public:
    ADeadlandsDoor();
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual bool CanInteract_Implementation(ACharacter* InteractingCharacter) const override;
    virtual void Interact_Implementation(ACharacter* InteractingCharacter) override;

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Deadlands|Door")
    void SetLocked(bool bNewLocked);

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    TObjectPtr<USceneComponent> DoorRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    TObjectPtr<UStaticMeshComponent> DoorMesh;

    /** Fixed interaction volume near the hinge, available whether the door is open or shut. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    TObjectPtr<UBoxComponent> InteractionVolume;

    UPROPERTY(EditInstanceOnly, Category="Deadlands|Persistence")
    FName PersistenceId = NAME_None;

    UPROPERTY(EditAnywhere, ReplicatedUsing=OnRep_Open, BlueprintReadOnly, Category="Deadlands|Door")
    bool bOpen = false;

    UPROPERTY(EditAnywhere, ReplicatedUsing=OnRep_Locked, BlueprintReadOnly, Category="Deadlands|Door")
    bool bLocked = false;

    UPROPERTY(EditAnywhere, Category="Deadlands|Door")
    float OpenAngle = 90.f;

    UFUNCTION()
    void OnRep_Open();

    UFUNCTION()
    void OnRep_Locked();

    UFUNCTION(BlueprintImplementableEvent, Category="Deadlands|Door")
    void OnDoorStateChanged(bool bIsOpen, bool bIsLocked);

private:
    FString GetPersistenceKey() const;
    void ApplyState();
};
