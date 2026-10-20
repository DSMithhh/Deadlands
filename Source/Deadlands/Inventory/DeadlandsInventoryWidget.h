#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DeadlandsInventoryWidget.generated.h"

class ADeadlandsCharacter;
class ADeadlandsContainer;
class UTextBlock;
class UComboBoxString;
class UButton;

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
    float RefreshCountdown = 0.f;
    void Refresh();
    UFUNCTION() void TakeOne();
    UFUNCTION() void PutOne();
    UFUNCTION() void TakeStack();
    UFUNCTION() void PutStack();
    UFUNCTION() void ConsumeSelected();
    UFUNCTION() void DropSelected();
};
