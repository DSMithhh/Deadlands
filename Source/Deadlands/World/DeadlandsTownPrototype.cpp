#include "World/DeadlandsTownPrototype.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ADeadlandsTownPrototype::ADeadlandsTownPrototype()
{
    bReplicates = true;
    SetReplicateMovement(false);
    bNetLoadOnClient = true;
    PrimaryActorTick.bCanEverTick = false;
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("TownRoot"));
    SetRootComponent(SceneRoot);

    // 100 Unreal units = one metre. Everything uses engine-supplied cubes.
    // Town is intentionally a compact greybox, not a finished art environment.
    AddBlock(TEXT("Ground"), FVector(0, 0, -35), FVector(6500, 5200, 60));
    AddBlock(TEXT("MainRoad"), FVector(0, 0, 1), FVector(6100, 620, 8));
    AddBlock(TEXT("CrossRoad"), FVector(200, 0, 3), FVector(620, 4750, 8));
    AddBlock(TEXT("NorthSidewalk"), FVector(0, 405, 9), FVector(6000, 170, 18));
    AddBlock(TEXT("SouthSidewalk"), FVector(0, -405, 9), FVector(6000, 170, 18));
    for (int32 I = -5; I <= 5; ++I)
    {
        if (I == 0) continue;
        AddBlock(FString::Printf(TEXT("RoadStripe_%d"), I),
            FVector(I * 490.f, 0, 8), FVector(205, 14, 3));
    }

    // Four enterable structures: doors are intentionally left as wall gaps.
    AddBuilding(TEXT("HouseWest"), FVector(-1950, 1100, 0), 920, 780, false);
    AddBuilding(TEXT("HouseEast"), FVector(1550, 1150, 0), 950, 850, false);
    AddBuilding(TEXT("Market"), FVector(-1500, -1220, 0), 1200, 900, true);
    AddBuilding(TEXT("GasStation"), FVector(1400, -1280, 0), 1150, 850, true);

    // Gas station canopy and two pump islands.
    AddBlock(TEXT("Canopy"), FVector(1400, -420, 410), FVector(1550, 700, 35));
    for (int32 Side : {-1, 1})
    {
        AddBlock(FString::Printf(TEXT("CanopyPost_%d"), Side),
            FVector(1400 + Side * 620.f, -420, 210), FVector(35, 35, 400));
        AddBlock(FString::Printf(TEXT("FuelPump_%d"), Side),
            FVector(1400 + Side * 340.f, -400, 75), FVector(75, 65, 150));
    }

    // Interior landmarks and simple furniture, with open walkable floor space.
    AddBlock(TEXT("MarketCounter"), FVector(-1500, -1160, 65), FVector(500, 85, 130));
    AddBlock(TEXT("MarketShelfA"), FVector(-1770, -1440, 90), FVector(90, 240, 180));
    AddBlock(TEXT("MarketShelfB"), FVector(-1220, -1440, 90), FVector(90, 240, 180));
    AddBlock(TEXT("HouseTable"), FVector(-1950, 1120, 60), FVector(240, 160, 115));
    AddBlock(TEXT("HouseBed"), FVector(1540, 1350, 45), FVector(280, 190, 85));

    // Simple perimeter silhouettes, placed away from the main walkable streets.
    for (int32 I = 0; I < 14; ++I)
    {
        const float X = -2900.f + I * 440.f;
        const float Y = (I % 2 == 0) ? 2320.f : -2320.f;
        AddBlock(FString::Printf(TEXT("TreeTrunk_%d"), I),
            FVector(X, Y, 165), FVector(45, 45, 330));
        AddBlock(FString::Printf(TEXT("TreeCrown_%d"), I),
            FVector(X, Y, 365), FVector(240, 240, 230));
    }
}

void ADeadlandsTownPrototype::AddBlock(const FString& Name, const FVector& Center,
    const FVector& Size, const FRotator& Rotation)
{
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(*Name);
    Part->SetupAttachment(SceneRoot);
    if (Cube.Succeeded()) Part->SetStaticMesh(Cube.Object);
    Part->SetRelativeLocation(Center);
    Part->SetRelativeRotation(Rotation);
    Part->SetRelativeScale3D(Size / 100.f);
    Part->SetCollisionProfileName(TEXT("BlockAll"));
    Part->SetMobility(EComponentMobility::Static);
}

void ADeadlandsTownPrototype::AddBuilding(const FString& Name, const FVector& Center,
    float Width, float Depth, bool bShop)
{
    const float Wall = 28.f;
    const float Height = bShop ? 340.f : 300.f;
    const float DoorWidth = 180.f;
    const float FrontY = Center.Y < 0.f ? Center.Y + Depth * 0.5f : Center.Y - Depth * 0.5f;
    const float BackY = Center.Y < 0.f ? Center.Y - Depth * 0.5f : Center.Y + Depth * 0.5f;
    const float HalfSegment = (Width - DoorWidth) * 0.25f;
    AddBlock(Name + TEXT("_Floor"), FVector(Center.X, Center.Y, 8), FVector(Width, Depth, 16));
    AddBlock(Name + TEXT("_Back"), FVector(Center.X, BackY, Height * 0.5f),
        FVector(Width, Wall, Height));
    AddBlock(Name + TEXT("_Left"), FVector(Center.X - Width * 0.5f, Center.Y, Height * 0.5f),
        FVector(Wall, Depth, Height));
    AddBlock(Name + TEXT("_Right"), FVector(Center.X + Width * 0.5f, Center.Y, Height * 0.5f),
        FVector(Wall, Depth, Height));
    AddBlock(Name + TEXT("_FrontLeft"),
        FVector(Center.X - (DoorWidth * 0.5f + HalfSegment), FrontY, Height * 0.5f),
        FVector(HalfSegment * 2.f, Wall, Height));
    AddBlock(Name + TEXT("_FrontRight"),
        FVector(Center.X + (DoorWidth * 0.5f + HalfSegment), FrontY, Height * 0.5f),
        FVector(HalfSegment * 2.f, Wall, Height));
    AddBlock(Name + TEXT("_DoorHeader"), FVector(Center.X, FrontY, Height - 35.f),
        FVector(DoorWidth, Wall, 70.f));
    AddBlock(Name + TEXT("_Roof"), FVector(Center.X, Center.Y, Height + 16.f),
        FVector(Width + 70.f, Depth + 70.f, 32.f));
}
