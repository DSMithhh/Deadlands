#include "Interaction/DeadlandsContainer.h"
#include "DeadlandsCharacter.h"
#include "Deadlands.h"
#include "Components/StaticMeshComponent.h"
#include "Persistence/DeadlandsWorldPersistence.h"
#include "Net/UnrealNetwork.h"

ADeadlandsContainer::ADeadlandsContainer()
{
    bReplicates = true;
    ContainerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ContainerMesh"));
    RootComponent = ContainerMesh;
    ContainerMesh->SetCollisionProfileName(TEXT("BlockAll"));
}

void ADeadlandsContainer::BeginPlay()
{
    Super::BeginPlay();
    if (HasAuthority())
    {
        if (!GetIsReplicated()) SetReplicates(true);
        FDeadlandsWorldPersistence::LoadActorFlag(this, GetPersistenceKey(), TEXT("Open"), bOpen);
        RemainingSupplies = FMath::Max(0, InitialSupplies);
        // A saved zero is meaningful: empty containers never refill.
        FDeadlandsWorldPersistence::LoadContainerSupplies(this, GetPersistenceKey(), RemainingSupplies);
        Items = InitialItems;
        // Never refill an inventory already saved as empty.
        if (!FDeadlandsWorldPersistence::LoadInventory(this, TEXT("Container|") + GetPersistenceKey(), Items))
        {
            TArray<FDeadlandsItemStack> Sanitized;
            for (const auto& Stack : Items)
            {
                if (!FDeadlandsItems::Valid(Stack)) continue;
                for (int32 i = 0; i < FMath::Min(Stack.Quantity, 1000); ++i)
                    FDeadlandsItems::AddOne(Sanitized, Stack.ItemId, 1000);
            }
            Items = MoveTemp(Sanitized);
            if (Items.IsEmpty() && InitialItems.IsEmpty() && bGenerateLoot)
            {
                TArray<FDeadlandsItemStack> Rolled = GenerateInitialLoot();
                if (FDeadlandsWorldPersistence::SaveInventory(this, TEXT("Container|") + GetPersistenceKey(), Rolled))
                    Items = MoveTemp(Rolled);
            }
        }
    }
    ApplyState();
    OnRemainingSuppliesChanged(RemainingSupplies);
    OnContainerItemsChanged();
}

void ADeadlandsContainer::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ADeadlandsContainer, bOpen);
    DOREPLIFETIME(ADeadlandsContainer, RemainingSupplies);
    DOREPLIFETIME(ADeadlandsContainer, Items);
}

bool ADeadlandsContainer::CanInteract_Implementation(ACharacter* InteractingCharacter) const
{
    return IsValid(InteractingCharacter);
}

void ADeadlandsContainer::Interact_Implementation(ACharacter* InteractingCharacter)
{
    if (!HasAuthority() || !IsValid(InteractingCharacter)) return;

    if (!bOpen)
    {
        bOpen = true;
        ApplyState();
        FDeadlandsWorldPersistence::SaveActorFlag(this, GetPersistenceKey(), TEXT("Open"), bOpen);
        ForceNetUpdate();
        return;
    }

    // Typed containers are managed by the inventory UI. F only opens them.
    if (InitialItems.Num() > 0 || Items.Num() > 0) return;

    ADeadlandsCharacter* Survivor = Cast<ADeadlandsCharacter>(InteractingCharacter);
    // Legacy supplies still work for containers without typed items.
    // Once opened, each F press transfers exactly one supply. When empty,
    // pressing F closes the container.
    if (RemainingSupplies <= 0)
    {
        bOpen = false;
        ApplyState();
        FDeadlandsWorldPersistence::SaveActorFlag(this, GetPersistenceKey(), TEXT("Open"), bOpen);
        ForceNetUpdate();
        return;
    }

    if (!Survivor || Survivor->GetSupplies() >= Survivor->GetSupplyCapacity())
    {
        UE_LOG(LogDeadlands, Log, TEXT("Cannot loot %s: player inventory full or incompatible"), *GetNameSafe(this));
        return;
    }

    // Persist depletion before awarding the item, so a failed disk write
    // cannot generate additional supplies on the next server restart.
    const int32 NewRemaining = RemainingSupplies - 1;
    if (!FDeadlandsWorldPersistence::SaveContainerSupplies(this, GetPersistenceKey(), NewRemaining))
    {
        return;
    }
    if (!Survivor->AddSupplies(1))
    {
        UE_LOG(LogDeadlands, Error, TEXT("Supply transfer failed after save for %s"), *GetNameSafe(this));
        return;
    }
    RemainingSupplies = NewRemaining;
    OnRemainingSuppliesChanged(RemainingSupplies);
    UE_LOG(LogDeadlands, Log, TEXT("Looted %s: %d supplies left"), *GetNameSafe(this), RemainingSupplies);
    ForceNetUpdate();
}

