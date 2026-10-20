#include "Zombies/DeadlandsZombieBootstrap.h"
#include "Zombies/DeadlandsZombie.h"
#include "DeadlandsCharacter.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"

void UDeadlandsZombieBootstrap::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);
    if (!InWorld.IsGameWorld() || InWorld.GetNetMode() == NM_Client) return;
    // Give GameMode time to spawn player pawns (including the listen-server host).
    InWorld.GetTimerManager().SetTimer(SpawnTimer, this,
        &UDeadlandsZombieBootstrap::SpawnTestZombies, 3.f, false);
}

void UDeadlandsZombieBootstrap::SpawnTestZombies()
{
    UWorld* World = GetWorld();
    if (!World || World->GetNetMode() == NM_Client) return;

    ADeadlandsCharacter* Player = nullptr;
    for (TActorIterator<ADeadlandsCharacter> It(World); It; ++It)
    {
        if (IsValid(*It) && (*It)->GetController())
        {
            Player = *It;
            break;
        }
    }
    if (!Player)
    {
        // A delayed spawn or seamless travel may mean players are not ready yet.
        World->GetTimerManager().SetTimer(SpawnTimer, this,
            &UDeadlandsZombieBootstrap::SpawnTestZombies, 2.f, false);
        return;
    }

    // Skip automatic spawning if the designer already placed zombies in this level.
    for (TActorIterator<ADeadlandsZombie> It(World); It; ++It) return;

    constexpr FVector2D Offsets[] = {
        FVector2D(500.f, 0.f), FVector2D(-450.f, 200.f), FVector2D(100.f, 600.f)
    };
    for (const FVector2D& Offset : Offsets)
    {
        const FVector Location = Player->GetActorLocation() +
            Player->GetActorForwardVector() * Offset.X +
            Player->GetActorRightVector() * Offset.Y +
            FVector(0.f, 0.f, 10.f);
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
        World->SpawnActor<ADeadlandsZombie>(ADeadlandsZombie::StaticClass(),
            Location, FRotator::ZeroRotator, Params);
    }
}
