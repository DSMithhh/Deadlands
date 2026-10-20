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

bool ADeadlandsZombie::CanSeePlayer(const ADeadlandsCharacter* Player) const
{
    if (!IsValid(Player) || !GetWorld()) return false;
    const FVector Eye = GetActorLocation() + FVector(0.f, 0.f, 60.f);
    const FVector TargetEye = Player->GetActorLocation() + FVector(0.f, 0.f, 60.f);
    const FVector Direction = (TargetEye - Eye).GetSafeNormal();
    const FVector Forward = GetActorForwardVector();
    if (FVector::DotProduct(Forward, Direction) < FMath::Cos(FMath::DegreesToRadians(FieldOfViewDegrees * 0.5f))) return false;
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(DeadlandsZombieSight), false, this);
    const bool bBlocked = GetWorld()->LineTraceSingleByChannel(Hit, Eye, TargetEye, ECC_Visibility, Params);
    return !bBlocked || Hit.GetActor() == Player;
}

FVector ADeadlandsZombie::ChooseSteering(const FVector& Desired) const
{
    if (!GetWorld()) return Desired;
    const FVector Start = GetActorLocation() + FVector(0.f, 0.f, 45.f);
    FCollisionQueryParams Params(SCENE_QUERY_STAT(DeadlandsZombieSteering), false, this);
    auto Clear = [&](const FVector& Dir)
    {
        FHitResult Hit;
        return !GetWorld()->SweepSingleByChannel(Hit, Start, Start + Dir * 140.f,
            FQuat::Identity, ECC_WorldStatic, FCollisionShape::MakeSphere(28.f), Params);
    };
    if (Clear(Desired)) return Desired;
    // Lightweight no-navmesh obstacle avoidance for the test level.
    const FVector Left = Desired.RotateAngleAxis(65.f, FVector::UpVector);
    const FVector Right = Desired.RotateAngleAxis(-65.f, FVector::UpVector);
    if (Clear(Left)) return Left;
    if (Clear(Right)) return Right;
    return FVector::ZeroVector;
}

void ADeadlandsZombie::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!HasAuthority() || bDead || !GetWorld()) return;
    const float Now = GetWorld()->GetTimeSeconds();

    ADeadlandsCharacter* Best = nullptr;
    float BestScore = -1.f;
    for (TActorIterator<ADeadlandsCharacter> It(GetWorld()); It; ++It)
    {
        ADeadlandsCharacter* Candidate = *It;
        if (!IsValid(Candidate) || Candidate->GetHealth() <= 0.f) continue;
        const float Distance = FVector::Dist(GetActorLocation(), Candidate->GetActorLocation());
        const bool bVisible = Distance <= DetectionRange && CanSeePlayer(Candidate);
        const float AudibleRange = Candidate->IsSprinting() ? SprintHearingRange : HearingRange;
        const bool bAudible = Distance <= AudibleRange;
        if (!bVisible && !bAudible) continue;
        // Sight is more compelling than footsteps; sprinting attracts from farther away.
        const float Score = (bVisible ? 2000.f : 1000.f) + (Candidate->IsSprinting() ? 300.f : 0.f) - Distance * 0.5f;
        if (Score > BestScore) { BestScore = Score; Best = Candidate; }
    }

    if (Best)
    {
        CurrentTarget = Best;
        LastKnownLocation = Best->GetActorLocation();
        LastSensedTime = Now;
    }

    const bool bRemembering = Now - LastSensedTime <= MemorySeconds;
    ADeadlandsCharacter* Target = CurrentTarget.Get();
    AAIController* AI = Cast<AAIController>(GetController());
    if (!bRemembering)
    {
        CurrentTarget.Reset();
        if (AI) AI->StopMovement();
        return;
    }

    // Attack only while currently sensing the victim; no damage through walls.
    if (Best && Target == Best &&
        FVector::Dist(GetActorLocation(), Best->GetActorLocation()) <= AttackRange &&
        CanSeePlayer(Best))
    {
        if (AI) AI->StopMovement();
        if (Now >= NextAttackTime)
        {
            NextAttackTime = Now + AttackCooldown;
            Best->ApplyCombatDamage(AttackDamage);
        }
        return;
    }

    FVector Toward = LastKnownLocation - GetActorLocation();
    Toward.Z = 0.f;
    if (Toward.SizeSquared() < FMath::Square(65.f))
    {
        if (AI) AI->StopMovement();
        return;
    }
    if (AI) AI->StopMovement();
    AddMovementInput(ChooseSteering(Toward.GetSafeNormal()), 1.f);
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
