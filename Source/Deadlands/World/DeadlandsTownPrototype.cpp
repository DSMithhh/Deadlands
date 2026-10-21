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

    // Pack 17: roadside edges, street furniture, and believable neighborhood silhouettes.
    // Keep all obstacles clear of the main travel lanes and front door openings.
    for (int32 I = -5; I <= 5; ++I)
    {
        const float X = I * 540.f;
        if (FMath::Abs(X - 200.f) < 500.f) continue; // Crossroad stays open.
        for (int32 Side : {-1, 1})
        {
            const float Y = Side * 502.f;
            AddBlock(FString::Printf(TEXT("Curb_%d_%d"), I, Side),
                FVector(X, Y, 16), FVector(480, 20, 30));
        }
    }

    // Secondary-road curbs: leave the main intersection open.
    for (int32 I : {-3, -2, 2, 3})
    {
        for (int32 Side : {-1, 1})
        {
            AddBlock(FString::Printf(TEXT("CrossCurb_%d_%d"), I, Side),
                FVector(200.f + Side * 415.f, I * 520.f, 16),
                FVector(22, 420, 30));
        }
    }

    // Wooden fence sections along rear yards, with gaps for traversal.
    for (int32 I = 0; I < 5; ++I)
    {
        const float X = -2750.f + I * 290.f;
        AddBlock(FString::Printf(TEXT("WestFencePost_%d"), I),
            FVector(X, 1790, 85), FVector(22, 22, 170));
        if (I < 4)
            AddBlock(FString::Printf(TEXT("WestFenceRail_%d"), I),
                FVector(X + 145.f, 1790, 85), FVector(270, 12, 16));
    }
    for (int32 I = 0; I < 5; ++I)
    {
        const float X = 1050.f + I * 290.f;
        AddBlock(FString::Printf(TEXT("EastFencePost_%d"), I),
            FVector(X, 1810, 85), FVector(22, 22, 170));
        if (I < 4)
            AddBlock(FString::Printf(TEXT("EastFenceRail_%d"), I),
                FVector(X + 145.f, 1810, 85), FVector(270, 12, 16));
    }

    // Power poles and suspended-cable proxies run outside the street.
    for (int32 I = 0; I < 5; ++I)
    {
        const float X = -2600.f + I * 1250.f;
        AddBlock(FString::Printf(TEXT("PowerPole_%d"), I),
            FVector(X, -620, 380), FVector(36, 36, 760));
        AddBlock(FString::Printf(TEXT("PowerCrossarm_%d"), I),
            FVector(X, -620, 675), FVector(28, 260, 22));
        if (I < 4)
            AddBlock(FString::Printf(TEXT("PowerLine_%d"), I),
                FVector(X + 625.f, -720, 690), FVector(1230, 6, 6));
    }

    // Abandoned cars: blocky silhouettes built from separate body, cabin and wheels.
    // Parked off the main road so players and zombies can still move through town.
    struct FCarLocation { float X; float Y; float Yaw; };
    const FCarLocation Cars[] = {
        {-2400.f, -820.f, 0.f}, {2520.f, 830.f, 90.f},
        {2400.f, -1860.f, 0.f}
    };
    for (int32 I = 0; I < 3; ++I)
    {
        const FVector CarCenter(Cars[I].X, Cars[I].Y, 0);
        const FRotator CarRotation(0, Cars[I].Yaw, 0);
        // Body/cabin geometry rotates around its own centre. Wheels are visual proxies.
        AddBlock(FString::Printf(TEXT("CarBody_%d"), I),
            CarCenter + FVector(0, 0, 75), FVector(400, 180, 95), CarRotation);
        AddBlock(FString::Printf(TEXT("CarCabin_%d"), I),
            CarCenter + FVector(0, 0, 155), FVector(235, 155, 70), CarRotation);
        for (int32 Axle : {-1, 1})
            for (int32 Side : {-1, 1})
            {
                const FVector WheelOffset = CarRotation.RotateVector(
                    FVector(Axle * 125.f, Side * 92.f, 38.f));
                AddBlock(FString::Printf(TEXT("CarWheel_%d_%d_%d"), I, Axle, Side),
                    CarCenter + WheelOffset, FVector(62, 28, 70), CarRotation);
            }
    }

    // Gas station signs and forecourt safety bollards.
    AddBlock(TEXT("FuelPriceSignPost"), FVector(2350, -700, 190), FVector(34, 34, 380));
    AddBlock(TEXT("FuelPriceSignFace"), FVector(2350, -700, 390), FVector(160, 32, 180));
    for (int32 I = 0; I < 4; ++I)
        AddBlock(FString::Printf(TEXT("ForecourtBollard_%d"), I),
            FVector(1020.f + I * 260.f, -90, 50), FVector(28, 28, 100));

    // Low debris and abandoned planters outside entry paths.
    for (int32 I = 0; I < 8; ++I)
    {
        const float X = -2650.f + I * 700.f;
        const float Y = (I % 2 == 0) ? 680.f : -680.f;
        AddBlock(FString::Printf(TEXT("Planter_%d"), I),
            FVector(X, Y, 35), FVector(110, 90, 70));
        AddBlock(FString::Printf(TEXT("Overgrowth_%d"), I),
            FVector(X, Y, 95), FVector(80, 75, 80));
    }

    // Pack 20: shrubs, young trees and debris kept outside the main roads.
    for (int32 I = 0; I < 18; ++I)
    {
        const float X = -2750.f + (I % 9) * 650.f;
        const float Y = (I < 9) ? 2040.f : -2070.f;
        const float Spread = (I % 3 - 1) * 55.f;
        AddBlock(FString::Printf(TEXT("ShrubStem_%d"), I),
            FVector(X, Y + Spread, 48), FVector(24, 24, 96));
        AddBlock(FString::Printf(TEXT("ShrubLeaves_%d"), I),
            FVector(X, Y + Spread, 122), FVector(155, 145, 115));
        if (I % 3 == 0)
        {
            AddBlock(FString::Printf(TEXT("ShrubBranch_%d"), I),
                FVector(X + 48.f, Y + Spread, 107), FVector(95, 34, 32),
                FRotator(0, 0, 24));
        }
    }

    // Uneven piles of discarded lumber and rubble near the back lots.
    for (int32 I = 0; I < 9; ++I)
    {
        const float X = -2450.f + I * 590.f;
        const float Y = (I % 2 == 0) ? 1910.f : -1900.f;
        AddBlock(FString::Printf(TEXT("DebrisPlank_%d"), I),
            FVector(X, Y, 22), FVector(155, 35, 32), FRotator(0, I * 23.f, 5.f));
        AddBlock(FString::Printf(TEXT("Rubble_%d"), I),
            FVector(X + 78.f, Y + 38.f, 22), FVector(65, 70, 42),
            FRotator(0, I * 37.f, 0));
    }

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
    // Pack 20: raised roof profile, porch and awning. Existing flat roof
    // remains underneath as a weatherproof ceiling for accessible interiors.
    const float FrontDirection = FrontY > Center.Y ? 1.f : -1.f;
    if (!bShop)
    {
        const float RoofSlope = 20.f;
        const float RoofWidth = Width * 0.54f;
        AddBlock(Name + TEXT("_PitchedRoofLeft"),
            FVector(Center.X - Width * 0.25f, Center.Y, Height + 110.f),
            FVector(RoofWidth, Depth + 95.f, 26.f),
            FRotator(0, 0, RoofSlope));
        AddBlock(Name + TEXT("_PitchedRoofRight"),
            FVector(Center.X + Width * 0.25f, Center.Y, Height + 110.f),
            FVector(RoofWidth, Depth + 95.f, 26.f),
            FRotator(0, 0, -RoofSlope));
        AddBlock(Name + TEXT("_PorchDeck"),
            FVector(Center.X, FrontY + FrontDirection * 170.f, 12.f),
            FVector(390, 300, 24));
        for (int32 Side : {-1, 1})
        {
            AddBlock(Name + FString::Printf(TEXT("_PorchPost_%d"), Side),
                FVector(Center.X + Side * 175.f,
                    FrontY + FrontDirection * 285.f, 125.f),
                FVector(25, 25, 250));
        }
        AddBlock(Name + TEXT("_PorchCover"),
            FVector(Center.X, FrontY + FrontDirection * 170.f, 262.f),
            FVector(440, 320, 25));
        AddBlock(Name + TEXT("_Chimney"),
            FVector(Center.X + Width * 0.30f, Center.Y + Depth * 0.18f,
                Height + 135.f), FVector(95, 95, 270));
    }
    else
    {
        AddBlock(Name + TEXT("_StoreAwning"),
            FVector(Center.X, FrontY + FrontDirection * 135.f, Height - 65.f),
            FVector(Width * 0.60f, 260, 22));
        for (int32 Side : {-1, 1})
            AddBlock(Name + FString::Printf(TEXT("_AwningBracket_%d"), Side),
                FVector(Center.X + Side * Width * 0.24f,
                    FrontY + FrontDirection * 85.f, Height - 115.f),
                FVector(18, 130, 80));
        AddBlock(Name + TEXT("_RoofVent"),
            FVector(Center.X + Width * 0.28f, Center.Y, Height + 60.f),
            FVector(120, 95, 85));
    }
    // Architectural trim sits on existing walls; door opening remains unobstructed.
    const float DoorFrameX = DoorWidth * 0.5f + 8.f;
    const float FrontSign = (FrontY > Center.Y) ? 1.f : -1.f;
    for (int32 Side : {-1, 1})
    {
        AddBlock(Name + FString::Printf(TEXT("_DoorJamb_%d"), Side),
            FVector(Center.X + Side * DoorFrameX, FrontY + FrontSign * 18.f, 112.f),
            FVector(12, 14, 224));
        // Window panels are decorative, not openings in the current greybox walls.
        AddBlock(Name + FString::Printf(TEXT("_WindowPanel_%d"), Side),
            FVector(Center.X + Side * Width * 0.32f, FrontY + FrontSign * 18.f, 190.f),
            FVector(125, 12, 100));
        AddBlock(Name + FString::Printf(TEXT("_WindowSill_%d"), Side),
            FVector(Center.X + Side * Width * 0.32f, FrontY + FrontSign * 26.f, 135.f),
            FVector(150, 26, 12));
    }
    if (bShop)
    {
        AddBlock(Name + TEXT("_StoreSign"),
            FVector(Center.X, FrontY + FrontSign * 28.f, Height - 90.f),
            FVector(Width * 0.42f, 18, 55));
    }
}
