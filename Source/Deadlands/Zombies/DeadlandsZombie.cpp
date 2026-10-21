#include "Zombies/DeadlandsZombie.h"
#include "DeadlandsCharacter.h"
#include "Interaction/DeadlandsContainer.h"
#include "Persistence/DeadlandsWorldPersistence.h"
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
    PlaceholderBody->SetRelativeScale3D(FVector(0.37f, 0.23f, 0.57f));
    PlaceholderBody->SetRelativeLocation(FVector(0.f, 0.f, 14.f));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (Sphere.Succeeded()) PlaceholderBody->SetStaticMesh(Sphere.Object);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    auto AddPart = [&](const TCHAR* Name, const FVector& Location, const FVector& Scale, bool bRound) -> UStaticMeshComponent*
    {
        UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
        Part->SetupAttachment(GetCapsuleComponent());
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Part->SetRelativeLocation(Location);
        Part->SetRelativeScale3D(Scale);
        if (bRound && Sphere.Succeeded()) Part->SetStaticMesh(Sphere.Object);
        else if (Cube.Succeeded()) Part->SetStaticMesh(Cube.Object);
        return Part;
    };
    HeadVisual = AddPart(TEXT("ZombieHead"), FVector(13.f, 0.f, 63.f), FVector(.30f, .27f, .32f), true);
    FaceVisual = AddPart(TEXT("ZombieFaceDirection"), FVector(39.f, 0.f, 64.f), FVector(.10f, .20f, .10f), false);
    LeftArmVisual = AddPart(TEXT("ZombieLeftArm"), FVector(5.f, -30.f, 8.f), FVector(.18f, .17f, .68f), false);
    RightArmVisual = AddPart(TEXT("ZombieRightArm"), FVector(5.f, 30.f, 8.f), FVector(.18f, .17f, .68f), false);
    LeftLegVisual = AddPart(TEXT("ZombieLeftLeg"), FVector(0.f, -14.f, -49.f), FVector(.19f, .20f, .75f), false);
    RightLegVisual = AddPart(TEXT("ZombieRightLeg"), FVector(0.f, 14.f, -49.f), FVector(.19f, .20f, .75f), false);
}

