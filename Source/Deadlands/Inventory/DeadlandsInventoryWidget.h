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
    UPROPERTY() TObjectPtr<UTextBlock> QuickbarLabel;
    TArray<FName> QuickSlots;
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
    UFUNCTION() void SelectSlot16();
    UFUNCTION() void SelectSlot17();
    UFUNCTION() void SelectSlot18();
    UFUNCTION() void SelectSlot19();
    UFUNCTION() void SelectSlot20();
    UFUNCTION() void SelectSlot21();
    UFUNCTION() void SelectSlot22();
    UFUNCTION() void SelectSlot23();
    UFUNCTION() void SelectSlot24();
    UFUNCTION() void SelectSlot25();
    UFUNCTION() void SelectSlot26();
    UFUNCTION() void SelectSlot27();
    UFUNCTION() void SelectSlot28();
    UFUNCTION() void SelectSlot29();
    UFUNCTION() void SelectSlot30();
    UFUNCTION() void SelectSlot31();
    void Refresh();
    UFUNCTION() void TakeOne();
    UFUNCTION() void PutOne();
    UFUNCTION() void TakeFive();
    UFUNCTION() void PutFive();
    UFUNCTION() void TakeStack();
    UFUNCTION() void PutStack();
    UFUNCTION() void ConsumeSelected();
    UFUNCTION() void DropSelected();
    UFUNCTION() void EquipBackpack();
    UFUNCTION() void CraftBandage();
    UFUNCTION() void CraftTools();
    UFUNCTION() void CraftMedicine();
    UFUNCTION() void RepairTools();
    UFUNCTION() void AssignQuickSlot1();
    UFUNCTION() void AssignQuickSlot2();
    UFUNCTION() void AssignQuickSlot3();
    UFUNCTION() void AssignQuickSlot4();
    UFUNCTION() void AssignQuickSlot5();
    UFUNCTION() void UseQuickSlot1();
    UFUNCTION() void UseQuickSlot2();
    UFUNCTION() void UseQuickSlot3();
    UFUNCTION() void UseQuickSlot4();
    UFUNCTION() void UseQuickSlot5();
    void AssignQuickSlot(int32 Slot);
    void UseQuickSlot(int32 Slot);
};
