#include "Interaction/DeadlandsContainer.h"
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
    }
    ApplyState();
}

void ADeadlandsContainer::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ADeadlandsContainer, bOpen);
}

bool ADeadlandsContainer::CanInteract_Implementation(ACharacter* InteractingCharacter) const
{
    return IsValid(InteractingCharacter);
}

void ADeadlandsContainer::Interact_Implementation(ACharacter* InteractingCharacter)
{
    if (!HasAuthority() || !IsValid(InteractingCharacter)) return;
    bOpen = !bOpen;
    ApplyState();
    FDeadlandsWorldPersistence::SaveActorFlag(this, GetPersistenceKey(), TEXT("Open"), bOpen);
    ForceNetUpdate();
}

void ADeadlandsContainer::OnRep_Open() { ApplyState(); }

FString ADeadlandsContainer::GetPersistenceKey() const
{
    return PersistenceId.IsNone() ? GetName() : PersistenceId.ToString();
}

void ADeadlandsContainer::ApplyState()
{
    OnContainerStateChanged(bOpen);
}
