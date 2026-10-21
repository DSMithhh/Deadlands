#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DeadlandsInventoryTile.generated.h"

class UDeadlandsInventoryWidget;
class UBorder;
class UTextBlock;

UCLASS()
class DEADLANDS_API UDeadlandsInventoryTile : public UUserWidget
{
    GENERATED_BODY()
public:
    void Configure(UDeadlandsInventoryWidget* Owner, bool bContainer, int32 Index, FName Id, int32 Quantity);
    void SetSelected(bool bSelected);
protected:
    virtual void NativeOnInitialized() override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;
    virtual FReply NativeOnMouseButtonDoubleClick(const FGeometry& Geometry, const FPointerEvent& Event) override;
    virtual void NativeOnDragDetected(const FGeometry& Geometry, const FPointerEvent& Event, UDragDropOperation*& Operation) override;
    virtual bool NativeOnDrop(const FGeometry& Geometry, const FDragDropEvent& Event, UDragDropOperation* Operation) override;
    virtual void NativeOnDragEnter(const FGeometry& Geometry, const FDragDropEvent& Event, UDragDropOperation* Operation) override;
    virtual void NativeOnDragLeave(const FDragDropEvent& Event, UDragDropOperation* Operation) override;
private:
    UPROPERTY() TObjectPtr<UDeadlandsInventoryWidget> InventoryOwner;
    UPROPERTY() TObjectPtr<UBorder> TileBorder;
    UPROPERTY() TObjectPtr<UTextBlock> TileText;
    FName ItemId = NAME_None;
    int32 SlotIndex = INDEX_NONE;
    int32 Count = 0;
    bool bContainerTile = false;
    bool bHighlighted = false;
    bool bDragHighlighted = false;
    void UpdateAppearance();
};
