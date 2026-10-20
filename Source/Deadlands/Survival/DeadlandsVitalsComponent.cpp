#include "Survival/DeadlandsVitalsComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Actor.h"
UDeadlandsVitalsComponent::UDeadlandsVitalsComponent()
{
    SetIsReplicatedByDefault(true);
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.1f;
}
void UDeadlandsVitalsComponent::BeginPlay()
{
    Super::BeginPlay();
}
void UDeadlandsVitalsComponent::SetSprinting(bool bNewSprinting)
{
    if (GetOwner() && GetOwner()->HasAuthority()) bSprinting = bNewSprinting;
}
void UDeadlandsVitalsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!GetOwner() || !GetOwner()->HasAuthority()) return;
    const float PreviousStamina = Stamina;
    Stamina = FMath::Clamp(Stamina + (bSprinting ? -StaminaDrainPerSecond : StaminaRecoveryPerSecond) * DeltaTime, 0.f, 100.f);
    Hunger = FMath::Clamp(Hunger - HungerLossPerMinute * DeltaTime / 60.f, 0.f, 100.f);
    Thirst = FMath::Clamp(Thirst - ThirstLossPerMinute * DeltaTime / 60.f, 0.f, 100.f);
    if (bSprinting && PreviousStamina > 0.f && Stamina <= 0.f)
    {
        bSprinting = false;
    }
}
void UDeadlandsVitalsComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UDeadlandsVitalsComponent, Stamina, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UDeadlandsVitalsComponent, Hunger, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UDeadlandsVitalsComponent, Thirst, COND_OwnerOnly);
}