void ADeadlandsZombie::BeginPlay()
{
    Super::BeginPlay();
    if (HasAuthority())
    {
        // Two server-selected prototypes: common walkers and uncommon runners.
        bRareVariant = FMath::FRand() < 0.10f;
        bFastVariant = !bRareVariant && FMath::FRand() < 0.25f;
        WalkSpeed = bRareVariant ? 145.f : (bFastVariant ? 310.f : 180.f);
        MaxHealth = bRareVariant ? 120.f : (bFastVariant ? 40.f : FMath::FRandRange(50.f, 75.f));
        AttackDamage = bRareVariant ? 17.f : (bFastVariant ? 7.f : 10.f);
        OnRep_Variant();
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

void ADeadlandsZombie::HearCombatNoise(const FVector& NoiseLocation)
{
    if (!HasAuthority() || bDead || !GetWorld()) return;
    if (FVector::DistSquared(GetActorLocation(), NoiseLocation) > FMath::Square(1300.f)) return;
    if (CurrentTarget.IsValid()) return;
    LastKnownLocation = NoiseLocation;
    LastSensedTime = GetWorld()->GetTimeSeconds();
    bSearching = false;
    SetZombieState(EDeadlandsZombieState::Suspicious);
}

void ADeadlandsZombie::AlertFromZombie(const FVector& AlertLocation)
{
    if (!HasAuthority() || bDead || CurrentTarget.IsValid() || !GetWorld()) return;
    if (FVector::DistSquared(GetActorLocation(), AlertLocation) > FMath::Square(900.f)) return;
    LastKnownLocation = AlertLocation;
    LastSensedTime = GetWorld()->GetTimeSeconds();
    bSearching = false;
    SetZombieState(EDeadlandsZombieState::Suspicious);
}

void ADeadlandsZombie::BroadcastAlert(const FVector& Location)
{
    if (!GetWorld()) return;
    for (TActorIterator<ADeadlandsZombie> It(GetWorld()); It; ++It)
    {
        if (*It != this) It->AlertFromZombie(Location);
    }
}

void ADeadlandsZombie::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    UpdateZombieVisual(DeltaSeconds);
    if (!HasAuthority() || !GetWorld()) return;
    if (bDead)
    {
        if (!bLootBagRevealed && GetLifeSpan() > 0.f && GetLifeSpan() < 1.5f)
        {
            bLootBagRevealed = true;
            if (IsValid(CorpseLoot)) CorpseLoot->RevealCorpseBag();
        }
        return;
    }
    const float Now = GetWorld()->GetTimeSeconds();
    if (Now < StaggerEndTime)
    {
        if (AAIController* AI = Cast<AAIController>(GetController())) AI->StopMovement();
        return;
    }

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
            if (Now >= NextAlertTime)
            {
                NextAlertTime = Now + 5.f;
                BroadcastAlert(LastKnownLocation);
            }
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

    // Wind-up is interruptible. Revalidate the victim, reach, and line of sight
    // at the instant damage lands so a player can escape around a corner.
    if (bWindingUp)
    {
        ADeadlandsCharacter* Victim = WindupTarget.Get();
        if (!IsValid(Victim) || Victim->GetHealth() <= 0.f ||
            FVector::Dist(GetActorLocation(), Victim->GetActorLocation()) > AttackRange ||
            !CanSeePlayer(Victim))
        {
            bWindingUp = false;
            WindupTarget.Reset();
        }
        else if (Now >= AttackWindupEnd)
        {
            bWindingUp = false;
            WindupTarget.Reset();
            NextAttackTime = Now + AttackCooldown;
            Victim->ApplyCombatDamage(AttackDamage);
            FVector Push = Victim->GetActorLocation() - GetActorLocation();
            Push.Z = 0.f;
            Victim->LaunchCharacter(Push.GetSafeNormal() * KnockbackStrength + FVector(0.f, 0.f, 120.f), true, false);
            HearCombatNoise(GetActorLocation());
        }
        if (bWindingUp)
        {
            SetZombieState(EDeadlandsZombieState::Attacking);
            if (AI) AI->StopMovement();
            return;
        }
    }

    if (Best && Target == Best &&
        FVector::Dist(GetActorLocation(), Best->GetActorLocation()) <= AttackRange &&
        CanSeePlayer(Best))
    {
        SetZombieState(EDeadlandsZombieState::Attacking);
        if (AI) AI->StopMovement();
        if (Now >= NextAttackTime)
        {
            bWindingUp = true;
            WindupTarget = Best;
            AttackWindupEnd = Now + AttackWindupSeconds;
        }
        return;
    }

    SetZombieState(Best == Target ? EDeadlandsZombieState::Chasing : EDeadlandsZombieState::Suspicious);
    GetCharacterMovement()->MaxWalkSpeed = Best == Target ? WalkSpeed : WalkSpeed * 0.8f;
    NavigateTo(LastKnownLocation, 65.f, Now);
}

void ADeadlandsZombie::ReceiveMeleeDamage(float Amount, bool bHeadshot, bool bHeavy)
{
    if (!HasAuthority() || bDead || Amount <= 0.f) return;
    Health = FMath::Max(0.f, Health - Amount * (bHeadshot ? 2.5f : 1.f));
    ++HitReactionSerial;
    OnRep_HitReaction();
    bWindingUp = false;
    WindupTarget.Reset();
    StaggerEndTime = GetWorld() ? GetWorld()->GetTimeSeconds() + StaggerDuration * (bHeavy ? 1.9f : 1.f) : 0.f;
    NextAttackTime = StaggerEndTime + 0.4f;
    if (AAIController* AI = Cast<AAIController>(GetController())) AI->StopMovement();
    OnRep_Health();
    if (Health <= 0.f)
    {
        bDead = true;
        SetZombieState(EDeadlandsZombieState::Dead);
        if (AAIController* AI = Cast<AAIController>(GetController())) AI->StopMovement();
        GetCharacterMovement()->DisableMovement();
        GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);
        GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
        CreateCorpseLoot();
        ApplyDeathVisual();
        SetLifeSpan(CorpseDestroyDelay);
    }
    ForceNetUpdate();
}

