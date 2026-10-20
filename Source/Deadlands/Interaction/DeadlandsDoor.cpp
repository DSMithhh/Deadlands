#include "Interaction/DeadlandsDoor.h"
#include "Components/SceneComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Persistence/DeadlandsWorldPersistence.h"
#include "Net/UnrealNetwork.h"

ADeadlandsDoor::ADeadlandsDoor()
{
    bReplicates = true;
    DoorRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DoorRoot"));
    RootComponent = DoorRoot;
    DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
    DoorMesh->SetupAttachment(DoorRoot);
    DoorMesh->SetCollisionProfileName(TEXT("BlockAll"));
    // The stationary interaction volume handles visibility traces, so a rotated
    // door mesh cannot intercept the player's interaction ray.
    DoorMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);

    InteractionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionVolume"));
    InteractionVolume->SetupAttachment(DoorRoot);
    InteractionVolume->SetBoxExtent(FVector(35.f, 75.f, 110.f));
    InteractionVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    InteractionVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
    InteractionVolume->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    InteractionVolume->SetHiddenInGame(true);
    // Offset mesh away from hinge so rotating the component swings the door.
    DoorMesh->SetRelativeLocation(FVector(0.f, 50.f, 0.f));
}

void ADeadlandsDoor::BeginPlay()
{
    Super::BeginPlay();
    if (HasAuthority())
    {
        if (!GetIsReplicated()) SetReplicates(true);
        FDeadlandsWorldPersistence::LoadActorFlag(this, GetPersistenceKey(), TEXT("Open"), bOpen);
        FDeadlandsWorldPersistence::LoadActorFlag(this, GetPersistenceKey(), TEXT("Locked"), bLocked);
    }
    ApplyState();
}

void ADeadlandsDoor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ADeadlandsDoor, bOpen);
    DOREPLIFETIME(ADeadlandsDoor, bLocked);
}

bool ADeadlandsDoor::CanInteract_Implementation(ACharacter* InteractingCharacter) const
{
    return IsValid(InteractingCharacter) && !bLocked;
}

void ADeadlandsDoor::Interact_Implementation(ACharacter* InteractingCharacter)
{
    if (!HasAuthority() || !CanInteract_Implementation(InteractingCharacter)) return;
    bOpen = !bOpen;
    ApplyState();
    FDeadlandsWorldPersistence::SaveActorFlag(this, GetPersistenceKey(), TEXT("Open"), bOpen);
    ForceNetUpdate();
}

void ADeadlandsDoor::SetLocked(bool bNewLocked)
{
    if (!HasAuthority()) return;
    bLocked = bNewLocked;
    FDeadlandsWorldPersistence::SaveActorFlag(this, GetPersistenceKey(), TEXT("Locked"), bLocked);
    ApplyState();
    ForceNetUpdate();
}

void ADeadlandsDoor::OnRep_Open() { ApplyState(); }
void ADeadlandsDoor::OnRep_Locked() { ApplyState(); }

FString ADeadlandsDoor::GetPersistenceKey() const
{
    return PersistenceId.IsNone() ? GetName() : PersistenceId.ToString();
}

void ADeadlandsDoor::ApplyState()
{
    DoorMesh->SetRelativeRotation(FRotator(0.f, bOpen ? OpenAngle : 0.f, 0.f));
    OnDoorStateChanged(bOpen, bLocked);
}