void ADeadlandsContainer::OnRep_Open() { ApplyState(); }
void ADeadlandsContainer::OnRep_Supplies() { OnRemainingSuppliesChanged(RemainingSupplies); }
void ADeadlandsContainer::OnRep_Items() { OnContainerItemsChanged(); }

FString ADeadlandsContainer::GetPersistenceKey() const
{
    return PersistenceId.IsNone() ? GetName() : PersistenceId.ToString();
}

void ADeadlandsContainer::ApplyState()
{
    OnContainerStateChanged(bOpen);
}

bool ADeadlandsContainer::TransferItemsToCharacter(ADeadlandsCharacter* Character, FName ItemId, bool bTake, int32 Quantity)
{
    if (!HasAuthority() || !IsValid(Character) || !bOpen ||
        Quantity < 1 || Quantity > 20 || FDeadlandsItems::MaxStack(ItemId) == 0 ||
        !Character->EnsureItemInventory()) return false;

    TArray<FDeadlandsItemStack>& PlayerItems = Character->GetMutableItems();
    const FString ContainerKey = TEXT("Container|") + GetPersistenceKey();
    const FString PlayerKey = Character->GetInventorySaveKey();
    if (PlayerKey.IsEmpty()) return false;

    bool bAny = false;
    for (int32 Index = 0; Index < Quantity; ++Index)
    {
        const bool bSuccess = bTake
            ? FDeadlandsWorldPersistence::TransferItem(this, ContainerKey, PlayerKey, ItemId,
                Character->GetSupplyCapacity(), Items, PlayerItems)
            : FDeadlandsWorldPersistence::TransferItem(this, PlayerKey, ContainerKey, ItemId,
                1000, PlayerItems, Items);
        if (!bSuccess) break;
        bAny = true;
    }
    if (bAny)
    {
        OnContainerItemsChanged();
        Character->NotifyItemsChanged();
        ForceNetUpdate();
        UE_LOG(LogDeadlands, Log, TEXT("Inventory %s: %s x%d"), bTake ? TEXT("take") : TEXT("put"),
            *ItemId.ToString(), Quantity);
    }
    return bAny;
}

TArray<FDeadlandsItemStack> ADeadlandsContainer::GenerateInitialLoot() const
{
    FRandomStream Random(GetTypeHash(GetPersistenceKey()));
    TArray<FName> Pool;
    switch (LootProfile)
    {
        case EDeadlandsLootProfile::Store:
            Pool = {TEXT("Food"), TEXT("Food"), TEXT("Water"), TEXT("Water"), TEXT("Materials"), TEXT("Cloth"), TEXT("Backpack")};
            break;
        case EDeadlandsLootProfile::Garage:
            Pool = {TEXT("Scrap"), TEXT("Scrap"), TEXT("Materials"), TEXT("Tools"), TEXT("Cloth"), TEXT("Ammo12g")};
            break;
        case EDeadlandsLootProfile::Medical:
            Pool = {TEXT("Bandage"), TEXT("Bandage"), TEXT("Medicine"), TEXT("Antibiotics"), TEXT("Water")};
            break;
        default:
            Pool = {TEXT("Food"), TEXT("Water"), TEXT("Cloth"), TEXT("Bandage"), TEXT("Medicine")};
            break;
    }
    TArray<FDeadlandsItemStack> Result;
    const int32 Rolls = Random.RandRange(2, 6);
    for (int32 Index = 0; Index < Rolls; ++Index)
        FDeadlandsItems::AddOne(Result, Pool[Random.RandRange(0, Pool.Num() - 1)], 1000);
    return Result;
}
