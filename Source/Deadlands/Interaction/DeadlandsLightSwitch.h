#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/DeadlandsInteractable.h"
#include "DeadlandsLightSwitch.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;

UCLASS(Blueprintable)
class DEADLANDS_API ADeadlandsLightSwitch : public AActor, public IDeadlandsInteractable
{
    GENERATED_BODY()

public:
    ADeadlandsLightSwitch();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual bool CanInteract_Implementation(ACharacter* InteractingCharacter) const override;
    virtual void Interact_Implementation(ACharacter* InteractingCharacter) override;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    TObjectPtr<UStaticMeshComponent> SwitchMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    TObjectPtr<UPointLightComponent> Light;

    /** Stable unique name for this level-placed switch; if unset, use the actor name. */
    UPROPERTY(EditInstanceOnly, Category="Deadlands|Persistence")
    FName PersistenceId = NAME_None;

    UPROPERTY(EditAnywhere, ReplicatedUsing=OnRep_IsOn, BlueprintReadOnly, Category="Deadlands|Switch")
    bool bIsOn = true;

    UFUNCTION()
    void OnRep_IsOn();

    UFUNCTION(BlueprintImplementableEvent, Category="Deadlands|Switch")
    void OnSwitchStateChanged(bool bNewIsOn);

private:
    FString GetPersistenceKey() const;
    void ApplyLightState();
};
