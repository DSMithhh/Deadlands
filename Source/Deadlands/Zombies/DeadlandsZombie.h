#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "DeadlandsZombie.generated.h"

class UStaticMeshComponent;
class ADeadlandsCharacter;

/** Simple server-driven zombie prototype. Place BP child actors in the level. */
UCLASS()
class DEADLANDS_API ADeadlandsZombie : public ACharacter
{
    GENERATED_BODY()
public:
    ADeadlandsZombie();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    void ReceiveMeleeDamage(float Amount);

    UFUNCTION(BlueprintPure, Category="Deadlands|Zombie")
    float GetZombieHealth() const { return Health; }

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Deadlands|Zombie")
    TObjectPtr<UStaticMeshComponent> PlaceholderBody;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Deadlands|Zombie")
    float DetectionRange = 1200.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Deadlands|Zombie")
    float AttackRange = 140.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Deadlands|Zombie")
    float AttackDamage = 10.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Deadlands|Zombie")
    float AttackCooldown = 1.5f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Deadlands|Zombie")
    float WalkSpeed = 180.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Deadlands|Zombie")
    float MaxHealth = 60.f;

    UPROPERTY(ReplicatedUsing=OnRep_Health, VisibleAnywhere, BlueprintReadOnly, Category="Deadlands|Zombie")
    float Health = 60.f;
    UPROPERTY(ReplicatedUsing=OnRep_Dead, VisibleAnywhere, BlueprintReadOnly, Category="Deadlands|Zombie")
    bool bDead = false;

    UFUNCTION() void OnRep_Health();
    UFUNCTION() void OnRep_Dead();

private:
    UPROPERTY(EditAnywhere, Category="Deadlands|Zombie AI")
    float HearingRange = 650.f;
    UPROPERTY(EditAnywhere, Category="Deadlands|Zombie AI")
    float SprintHearingRange = 1600.f;
    UPROPERTY(EditAnywhere, Category="Deadlands|Zombie AI")
    float MemorySeconds = 7.f;
    UPROPERTY(EditAnywhere, Category="Deadlands|Zombie AI")
    float FieldOfViewDegrees = 150.f;
    TWeakObjectPtr<ADeadlandsCharacter> CurrentTarget;
    FVector LastKnownLocation = FVector::ZeroVector;
    float LastSensedTime = -1000.f;
    float NextDecisionTime = 0.f;
    float NextAttackTime = 0.f;
    bool CanSeePlayer(const ADeadlandsCharacter* Player) const;
    FVector ChooseSteering(const FVector& Desired) const;
    void ApplyDeathVisual();
};
