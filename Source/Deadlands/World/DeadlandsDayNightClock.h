#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DeadlandsDayNightClock.generated.h"
class ADirectionalLight;
class USceneComponent;
UCLASS(Blueprintable)
class DEADLANDS_API ADeadlandsDayNightClock : public AActor
{
    GENERATED_BODY()
public:
    ADeadlandsDayNightClock();
    virtual void Tick(float DeltaSeconds) override;
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    UFUNCTION(BlueprintPure, Category="Deadlands|Time") float GetHour() const { return Hour; }
protected:
    UPROPERTY(VisibleAnywhere, Category="Components")
    TObjectPtr<USceneComponent> SceneRoot;
    UPROPERTY(EditAnywhere, Replicated, BlueprintReadOnly, Category="Deadlands|Time", meta=(ClampMin="0", ClampMax="24")) float Hour = 12.f;
    UPROPERTY(EditAnywhere, Category="Deadlands|Time", meta=(ClampMin="1")) float RealSecondsPerDay = 1200.f;
    UPROPERTY(EditInstanceOnly, Category="Deadlands|Time") TObjectPtr<ADirectionalLight> Sun;
};
