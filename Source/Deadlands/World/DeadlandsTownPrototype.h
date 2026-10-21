#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DeadlandsTownPrototype.generated.h"

class UStaticMeshComponent;
class USceneComponent;

/** Networked, asset-free greybox neighborhood. Replace with a saved map later. */
UCLASS()
class DEADLANDS_API ADeadlandsTownPrototype : public AActor
{
    GENERATED_BODY()
public:
    ADeadlandsTownPrototype();
    virtual void BeginPlay() override;
private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> SceneRoot;
    void AddBlock(const FString& Name, const FVector& Center, const FVector& Size, const FRotator& Rotation = FRotator::ZeroRotator);
    void AddBuilding(const FString& Name, const FVector& Center, float Width, float Depth, bool bShop);
};
