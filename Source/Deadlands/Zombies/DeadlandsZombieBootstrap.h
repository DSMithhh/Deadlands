#pragma once

#include "CoreMinimal.h"
#include "TimerManager.h"
#include "Subsystems/WorldSubsystem.h"
#include "DeadlandsZombieBootstrap.generated.h"

/** Automatically creates prototype zombies in gameplay worlds without Blueprint or level edits. */
UCLASS()
class DEADLANDS_API UDeadlandsZombieBootstrap : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
private:
    FTimerHandle SpawnTimer;
    void SpawnTestZombies();
};
