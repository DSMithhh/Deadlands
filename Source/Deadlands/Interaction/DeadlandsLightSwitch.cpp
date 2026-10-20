#include "Interaction/DeadlandsLightSwitch.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Net/UnrealNetwork.h"
#include "Deadlands.h"

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
    UE_LOG(LogDeadlands, Log, TEXT("Server switch %s state: %s"), *GetName(), bIsOn ? TEXT("ON") : TEXT("OFF"));
    ApplyLightState();
    ForceNetUpdate();
}

void ADeadlandsLightSwitch::OnRep_IsOn()
{
    UE_LOG(LogDeadlands, Log, TEXT("Client received switch %s state: %s"), *GetName(), bIsOn ? TEXT("ON") : TEXT("OFF"));
    ApplyLightState();
}

void ADeadlandsLightSwitch::ApplyLightState()
{
    Light->SetVisibility(bIsOn);
    OnSwitchStateChanged(bIsOn);
}
