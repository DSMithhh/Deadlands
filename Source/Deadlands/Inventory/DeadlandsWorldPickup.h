#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/DeadlandsInteractable.h"
#include "DeadlandsWorldPickup.generated.h"
class USphereComponent;
class UStaticMeshComponent;
UCLASS()
class DEADLANDS_API ADeadlandsWorldPickup : public AActor, public IDeadlandsInteractable
{
    GENERATED_BODY()
public:
    ADeadlandsWorldPickup();
    void InitializePickup(FName NewItemId);
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual bool CanInteract_Implementation(ACharacter* Character) const override;
    virtual void Interact_Implementation(ACharacter* Character) override;
    UFUNCTION(BlueprintPure, Category="Deadlands|Pickup") FName GetItemId() const { return ItemId; }
protected:
    UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> Collision;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Mesh;
    UPROPERTY(Replicated, VisibleAnywhere, Category="Deadlands|Pickup") FName ItemId = NAME_None;
};
