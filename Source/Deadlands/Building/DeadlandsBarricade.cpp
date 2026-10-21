#include "Building/DeadlandsBarricade.h"
#include "DeadlandsCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Net/UnrealNetwork.h"
#include "Persistence/DeadlandsWorldPersistence.h"
#include "Inventory/DeadlandsItemStack.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"
#include "Engine/DamageEvents.h"

ADeadlandsBarricade::ADeadlandsBarricade()
{
 bReplicates=true; SetReplicateMovement(true);
 Mesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BarricadeMesh"));
 SetRootComponent(Mesh);
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
 if(Cube.Succeeded())Mesh->SetStaticMesh(Cube.Object);
 Mesh->SetRelativeScale3D(FVector(.18f,2.1f,1.45f));
 Mesh->SetCollisionProfileName(TEXT("BlockAll"));
 Mesh->SetCanEverAffectNavigation(true);
}
void ADeadlandsBarricade::SetPieceType(EDeadlandsBuildPiece NewType,bool bOpen)
{
 PieceType=NewType;bDoorOpen=bOpen;
 if(!Mesh)return;
 FVector Scale(.18f,2.1f,1.45f);
 if(PieceType==EDeadlandsBuildPiece::Foundation)Scale=FVector(2.2f,2.2f,.18f);
 else if(PieceType==EDeadlandsBuildPiece::Wall)Scale=FVector(.18f,2.2f,2.3f);
 else if(PieceType==EDeadlandsBuildPiece::Door)Scale=FVector(.16f,1.05f,2.2f);
 else if(PieceType==EDeadlandsBuildPiece::Storage)Scale=FVector(.95f,.7f,.7f);
 Mesh->SetRelativeScale3D(Scale);
 Mesh->SetCollisionEnabled(PieceType==EDeadlandsBuildPiece::Door&&bDoorOpen?ECollisionEnabled::NoCollision:ECollisionEnabled::QueryAndPhysics);
}
void ADeadlandsBarricade::BeginPlay(){Super::BeginPlay();RefreshVisual();}
void ADeadlandsBarricade::OnRep_Health(){SetPieceType(PieceType,bDoorOpen);RefreshVisual();}
void ADeadlandsBarricade::RefreshVisual()
{
 if(Mesh)Mesh->SetCustomDepthStencilValue(Health<MaxHealth*.35f?2:1);
}
void ADeadlandsBarricade::InitializeSaved(const FString& Id,float SavedHealth,EDeadlandsBuildPiece SavedType,bool bSavedOpen)
{
 if(!HasAuthority())return;
 PersistentId=Id;Health=FMath::Clamp(SavedHealth,1.f,MaxHealth);SetPieceType(SavedType,bSavedOpen);RefreshVisual();ForceNetUpdate();
}
bool ADeadlandsBarricade::CanInteract_Implementation(ACharacter* Character) const
{
 return Health>0.f&&Cast<ADeadlandsCharacter>(Character)!=nullptr&&
 (PieceType==EDeadlandsBuildPiece::Door||Health<MaxHealth);
}
void ADeadlandsBarricade::Interact_Implementation(ACharacter* Character)
{
 if(!HasAuthority())return;
 if(PieceType==EDeadlandsBuildPiece::Door&&!PersistentId.IsEmpty())
 {
  const bool NewOpen=!bDoorOpen;
  if(FDeadlandsWorldPersistence::SaveBarricade(this,PersistentId,GetActorTransform(),Health,static_cast<uint8>(PieceType),NewOpen))
  {bDoorOpen=NewOpen;OnRep_Health();ForceNetUpdate();}
 }
 else Repair(Cast<ADeadlandsCharacter>(Character));
}
bool ADeadlandsBarricade::Repair(ADeadlandsCharacter* Character)
{
 if(!HasAuthority()||!IsValid(Character)||Health>=MaxHealth||PersistentId.IsEmpty()||!Character->EnsureItemInventory())return false;
 TArray<FDeadlandsItemStack> Updated=Character->GetItems();
 if(!FDeadlandsItems::RemoveOne(Updated,TEXT("Materials")))return false;
 const float NewHealth=FMath::Min(MaxHealth,Health+75.f);
 if(!FDeadlandsWorldPersistence::SaveBarricade(this,PersistentId,GetActorTransform(),NewHealth,static_cast<uint8>(PieceType),bDoorOpen))return false;
 if(!FDeadlandsWorldPersistence::SaveInventory(Character,Character->GetInventorySaveKey(),Updated))return false;
 Character->GetMutableItems()=MoveTemp(Updated);Character->NotifyItemsChanged();
 Health=NewHealth;OnRep_Health();ForceNetUpdate();return true;
}
bool ADeadlandsBarricade::Dismantle(ADeadlandsCharacter* Character)
{
 if(!HasAuthority()||!IsValid(Character)||PersistentId.IsEmpty()||!Character->EnsureItemInventory())return false;
 TArray<FDeadlandsItemStack> Updated=Character->GetItems();
 if(!FDeadlandsItems::AddOne(Updated,TEXT("Materials"),Character->GetSupplyCapacity()))return false;
 if(!FDeadlandsWorldPersistence::SaveInventory(Character,Character->GetInventorySaveKey(),Updated))return false;
 if(!FDeadlandsWorldPersistence::RemoveBarricade(this,PersistentId))
 {
  FDeadlandsWorldPersistence::SaveInventory(Character,Character->GetInventorySaveKey(),Character->GetItems());
  return false;
 }
 Character->GetMutableItems()=MoveTemp(Updated);Character->NotifyItemsChanged();
 Destroy();return true;
}
float ADeadlandsBarricade::TakeDamage(float DamageAmount,const FDamageEvent& DamageEvent,AController* EventInstigator,AActor* Causer)
{
 if(!HasAuthority()||DamageAmount<=0.f)return 0.f;
 const float Applied=FMath::Min(Health,DamageAmount);
 const float NewHealth=Health-Applied;
 if(NewHealth<=0.f)
 {
  if(!PersistentId.IsEmpty()&&!FDeadlandsWorldPersistence::RemoveBarricade(this,PersistentId))return 0.f;
  Health=0.f;Destroy();
 }
 else
 {
  if(!PersistentId.IsEmpty()&&!FDeadlandsWorldPersistence::SaveBarricade(this,PersistentId,GetActorTransform(),NewHealth,static_cast<uint8>(PieceType),bDoorOpen))return 0.f;
  Health=NewHealth;OnRep_Health();ForceNetUpdate();
 }
 return Applied;
}
void ADeadlandsBarricade::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
 Super::GetLifetimeReplicatedProps(OutLifetimeProps);
 DOREPLIFETIME(ADeadlandsBarricade,Health);
 DOREPLIFETIME(ADeadlandsBarricade,PersistentId);
 DOREPLIFETIME(ADeadlandsBarricade,PieceType);
 DOREPLIFETIME(ADeadlandsBarricade,bDoorOpen);
}
