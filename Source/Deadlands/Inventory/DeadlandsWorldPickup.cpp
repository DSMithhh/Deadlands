#include "Inventory/DeadlandsWorldPickup.h"
#include "DeadlandsCharacter.h"
#include "Inventory/DeadlandsItemStack.h"
#include "Persistence/DeadlandsWorldPersistence.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Net/UnrealNetwork.h"
ADeadlandsWorldPickup::ADeadlandsWorldPickup()
{
    bReplicates = true;
    SetReplicateMovement(true);
    Collision = CreateDefaultSubobject<USphereComponent>(TEXT("PickupCollision"));
    SetRootComponent(Collision);
    Collision->InitSphereRadius(30.f);
    Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
    Collision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
    Mesh->SetupAttachment(Collision);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (Sphere.Succeeded()) Mesh->SetStaticMesh(Sphere.Object);
    Mesh->SetRelativeScale3D(FVector(0.35f));
}
void ADeadlandsWorldPickup::InitializePickup(FName NewItemId)
{
    if (HasAuthority() && FDeadlandsItems::MaxStack(NewItemId) > 0) ItemId = NewItemId;
}
bool ADeadlandsWorldPickup::CanInteract_Implementation(ACharacter* Character) const
{
    return ItemId != NAME_None && Cast<ADeadlandsCharacter>(Character) != nullptr;
}
void ADeadlandsWorldPickup::Interact_Implementation(ACharacter* Character)
{
    if (!HasAuthority()) return;
    ADeadlandsCharacter* Survivor = Cast<ADeadlandsCharacter>(Character);
    if (!Survivor || !Survivor->EnsureItemInventory()) return;
    TArray<FDeadlandsItemStack> NewItems = Survivor->GetItems();
    if (!FDeadlandsItems::AddOne(NewItems, ItemId, Survivor->GetSupplyCapacity())) return;
    if (!FDeadlandsWorldPersistence::SaveInventory(Survivor, Survivor->GetInventorySaveKey(), NewItems)) return;
    Survivor->GetMutableItems() = MoveTemp(NewItems);
    Survivor->NotifyItemsChanged();
    Destroy();
}
void ADeadlandsWorldPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ADeadlandsWorldPickup, ItemId);
}
