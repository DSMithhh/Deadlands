#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/DeadlandsInteractable.h"
#include "DeadlandsBarricade.generated.h"
class UStaticMeshComponent;
UCLASS()
class DEADLANDS_API ADeadlandsBarricade : public AActor, public IDeadlandsInteractable
{
 GENERATED_BODY()
public:
 ADeadlandsBarricade();
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Fortification") TObjectPtr<UStaticMeshComponent> Mesh;
 UPROPERTY(ReplicatedUsing=OnRep_Health,BlueprintReadOnly,Category="Fortification") float Health=200.f;
 UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="Fortification") float MaxHealth=200.f;
 UPROPERTY(Replicated,BlueprintReadOnly,Category="Fortification") FString PersistentId;
 void InitializeSaved(const FString& Id,float SavedHealth);
 bool Repair(class ADeadlandsCharacter* Character);
 bool Dismantle(class ADeadlandsCharacter* Character);
 virtual float TakeDamage(float DamageAmount,struct FDamageEvent const& DamageEvent,class AController* EventInstigator,AActor* DamageCauser) override;
 virtual bool CanInteract_Implementation(ACharacter* Character) const override;
 virtual void Interact_Implementation(ACharacter* Character) override;
 virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
protected:
 virtual void BeginPlay() override;
 UFUNCTION() void OnRep_Health();
 void RefreshVisual();
};
