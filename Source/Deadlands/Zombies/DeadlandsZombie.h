#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interaction/DeadlandsInteractable.h"
#include "DeadlandsZombie.generated.h"

class UStaticMeshComponent;
class ADeadlandsCharacter;
class ADeadlandsContainer;

UENUM(BlueprintType)
enum class EDeadlandsZombieState : uint8
{
    Idle, Wandering, Suspicious, Chasing, Searching, Attacking, Returning, Dead
};

/** Simple server-driven zombie prototype. Place BP child actors in the level. */
UCLASS()
class DEADLANDS_API ADeadlandsZombie : public ACharacter, public IDeadlandsInteractable
{
    GENERATED_BODY()
public:
    ADeadlandsZombie();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    void ReceiveMeleeDamage(float Amount, bool bHeadshot = false, bool bHeavy = false);
    virtual bool CanInteract_Implementation(ACharacter* InteractingCharacter) const override;
    virtual void Interact_Implementation(ACharacter* InteractingCharacter) override;
    ADeadlandsContainer* GetCorpseLoot() const { return CorpseLoot; }
    void HearCombatNoise(const FVector& NoiseLocation);
    void AlertFromZombie(const FVector& AlertLocation);

    UFUNCTION(BlueprintPure, Category="Deadlands|Zombie")
    float GetZombieHealth() const { return Health; }

    UFUNCTION(BlueprintPure, Category="Deadlands|Zombie")
    EDeadlandsZombieState GetZombieState() const { return ZombieState; }

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Deadlands|Zombie")
    TObjectPtr<UStaticMeshComponent> PlaceholderBody;
    UPROPERTY(VisibleAnywhere, Category="Deadlands|Zombie Visual") TObjectPtr<UStaticMeshComponent> HeadVisual;
    UPROPERTY(VisibleAnywhere, Category="Deadlands|Zombie Visual") TObjectPtr<UStaticMeshComponent> LeftArmVisual;
    UPROPERTY(VisibleAnywhere, Category="Deadlands|Zombie Visual") TObjectPtr<UStaticMeshComponent> RightArmVisual;
    UPROPERTY(VisibleAnywhere, Category="Deadlands|Zombie Visual") TObjectPtr<UStaticMeshComponent> LeftLegVisual;
    UPROPERTY(VisibleAnywhere, Category="Deadlands|Zombie Visual") TObjectPtr<UStaticMeshComponent> RightLegVisual;
    UPROPERTY(VisibleAnywhere, Category="Deadlands|Zombie Visual") TObjectPtr<UStaticMeshComponent> FaceVisual;
    void UpdateZombieVisual(float DeltaSeconds);
    UPROPERTY(ReplicatedUsing=OnRep_HitReaction) uint8 HitReactionSerial = 0;
    UFUNCTION() void OnRep_HitReaction();
    float VisualStaggerRemaining = 0.f;
    float LastHitDirection = 1.f;
    float VisualTime = 0.f;
    float DeathBlend = 0.f;

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

    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="Deadlands|Zombie")
    EDeadlandsZombieState ZombieState = EDeadlandsZombieState::Idle;

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
    float NextWanderTime = 0.f;
    float SearchEndTime = 0.f;
    FVector HomeLocation = FVector::ZeroVector;
    FVector WanderDestination = FVector::ZeroVector;
    FVector SearchDestination = FVector::ZeroVector;
    bool bSearching = false;
    bool bHasWanderDestination = false;
    void SetZombieState(EDeadlandsZombieState NewState);
    void NavigateTo(const FVector& Destination, float AcceptanceRadius, float Now);
    void UpdatePassiveBehavior(float Now);
    float NextAttackTime = 0.f;
    float AttackWindupEnd = 0.f;
    float StaggerEndTime = 0.f;
    bool bWindingUp = false;
    TWeakObjectPtr<ADeadlandsCharacter> WindupTarget;
    float NextAlertTime = 0.f;
    float CorpseDestroyDelay = 45.f;
    UPROPERTY(Replicated) TObjectPtr<ADeadlandsContainer> CorpseLoot;
    bool bLootBagRevealed = false;
    void CreateCorpseLoot();
    float StaggerDuration = 0.65f;
    float AttackWindupSeconds = 0.55f;
    float KnockbackStrength = 350.f;
    bool bFastVariant = false;
    void BroadcastAlert(const FVector& Location);
    bool CanSeePlayer(const ADeadlandsCharacter* Player) const;
    FVector ChooseSteering(const FVector& Desired) const;
    void ApplyDeathVisual();
};
