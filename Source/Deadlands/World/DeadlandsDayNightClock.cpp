#include "World/DeadlandsDayNightClock.h"
#include "Engine/DirectionalLight.h"
#include "Components/SceneComponent.h"
#include "Net/UnrealNetwork.h"
ADeadlandsDayNightClock::ADeadlandsDayNightClock()
{
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(SceneRoot);
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetNetUpdateFrequency(2.f);
    bAlwaysRelevant = true;
}
void ADeadlandsDayNightClock::BeginPlay()
{
    Super::BeginPlay();
}
void ADeadlandsDayNightClock::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (HasAuthority()) Hour = FMath::Fmod(Hour + DeltaSeconds * 24.f / FMath::Max(RealSecondsPerDay, 1.f), 24.f);
    if (Sun)
    {
        // Build a deterministic sun orientation from time, not from the light's
        // current Euler angles. The fixed yaw/roll avoids a pole flip at noon.
        const float SolarPitchDegrees = 90.f - Hour * 15.f;
        const FQuat SolarRotation = FRotator(SolarPitchDegrees, 0.f, 0.f).Quaternion();
        Sun->SetActorRotation(SolarRotation);
    }
}
void ADeadlandsDayNightClock::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ADeadlandsDayNightClock, Hour);
}
