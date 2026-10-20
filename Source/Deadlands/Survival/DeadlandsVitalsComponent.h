#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DeadlandsVitalsComponent.generated.h"

UCLASS(ClassGroup=(Deadlands), meta=(BlueprintSpawnableComponent))
class DEADLANDS_API UDeadlandsVitalsComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UDeadlandsVitalsComponent();
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    UFUNCTION(BlueprintPure, Category="Deadlands|Vitals") float GetStamina() const { return Stamina; }
    UFUNCTION(BlueprintPure, Category="Deadlands|Vitals") float GetHunger() const { return Hunger; }
    UFUNCTION(BlueprintPure, Category="Deadlands|Vitals") float GetThirst() const { return Thirst; }
    UFUNCTION(BlueprintPure, Category="Deadlands|Vitals") bool CanSprint() const { return Stamina > 1.f; }
    UFUNCTION(BlueprintCallable, Category="Deadlands|Vitals") void SetSprinting(bool bNewSprinting);
protected:
    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="Deadlands|Vitals") float Stamina = 100.f;
    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="Deadlands|Vitals") float Hunger = 100.f;
    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="Deadlands|Vitals") float Thirst = 100.f;
    UPROPERTY(EditDefaultsOnly, Category="Deadlands|Vitals") float StaminaDrainPerSecond = 18.f;
    UPROPERTY(EditDefaultsOnly, Category="Deadlands|Vitals") float StaminaRecoveryPerSecond = 12.f;
    UPROPERTY(EditDefaultsOnly, Category="Deadlands|Vitals") float HungerLossPerMinute = 0.5f;
    UPROPERTY(EditDefaultsOnly, Category="Deadlands|Vitals") float ThirstLossPerMinute = 1.f;
private:
    bool bSprinting = false;
};
