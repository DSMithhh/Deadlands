#include "World/DeadlandsDayNightClock.h"
#include "Engine/DirectionalLight.h"
#include "Net/UnrealNetwork.h"
ADeadlandsDayNightClock::ADeadlandsDayNightClock()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetNetUpdateFrequency(2.f);
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
        const FRotator Existing = Sun->GetActorRotation();
        Sun->SetActorRotation(FRotator(Hour * 15.f - 90.f, Existing.Yaw, Existing.Roll));
    }
}
void ADeadlandsDayNightClock::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ADeadlandsDayNightClock, Hour);
}
