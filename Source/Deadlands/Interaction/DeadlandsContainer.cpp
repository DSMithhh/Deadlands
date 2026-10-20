#include "Interaction/DeadlandsContainer.h"
#include "DeadlandsCharacter.h"
#include "Deadlands.h"
#include "Components/StaticMeshComponent.h"
#include "Persistence/DeadlandsWorldPersistence.h"
#include "Net/UnrealNetwork.h"

ADeadlandsContainer::ADeadlandsContainer()
{
    bReplicates = true;
    ContainerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ContainerMesh"));
    RootComponent = ContainerMesh;
    ContainerMesh->SetCollisionProfileName(TEXT("BlockAll"));
}

void ADeadlandsContainer::BeginPlay()
{
    Super::BeginPlay();
    if (HasAuthority())
    {
        if (!GetIsReplicated()) SetReplicates(true);
        FDeadlandsWorldPersistence::LoadActorFlag(this, GetPersistenceKey(), TEXT("Open"), bOpen);
        RemainingSupplies = FMath::Max(0, InitialSupplies);
        // A saved zero is meaningful: empty containers never refill.
        FDeadlandsWorldPersistence::LoadContainerSupplies(this, GetPersistenceKey(), RemainingSupplies);
    }
    ApplyState();
    OnRemainingSuppliesChanged(RemainingSupplies);
}

void ADeadlandsContainer::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ADeadlandsContainer, bOpen);
    DOREPLIFETIME(ADeadlandsContainer, RemainingSupplies);
}

bool ADeadlandsContainer::CanInteract_Implementation(ACharacter* InteractingCharacter) const
{
    return IsValid(InteractingCharacter);
}

void ADeadlandsContainer::Interact_Implementation(ACharacter* InteractingCharacter)
{
    if (!HasAuthority() || !IsValid(InteractingCharacter)) return;

    if (!bOpen)
    {
        bOpen = true;
        ApplyState();
        FDeadlandsWorldPersistence::SaveActorFlag(this, GetPersistenceKey(), TEXT("Open"), bOpen);
        ForceNetUpdate();
        return;
    }

    // Once opened, each F press transfers exactly one supply. When empty,
    // pressing F closes the container.
    if (RemainingSupplies <= 0)
    {
        bOpen = false;
        ApplyState();
        FDeadlandsWorldPersistence::SaveActorFlag(this, GetPersistenceKey(), TEXT("Open"), bOpen);
        ForceNetUpdate();
        return;
    }

    ADeadlandsCharacter* Survivor = Cast<ADeadlandsCharacter>(InteractingCharacter);
    if (!Survivor || Survivor->GetSupplies() >= Survivor->GetSupplyCapacity())
    {
        UE_LOG(LogDeadlands, Log, TEXT("Cannot loot %s: player inventory full or incompatible"), *GetNameSafe(this));
        return;
    }

    // Persist depletion before awarding the item, so a failed disk write
    // cannot generate additional supplies on the next server restart.
    const int32 NewRemaining = RemainingSupplies - 1;
    if (!FDeadlandsWorldPersistence::SaveContainerSupplies(this, GetPersistenceKey(), NewRemaining))
    {
        return;
    }
    if (!Survivor->AddSupplies(1))
    {
        UE_LOG(LogDeadlands, Error, TEXT("Supply transfer failed after save for %s"), *GetNameSafe(this));
        return;
    }
    RemainingSupplies = NewRemaining;
    OnRemainingSuppliesChanged(RemainingSupplies);
    UE_LOG(LogDeadlands, Log, TEXT("Looted %s: %d supplies left"), *GetNameSafe(this), RemainingSupplies);
    ForceNetUpdate();
}

void ADeadlandsContainer::OnRep_Open() { ApplyState(); }
void ADeadlandsContainer::OnRep_Supplies() { OnRemainingSuppliesChanged(RemainingSupplies); }

FString ADeadlandsContainer::GetPersistenceKey() const
{
    return PersistenceId.IsNone() ? GetName() : PersistenceId.ToString();
}

void ADeadlandsContainer::ApplyState()
{
    OnContainerStateChanged(bOpen);
}
