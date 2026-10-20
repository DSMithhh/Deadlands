#include "Zombies/DeadlandsZombie.h"
#include "DeadlandsCharacter.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
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
        HomeLocation = GetActorLocation();
        NextWanderTime = GetWorld()->GetTimeSeconds() + FMath::FRandRange(3.f, 7.f);
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

void ADeadlandsZombie::SetZombieState(EDeadlandsZombieState NewState)
{
    if (ZombieState != NewState)
    {
        ZombieState = NewState;
        ForceNetUpdate();
    }
}

void ADeadlandsZombie::NavigateTo(const FVector& Destination, float AcceptanceRadius, float Now)
{
    if (Now < NextDecisionTime) return;
    NextDecisionTime = Now + 0.55f;
    AAIController* AI = Cast<AAIController>(GetController());
    if (!AI) return;
    UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if (!Nav) { AI->StopMovement(); return; }
    FNavLocation Projected;
    if (Nav->ProjectPointToNavigation(Destination, Projected, FVector(200.f, 200.f, 300.f)))
    {
        AI->MoveToLocation(Projected.Location, AcceptanceRadius, true, true, false, true, nullptr, true);
    }
    else AI->StopMovement();
}

void ADeadlandsZombie::UpdatePassiveBehavior(float Now)
{
    if (bSearching)
    {
        SetZombieState(EDeadlandsZombieState::Searching);
        GetCharacterMovement()->MaxWalkSpeed = WalkSpeed * 0.65f;
        if (FVector::DistSquared2D(GetActorLocation(), SearchDestination) < FMath::Square(110.f))
        {
            SearchDestination = LastKnownLocation + FVector(
                FMath::FRandRange(-220.f, 220.f), FMath::FRandRange(-220.f, 220.f), 0.f);
        }
        NavigateTo(SearchDestination, 90.f, Now);
        if (Now < SearchEndTime) return;
        bSearching = false;
        NextWanderTime = Now + 2.f;
    }

    if (FVector::DistSquared2D(GetActorLocation(), HomeLocation) > FMath::Square(950.f))
    {
        SetZombieState(EDeadlandsZombieState::Returning);
        GetCharacterMovement()->MaxWalkSpeed = WalkSpeed * 0.7f;
        NavigateTo(HomeLocation, 130.f, Now);
        return;
    }

    if (Now >= NextWanderTime)
    {
        NextWanderTime = Now + FMath::FRandRange(5.f, 10.f);
        WanderDestination = HomeLocation + FVector(
            FMath::FRandRange(-450.f, 450.f), FMath::FRandRange(-450.f, 450.f), 0.f);
        bHasWanderDestination = true;
    }
    if (bHasWanderDestination &&
        FVector::DistSquared2D(GetActorLocation(), WanderDestination) > FMath::Square(100.f))
    {
        SetZombieState(EDeadlandsZombieState::Wandering);
        GetCharacterMovement()->MaxWalkSpeed = WalkSpeed * 0.45f;
        NavigateTo(WanderDestination, 95.f, Now);
    }
    else
    {
        bHasWanderDestination = false;
        SetZombieState(EDeadlandsZombieState::Idle);
        if (AAIController* AI = Cast<AAIController>(GetController())) AI->StopMovement();
    }
}

void ADeadlandsZombie::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!HasAuthority() || bDead || !GetWorld()) return;
    const float Now = GetWorld()->GetTimeSeconds();

    // Commit to an existing hunt for a short period instead of switching to any visible bystander.
    ADeadlandsCharacter* Target = CurrentTarget.Get();
    if (Target && (!IsValid(Target) || Target->GetHealth() <= 0.f))
    {
        CurrentTarget.Reset();
        Target = nullptr;
        LastSensedTime = -1000.f;
    }

    ADeadlandsCharacter* Best = nullptr;
    float BestScore = -1.f;
    for (TActorIterator<ADeadlandsCharacter> It(GetWorld()); It; ++It)
    {
        ADeadlandsCharacter* Candidate = *It;
        if (!IsValid(Candidate) || Candidate->GetHealth() <= 0.f) continue;
        const float Distance = FVector::Dist(GetActorLocation(), Candidate->GetActorLocation());
        const bool bVisible = Distance <= DetectionRange && CanSeePlayer(Candidate);
        // Walking is deliberately quiet; sprinting is audible through walls.
        const bool bAudible = Candidate->IsSprinting() && Distance <= SprintHearingRange;
        const bool bClose = Distance <= 175.f;
        if (!bVisible && !bAudible && !bClose) continue;

        const float Score = (bVisible ? 2000.f : 0.f) +
            (bAudible ? 900.f : 0.f) + (bClose ? 500.f : 0.f) - Distance * 0.5f;
        // A remembered target wins unless another player is very close or creates a strong distraction.
        if (Candidate == Target && Now - LastSensedTime <= MemorySeconds)
        {
            Best = Candidate;
            BestScore = Score;
            break;
        }
        if (Score > BestScore) { BestScore = Score; Best = Candidate; }
    }

    if (Best)
    {
        const bool bCommitted = Target && Now - LastSensedTime <= MemorySeconds;
        const float BestDistance = FVector::Dist(GetActorLocation(), Best->GetActorLocation());
        const bool bStrongDistraction = Best != Target &&
            (BestDistance <= 220.f || (Best->IsSprinting() && BestDistance <= 450.f));
        if (!bCommitted || Best == Target || bStrongDistraction)
        {
            CurrentTarget = Best;
            Target = Best;
            LastKnownLocation = Best->GetActorLocation();
            LastSensedTime = Now;
            bSearching = false;
            GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
        }
    }

    const bool bRemembering = Now - LastSensedTime <= MemorySeconds;
    Target = CurrentTarget.Get();
    AAIController* AI = Cast<AAIController>(GetController());
    if (!bRemembering)
    {
        if (Target && !bSearching)
        {
            bSearching = true;
            SearchEndTime = Now + 6.f;
            SearchDestination = LastKnownLocation;
        }
        CurrentTarget.Reset();
        UpdatePassiveBehavior(Now);
        return;
    }

    // Attack only while currently sensing the victim; no damage through walls.
    if (Best && Target == Best &&
        FVector::Dist(GetActorLocation(), Best->GetActorLocation()) <= AttackRange &&
        CanSeePlayer(Best))
    {
        SetZombieState(EDeadlandsZombieState::Attacking);
        if (AI) AI->StopMovement();
        if (Now >= NextAttackTime)
        {
            NextAttackTime = Now + AttackCooldown;
            Best->ApplyCombatDamage(AttackDamage);
        }
        return;
    }

    SetZombieState(Best == Target ? EDeadlandsZombieState::Chasing : EDeadlandsZombieState::Suspicious);
    GetCharacterMovement()->MaxWalkSpeed = Best == Target ? WalkSpeed : WalkSpeed * 0.8f;
    NavigateTo(LastKnownLocation, 65.f, Now);
}

void ADeadlandsZombie::ReceiveMeleeDamage(float Amount)
{
    if (!HasAuthority() || bDead || Amount <= 0.f) return;
    Health = FMath::Max(0.f, Health - Amount);
    OnRep_Health();
    if (Health <= 0.f)
    {
        bDead = true;
        SetZombieState(EDeadlandsZombieState::Dead);
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
    DOREPLIFETIME(ADeadlandsZombie, ZombieState);
}
