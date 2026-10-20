#include "Interaction/DeadlandsLightSwitch.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Net/UnrealNetwork.h"
#include "Deadlands.h"
#include "Persistence/DeadlandsWorldPersistence.h"

ADeadlandsLightSwitch::ADeadlandsLightSwitch()
{
    bReplicates = true;
    SetReplicateMovement(false);

    SwitchMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SwitchMesh"));
    RootComponent = SwitchMesh;
    SwitchMesh->SetCollisionProfileName(TEXT("BlockAll"));

    Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
    Light->SetupAttachment(RootComponent);
    Light->SetRelativeLocation(FVector(0.f, 0.f, 120.f));
    Light->SetIntensity(3000.f);
    Light->SetVisibility(bIsOn);
}

void ADeadlandsLightSwitch::BeginPlay()
{
    Super::BeginPlay();

    // A Blueprint asset may override the native constructor's replication default.
    // Enforce the authoritative world actor's replication before gameplay begins.
    if (HasAuthority() && !GetIsReplicated())
    {
        SetReplicates(true);
    }

    if (HasAuthority())
    {
        bool bSavedState = bIsOn;
        if (FDeadlandsWorldPersistence::LoadSwitchState(this, GetPersistenceKey(), bSavedState))
        {
            bIsOn = bSavedState;
            UE_LOG(LogDeadlands, Log, TEXT("Restored switch %s to %s"), *GetPersistenceKey(),
                bIsOn ? TEXT("ON") : TEXT("OFF"));
        }
    }

    ApplyLightState();
    UE_LOG(LogDeadlands, Warning,
        TEXT("Switch BEGIN %s | Authority=%d | Replicates=%d | NetMode=%d | State=%d"),
        *GetPathName(), HasAuthority() ? 1 : 0, GetIsReplicated() ? 1 : 0,
        static_cast<int32>(GetNetMode()), bIsOn ? 1 : 0);
}

void ADeadlandsLightSwitch::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ADeadlandsLightSwitch, bIsOn);
}

bool ADeadlandsLightSwitch::CanInteract_Implementation(ACharacter* InteractingCharacter) const
{
    return IsValid(InteractingCharacter);
}

void ADeadlandsLightSwitch::Interact_Implementation(ACharacter* InteractingCharacter)
{
    if (!HasAuthority() || !IsValid(InteractingCharacter))
    {
        return;
    }

    bIsOn = !bIsOn;
    UE_LOG(LogDeadlands, Log, TEXT("Switch SERVER %s state: %s | Replicates=%d"), *GetPathName(), bIsOn ? TEXT("ON") : TEXT("OFF"), GetIsReplicated() ? 1 : 0);
    ApplyLightState();
    if (!FDeadlandsWorldPersistence::SaveSwitchState(this, GetPersistenceKey(), bIsOn))
    {
        UE_LOG(LogDeadlands, Warning, TEXT("Switch state not persisted: %s"), *GetPersistenceKey());
    }
    ForceNetUpdate();
}

void ADeadlandsLightSwitch::OnRep_IsOn()
{
    UE_LOG(LogDeadlands, Log, TEXT("Switch ONREP %s state: %s"), *GetPathName(), bIsOn ? TEXT("ON") : TEXT("OFF"));
    ApplyLightState();
}

void ADeadlandsLightSwitch::ApplyLightState()
{
    Light->SetVisibility(bIsOn);
    OnSwitchStateChanged(bIsOn);
}

FString ADeadlandsLightSwitch::GetPersistenceKey() const
{
    // A stable, per-instance ID is recommended when adding multiple switches.
    // The native actor name is sufficient for our initial level-placed test switch,
    // but may change if the actor is renamed or recreated.
    return PersistenceId.IsNone() ? GetName() : PersistenceId.ToString();
}
