#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DeadlandsInventoryWidget.generated.h"

class ADeadlandsCharacter;
class ADeadlandsContainer;
class UTextBlock;
class UComboBoxString;
class UButton;
class UUniformGridPanel;

/** Minimal runtime-created inventory UI. No Blueprint widget asset required. */
UCLASS()
class DEADLANDS_API UDeadlandsInventoryWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void SetInventoryContext(ADeadlandsCharacter* Character, ADeadlandsContainer* Container);
    virtual void NativeOnInitialized() override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    UPROPERTY() TObjectPtr<ADeadlandsCharacter> Survivor;
    UPROPERTY() TObjectPtr<ADeadlandsContainer> LootContainer;
    UPROPERTY() TObjectPtr<UTextBlock> Header;
    UPROPERTY() TObjectPtr<UTextBlock> PlayerList;
    UPROPERTY() TObjectPtr<UTextBlock> ContainerList;
    UPROPERTY() TObjectPtr<UComboBoxString> ItemPicker;
    UPROPERTY() TObjectPtr<UButton> TakeButton;
    UPROPERTY() TObjectPtr<UButton> PutButton;
    UPROPERTY() TObjectPtr<UUniformGridPanel> PlayerGrid;
    UPROPERTY() TObjectPtr<UUniformGridPanel> LootGrid;
    UPROPERTY() TObjectPtr<UTextBlock> ItemDetails;
    FName SelectedId = NAME_None;
    bool bSelectedFromContainer = false;
    FString GridSignature;
    float RefreshCountdown = 0.f;
    void SelectIndex(int32 Index);
    UFUNCTION() void SelectSlot0();
    UFUNCTION() void SelectSlot1();
    UFUNCTION() void SelectSlot2();
    UFUNCTION() void SelectSlot3();
    UFUNCTION() void SelectSlot4();
    UFUNCTION() void SelectSlot5();
    UFUNCTION() void SelectSlot6();
    UFUNCTION() void SelectSlot7();
    UFUNCTION() void SelectSlot8();
    UFUNCTION() void SelectSlot9();
    UFUNCTION() void SelectSlot10();
    UFUNCTION() void SelectSlot11();
    UFUNCTION() void SelectSlot12();
    UFUNCTION() void SelectSlot13();
    UFUNCTION() void SelectSlot14();
    UFUNCTION() void SelectSlot15();
    void Refresh();
    UFUNCTION() void TakeOne();
    UFUNCTION() void PutOne();
    UFUNCTION() void TakeStack();
    UFUNCTION() void PutStack();
    UFUNCTION() void ConsumeSelected();
    UFUNCTION() void DropSelected();
    UFUNCTION() void EquipBackpack();
    UFUNCTION() void CraftBandage();
    UFUNCTION() void CraftTools();
    UFUNCTION() void CraftMedicine();
    UFUNCTION() void RepairTools();
};
