#include "Inventory/DeadlandsInventoryTile.h"
#include "Inventory/DeadlandsInventoryWidget.h"
#include "Inventory/DeadlandsInventoryDrag.h"
#include "Blueprint/DragDropOperation.h"
#include "Inventory/DeadlandsItemStack.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"
#include "InputCoreTypes.h"

void UDeadlandsInventoryTile::NativeOnInitialized()
{
 Super::NativeOnInitialized();
 USizeBox* Size=WidgetTree->ConstructWidget<USizeBox>();
 Size->SetWidthOverride(108.f); Size->SetHeightOverride(96.f);
 WidgetTree->RootWidget=Size;
 TileBorder=WidgetTree->ConstructWidget<UBorder>();
 TileBorder->SetPadding(FMargin(5.f)); Size->SetContent(TileBorder);
 TileText=WidgetTree->ConstructWidget<UTextBlock>();
 TileText->SetJustification(ETextJustify::Center);
 TileText->SetAutoWrapText(true); TileBorder->SetContent(TileText);
 UpdateAppearance();
}
void UDeadlandsInventoryTile::Configure(UDeadlandsInventoryWidget* Owner,bool bContainer,int32 Index,FName Id,int32 Quantity)
{
 InventoryOwner=Owner; bEquipmentTile=false; bContainerTile=bContainer; SlotIndex=Index; ItemId=Id; Count=Quantity;
 if(TileText) TileText->SetText(FText::FromString(Id.IsNone()?TEXT("+"):FString::Printf(TEXT("%s\nx%d"),*Id.ToString(),Count)));
 SetToolTipText(FText::FromString(Id.IsNone()?TEXT("Empty slot"):FString::Printf(TEXT("%s | %s | %d kg each"),*Id.ToString(),FDeadlandsItems::Rarity(Id),FDeadlandsItems::Weight(Id))));
 UpdateAppearance();
}
void UDeadlandsInventoryTile::ConfigureEquipment(UDeadlandsInventoryWidget* Owner,int32 EquipmentSlot,FName EquippedId)
{
 InventoryOwner=Owner; bEquipmentTile=true; bContainerTile=false;
 SlotIndex=EquipmentSlot; ItemId=EquippedId; Count=EquippedId.IsNone()?0:1;
 const TCHAR* Label=EquipmentSlot==0?TEXT("BACK"):EquipmentSlot==1?TEXT("TORSO"):EquipmentSlot==2?TEXT("LEGS"):TEXT("ARMOR");
 const FString Display=EquippedId.IsNone()?TEXT("EMPTY"):EquippedId.ToString();
 if(TileText)TileText->SetText(FText::FromString(FString::Printf(TEXT("%s\\n%s"),Label,*Display)));
 SetToolTipText(FText::FromString(FString::Printf(TEXT("%s slot | %s | Drag compatible clothing here, or drag worn item into inventory"),Label,*Display)));
 UpdateAppearance();
}
void UDeadlandsInventoryTile::SetSelected(bool bSelected){bHighlighted=bSelected;UpdateAppearance();}
void UDeadlandsInventoryTile::UpdateAppearance()
{
 if(!TileBorder)return;
 const FString Rarity=ItemId.IsNone()?TEXT("EMPTY"):FDeadlandsItems::Rarity(ItemId);
 const FLinearColor Base=Rarity==TEXT("RARE")?FLinearColor(.24f,.17f,.32f):Rarity==TEXT("UNCOMMON")?FLinearColor(.13f,.25f,.22f):ItemId.IsNone()?FLinearColor(.07f,.10f,.13f):FLinearColor(.13f,.18f,.23f);
 TileBorder->SetBrushColor(bDragHighlighted?FLinearColor(.30f,.52f,.44f):bHighlighted?FLinearColor(.36f,.47f,.58f):Base);
}
FReply UDeadlandsInventoryTile::NativeOnMouseButtonDown(const FGeometry& Geometry,const FPointerEvent& Event)
{
 if(Event.GetEffectingButton()==EKeys::LeftMouseButton)
 {
  if(InventoryOwner && !bEquipmentTile)InventoryOwner->SelectTile(bContainerTile,SlotIndex);
  if(!ItemId.IsNone())return UWidgetBlueprintLibrary::DetectDragIfPressed(Event,this,EKeys::LeftMouseButton).NativeReply;
  return FReply::Handled();
 }
 return Super::NativeOnMouseButtonDown(Geometry,Event);
}
FReply UDeadlandsInventoryTile::NativeOnMouseButtonDoubleClick(const FGeometry& Geometry,const FPointerEvent& Event)
{
 if(Event.GetEffectingButton()==EKeys::LeftMouseButton&&InventoryOwner&&!ItemId.IsNone())
 {if(bEquipmentTile)InventoryOwner->UnequipToInventory(ItemId);else InventoryOwner->TransferTile(bContainerTile,ItemId,1);return FReply::Handled();}
 return Super::NativeOnMouseButtonDoubleClick(Geometry,Event);
}
void UDeadlandsInventoryTile::NativeOnDragDetected(const FGeometry& Geometry,const FPointerEvent& Event,UDragDropOperation*& Operation)
{
 if(ItemId.IsNone()||!InventoryOwner)return;
 UDeadlandsInventoryDrag* Drag=NewObject<UDeadlandsInventoryDrag>(this);
 Drag->ItemId=ItemId;Drag->bFromContainer=bContainerTile;Drag->SourceIndex=SlotIndex;Drag->bWholeStack=Event.IsShiftDown();Drag->bFromEquipment=bEquipmentTile;
 UDeadlandsInventoryTile* Ghost=CreateWidget<UDeadlandsInventoryTile>(GetOwningPlayer(),UDeadlandsInventoryTile::StaticClass());
 if(Ghost){Ghost->Configure(nullptr,bContainerTile,SlotIndex,ItemId,Count);Ghost->SetVisibility(ESlateVisibility::HitTestInvisible);Drag->DefaultDragVisual=Ghost;}
 Operation=Drag;
}
bool UDeadlandsInventoryTile::NativeOnDrop(const FGeometry& Geometry,const FDragDropEvent& Event,UDragDropOperation* Operation)
{
 bDragHighlighted=false;UpdateAppearance();
 const UDeadlandsInventoryDrag* Drag=Cast<UDeadlandsInventoryDrag>(Operation);
 if(!Drag||!InventoryOwner)return false;
 if(bEquipmentTile)return InventoryOwner->DropOnEquipment(Drag->ItemId,Drag->bFromContainer,Drag->bFromEquipment,SlotIndex);
 if(Drag->bFromEquipment)return !bContainerTile && InventoryOwner->UnequipToInventory(Drag->ItemId);
 return InventoryOwner->DropTile(Drag->bFromContainer,Drag->SourceIndex,Drag->ItemId,Drag->bWholeStack,bContainerTile,SlotIndex);
}
void UDeadlandsInventoryTile::NativeOnDragEnter(const FGeometry& Geometry,const FDragDropEvent& Event,UDragDropOperation* Operation)
{Super::NativeOnDragEnter(Geometry,Event,Operation);bDragHighlighted=Cast<UDeadlandsInventoryDrag>(Operation)!=nullptr;UpdateAppearance();}
void UDeadlandsInventoryTile::NativeOnDragLeave(const FDragDropEvent& Event,UDragDropOperation* Operation)
{Super::NativeOnDragLeave(Event,Operation);bDragHighlighted=false;UpdateAppearance();}