bool ADeadlandsZombie::CanInteract_Implementation(ACharacter* InteractingCharacter) const
{
    return bDead && IsValid(CorpseLoot) && IsValid(InteractingCharacter);
}
void ADeadlandsZombie::Interact_Implementation(ACharacter* InteractingCharacter)
{
    // Client opens the existing inventory widget; the server validates interaction range.
}
void ADeadlandsZombie::CreateCorpseLoot()
{
    if (!HasAuthority() || !GetWorld() || IsValid(CorpseLoot)) return;
    TArray<FDeadlandsItemStack> Rolled;
    auto Roll = [&](FName Id, float Chance, int32 MaxCount)
    {
        if (FMath::FRand() < Chance)
        {
            const int32 Count = FMath::RandRange(1, MaxCount);
            for (int32 i = 0; i < Count; ++i) FDeadlandsItems::AddOne(Rolled, Id, 1000);
        }
    };
    const bool bRare = bRareVariant;
    Roll(TEXT("Cloth"), .65f, 3);
    Roll(TEXT("Scrap"), .55f, 3);
    Roll(TEXT("Food"), .22f, 2);
    Roll(TEXT("Water"), .20f, 2);
    Roll(TEXT("Bandage"), bRare ? .55f : .18f, 2);
    Roll(TEXT("Medicine"), bRare ? .35f : .07f, 1);
    Roll(TEXT("Antibiotics"), bRare ? .25f : .03f, 1);
    Roll(TEXT("Ammo9mm"), bFastVariant ? .3f : .12f, 5);
    Roll(TEXT("Tools"), bRare ? .22f : .08f, 1);
    if (Rolled.IsEmpty()) Roll(TEXT("Scrap"), 1.f, 1);
    const FName DropId(*FString::Printf(TEXT("Corpse_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits)));
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    ADeadlandsContainer* Bag = GetWorld()->SpawnActorDeferred<ADeadlandsContainer>(
        ADeadlandsContainer::StaticClass(), FTransform(FRotator::ZeroRotator, GetActorLocation() + FVector(0.f, 0.f, -76.f)),
        nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if (!Bag) return;
    Bag->ConfigureCorpseLoot(DropId, Rolled, false);
    Bag->FinishSpawning(FTransform(FRotator::ZeroRotator, GetActorLocation() + FVector(0.f, 0.f, -76.f)));
    CorpseLoot = Bag;
    FDeadlandsWorldPersistence::RegisterCorpseBag(this, DropId.ToString(), Bag->GetActorLocation());
    ForceNetUpdate();
}
void ADeadlandsZombie::ApplyDeathVisual()
{
    // Keep the inert placeholder visible as a temporary corpse until cleanup.
    // Continue ticking so the corpse can finish its collapse.
}
void ADeadlandsZombie::OnRep_Variant()
{
    if (PlaceholderBody) PlaceholderBody->SetRelativeScale3D(bRareVariant ? FVector(.52f, .33f, .72f) : (bFastVariant ? FVector(.30f, .20f, .52f) : FVector(.37f, .23f, .57f)));
    if (HeadVisual) HeadVisual->SetRelativeScale3D(bRareVariant ? FVector(.39f, .36f, .39f) : FVector(.30f, .27f, .32f));
}
void ADeadlandsZombie::OnRep_Health() {}
void ADeadlandsZombie::OnRep_HitReaction()
{
    VisualStaggerRemaining = 0.45f;
    LastHitDirection *= -1.f;
}

void ADeadlandsZombie::UpdateZombieVisual(float DeltaSeconds)
{
    if (!PlaceholderBody || !HeadVisual || !LeftArmVisual || !RightArmVisual || !LeftLegVisual || !RightLegVisual) return;
    VisualTime += DeltaSeconds;
    VisualStaggerRemaining = FMath::Max(0.f, VisualStaggerRemaining - DeltaSeconds);
    const float Speed = GetVelocity().Size2D();
    const float Moving = bDead ? 0.f : FMath::Clamp(Speed / 150.f, 0.f, 1.f);
    const float Frequency = ZombieState == EDeadlandsZombieState::Chasing ? 9.f : 5.f;
    const float Swing = FMath::Sin(VisualTime * Frequency) * 30.f * Moving;
    const bool bAttacking = !bDead && ZombieState == EDeadlandsZombieState::Attacking;
    const float AttackPulse = FMath::Sin(VisualTime * 13.f);
    LeftLegVisual->SetRelativeRotation(FRotator(Swing, 0.f, 0.f));
    RightLegVisual->SetRelativeRotation(FRotator(-Swing, 0.f, 0.f));
    LeftArmVisual->SetRelativeRotation(FRotator(bAttacking ? -65.f + AttackPulse * 22.f : -18.f - Swing * .6f, 0.f, -12.f));
    RightArmVisual->SetRelativeRotation(FRotator(bAttacking ? -65.f - AttackPulse * 22.f : -18.f + Swing * .6f, 0.f, 12.f));
    HeadVisual->SetRelativeRotation(FRotator(bAttacking ? -10.f : 5.f + FMath::Sin(VisualTime * 2.f) * 4.f, 0.f, 0.f));
    const float HitTilt = VisualStaggerRemaining > 0.f ? FMath::Sin((0.45f - VisualStaggerRemaining) * 11.f) * 20.f * LastHitDirection : 0.f;
    PlaceholderBody->SetRelativeRotation(FRotator(HitTilt, 0.f, FMath::Sin(VisualTime * Frequency) * 3.f * Moving));
    if (bDead)
    {
        DeathBlend = FMath::Min(1.f, DeathBlend + DeltaSeconds * 1.4f);
        const float Fall = FMath::InterpEaseInOut(0.f, 1.f, DeathBlend, 2.f);
        PlaceholderBody->SetRelativeRotation(FRotator(75.f * Fall, 0.f, 0.f));
        PlaceholderBody->SetRelativeLocation(FVector(25.f * Fall, 0.f, 14.f - 36.f * Fall));
        HeadVisual->SetRelativeLocation(FVector(13.f + 60.f * Fall, 0.f, 63.f - 95.f * Fall));
        FaceVisual->SetRelativeLocation(FVector(39.f + 55.f * Fall, 0.f, 64.f - 95.f * Fall));
        LeftArmVisual->SetRelativeRotation(FRotator(-18.f, 0.f, -75.f * Fall));
        RightArmVisual->SetRelativeRotation(FRotator(-18.f, 0.f, 75.f * Fall));
        LeftLegVisual->SetRelativeRotation(FRotator(55.f * Fall, 0.f, 0.f));
        RightLegVisual->SetRelativeRotation(FRotator(-25.f * Fall, 0.f, 0.f));
    }
}
void ADeadlandsZombie::OnRep_Dead()
{
    if (bDead)
    {
        GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);
        GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
        ApplyDeathVisual();
    }
}
void ADeadlandsZombie::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ADeadlandsZombie, Health);
    DOREPLIFETIME(ADeadlandsZombie, bDead);
    DOREPLIFETIME(ADeadlandsZombie, ZombieState);
    DOREPLIFETIME(ADeadlandsZombie, HitReactionSerial);
    DOREPLIFETIME(ADeadlandsZombie, CorpseLoot);
    DOREPLIFETIME(ADeadlandsZombie, bFastVariant);
    DOREPLIFETIME(ADeadlandsZombie, bRareVariant);
}
