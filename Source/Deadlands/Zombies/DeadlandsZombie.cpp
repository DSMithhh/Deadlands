#include "Zombies/DeadlandsZombie.h"
#include "DeadlandsCharacter.h"
#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"

ADeadlandsZombie::ADeadlandsZombie()
{
    bReplicates = true;
    SetReplicateMovement(true);
    PrimaryActorTick.bCanEverTick = true;
    AIControllerClass = AAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
    GetCapsuleComponent()->InitCapsuleSize(38.f, 88.f);
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    bUseControllerRotationYaw = false;

    PlaceholderBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderBody"));
    PlaceholderBody->SetupAttachment(GetCapsuleComponent());
    PlaceholderBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PlaceholderBody->SetRelativeScale3D(FVector(0.65f, 0.65f, 1.35f));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (Sphere.Succeeded()) PlaceholderBody->SetStaticMesh(Sphere.Object);
}

void ADeadlandsZombie::BeginPlay()
{
    Super::BeginPlay();
    if (HasAuthority())
    {
        Health = MaxHealth;
        GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
    }
}

void ADeadlandsZombie::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!HasAuthority() || bDead || !GetWorld()) return;
    const float Now = GetWorld()->GetTimeSeconds();
    if (Now < NextDecisionTime) return;
    NextDecisionTime = Now + 0.35f;

    ADeadlandsCharacter* Nearest = nullptr;
    float BestDistanceSq = FMath::Square(DetectionRange);
    for (TActorIterator<ADeadlandsCharacter> It(GetWorld()); It; ++It)
    {
        ADeadlandsCharacter* Candidate = *It;
        if (!IsValid(Candidate) || Candidate->GetHealth() <= 0.f) continue;
        const float DistanceSq = FVector::DistSquared2D(GetActorLocation(), Candidate->GetActorLocation());
        if (DistanceSq < BestDistanceSq)
        {
            BestDistanceSq = DistanceSq;
            Nearest = Candidate;
        }
    }

    AAIController* AI = Cast<AAIController>(GetController());
    if (!Nearest)
    {
        if (AI) AI->StopMovement();
        return;
    }

    const float Distance = FVector::Dist(GetActorLocation(), Nearest->GetActorLocation());
    if (Distance <= AttackRange)
    {
        if (AI) AI->StopMovement();
        if (Now >= NextAttackTime)
        {
            NextAttackTime = Now + AttackCooldown;
            Nearest->ApplyCombatDamage(AttackDamage);
        }
    }
    else
    {
        // Prototype direct pursuit does not require a Nav Mesh Bounds Volume.
        // CharacterMovement still respects physical collisions; obstacle pathfinding comes later.
        if (AI) AI->StopMovement();
        FVector Toward = Nearest->GetActorLocation() - GetActorLocation();
        Toward.Z = 0.f;
        if (!Toward.IsNearlyZero()) AddMovementInput(Toward.GetSafeNormal(), 1.f);
    }
}

void ADeadlandsZombie::ReceiveMeleeDamage(float Amount)
{
    if (!HasAuthority() || bDead || Amount <= 0.f) return;
    Health = FMath::Max(0.f, Health - Amount);
    OnRep_Health();
    if (Health <= 0.f)
    {
        bDead = true;
        if (AAIController* AI = Cast<AAIController>(GetController())) AI->StopMovement();
        GetCharacterMovement()->DisableMovement();
        GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        ApplyDeathVisual();
    }
    ForceNetUpdate();
}

void ADeadlandsZombie::ApplyDeathVisual()
{
    if (PlaceholderBody) PlaceholderBody->SetVisibility(!bDead);
    if (bDead) SetActorTickEnabled(false);
}
void ADeadlandsZombie::OnRep_Health() {}
void ADeadlandsZombie::OnRep_Dead()
{
    if (bDead)
    {
        GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        ApplyDeathVisual();
    }
}
void ADeadlandsZombie::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ADeadlandsZombie, Health);
    DOREPLIFETIME(ADeadlandsZombie, bDead);
}
