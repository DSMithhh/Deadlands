#include "Inventory/DeadlandsInventoryWidget.h"
#include "DeadlandsCharacter.h"
#include "Interaction/DeadlandsContainer.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/TextBlock.h"
#include "Components/ComboBoxString.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"

void UDeadlandsInventoryWidget::SetInventoryContext(ADeadlandsCharacter* Character, ADeadlandsContainer* Container)
{
    Survivor = Character;
    LootContainer = Container;
    Refresh();
}

void UDeadlandsInventoryWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    SetIsFocusable(true);
    UBorder* Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
    WidgetTree->RootWidget = Canvas;
    UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(Border);
    PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
    PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
    PanelSlot->SetAutoSize(true);
    Border->SetPadding(FMargin(24.f));
    Border->SetBrushColor(FLinearColor(0.025f, 0.035f, 0.045f, 0.96f));
    UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
    Border->SetContent(Column);
    auto MakeText = [&](const FString& Value) -> UTextBlock*
    {
        UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
        Label->SetText(FText::FromString(Value));
        Label->SetColorAndOpacity(FSlateColor(FLinearColor::White));
        Column->AddChildToVerticalBox(Label);
        return Label;
    };
    Header = MakeText(TEXT("DEADLANDS  |  INVENTORY  [TAB to close]"));
    PlayerList = MakeText(TEXT("PLAYER"));
    ContainerList = MakeText(TEXT("CONTAINER"));
    ItemPicker = WidgetTree->ConstructWidget<UComboBoxString>();
    for (const TCHAR* Id : { TEXT("Food"), TEXT("Water"), TEXT("Medicine"), TEXT("Tools"), TEXT("Materials") })
        ItemPicker->AddOption(FString(Id));
    ItemPicker->SetSelectedOption(TEXT("Food"));
    Column->AddChildToVerticalBox(ItemPicker);
    UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>();
    Column->AddChildToVerticalBox(Buttons);
    TakeButton = WidgetTree->ConstructWidget<UButton>();
    PutButton = WidgetTree->ConstructWidget<UButton>();
    Buttons->AddChildToHorizontalBox(TakeButton);
    Buttons->AddChildToHorizontalBox(PutButton);
    UTextBlock* TakeLabel = WidgetTree->ConstructWidget<UTextBlock>();
    TakeLabel->SetText(FText::FromString(TEXT(" TAKE ONE ")));
    TakeButton->AddChild(TakeLabel);
    UTextBlock* PutLabel = WidgetTree->ConstructWidget<UTextBlock>();
    PutLabel->SetText(FText::FromString(TEXT(" PUT ONE ")));
    PutButton->AddChild(PutLabel);
    TakeButton->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::TakeOne);
    PutButton->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::PutOne);
    UButton* TakeStackButton = WidgetTree->ConstructWidget<UButton>();
    UButton* PutStackButton = WidgetTree->ConstructWidget<UButton>();
    Column->AddChildToVerticalBox(TakeStackButton);
    Column->AddChildToVerticalBox(PutStackButton);
    UTextBlock* TakeStackLabel = WidgetTree->ConstructWidget<UTextBlock>();
    TakeStackLabel->SetText(FText::FromString(TEXT(" TAKE STACK ")));
    TakeStackButton->AddChild(TakeStackLabel);
    UTextBlock* PutStackLabel = WidgetTree->ConstructWidget<UTextBlock>();
    PutStackLabel->SetText(FText::FromString(TEXT(" PUT STACK ")));
    PutStackButton->AddChild(PutStackLabel);
    TakeStackButton->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::TakeStack);
    PutStackButton->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::PutStack);
    UButton* ConsumeButton = WidgetTree->ConstructWidget<UButton>();
    UTextBlock* ConsumeLabel = WidgetTree->ConstructWidget<UTextBlock>();
    ConsumeLabel->SetText(FText::FromString(TEXT(" EAT / DRINK SELECTED ")));
    ConsumeButton->AddChild(ConsumeLabel);
    Column->AddChildToVerticalBox(ConsumeButton);
    ConsumeButton->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::ConsumeSelected);
    UButton* DropButton = WidgetTree->ConstructWidget<UButton>();
    UTextBlock* DropLabel = WidgetTree->ConstructWidget<UTextBlock>();
    DropLabel->SetText(FText::FromString(TEXT(" DROP ONE SELECTED ")));
    DropButton->AddChild(DropLabel);
    Column->AddChildToVerticalBox(DropButton);
    DropButton->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::DropSelected);
    Refresh();
}

void UDeadlandsInventoryWidget::NativeTick(const FGeometry& Geometry, float Delta)
{
    Super::NativeTick(Geometry, Delta);
    RefreshCountdown -= Delta;
    if (RefreshCountdown <= 0.f)
    {
        RefreshCountdown = 0.2f;
        Refresh();
    }
}

void UDeadlandsInventoryWidget::Refresh()
{
    if (!PlayerList || !ContainerList) return;
    auto Format = [](const TArray<FDeadlandsItemStack>& Stacks) -> FString
    {
        if (Stacks.IsEmpty()) return TEXT("(empty)");
        FString Result;
        for (const auto& Stack : Stacks)
            Result += FString::Printf(TEXT("\n%s x%d"), *Stack.ItemId.ToString(), Stack.Quantity);
        return Result;
    };
    if (Survivor)
        PlayerList->SetText(FText::FromString(FString::Printf(TEXT("YOUR ITEMS (%d/%d):%s"),
            FDeadlandsItems::Total(Survivor->GetItems()), Survivor->GetSupplyCapacity(), *Format(Survivor->GetItems()))));
    ContainerList->SetText(FText::FromString(LootContainer
        ? TEXT("CONTAINER:") + Format(LootContainer->GetItems())
        : TEXT("CONTAINER: (look at a container and press F)")));
    if (TakeButton) TakeButton->SetIsEnabled(IsValid(LootContainer));
    if (PutButton) PutButton->SetIsEnabled(IsValid(LootContainer));
}

void UDeadlandsInventoryWidget::TakeOne()
{
    if (Survivor && LootContainer && ItemPicker)
        Survivor->RequestItemTransfer(LootContainer, FName(*ItemPicker->GetSelectedOption()), true, 1);
}

void UDeadlandsInventoryWidget::PutOne()
{
    if (Survivor && LootContainer && ItemPicker)
        Survivor->RequestItemTransfer(LootContainer, FName(*ItemPicker->GetSelectedOption()), false, 1);
}

void UDeadlandsInventoryWidget::TakeStack()
{
    if (Survivor && LootContainer && ItemPicker)
    {
        const FName Id(*ItemPicker->GetSelectedOption());
        Survivor->RequestItemTransfer(LootContainer, Id, true, FDeadlandsItems::MaxStack(Id));
    }
}

void UDeadlandsInventoryWidget::PutStack()
{
    if (Survivor && LootContainer && ItemPicker)
    {
        const FName Id(*ItemPicker->GetSelectedOption());
        Survivor->RequestItemTransfer(LootContainer, Id, false, FDeadlandsItems::MaxStack(Id));
    }
}

FReply UDeadlandsInventoryWidget::NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
    if (Event.GetKey() == EKeys::Tab || Event.GetKey() == EKeys::Escape)
    {
        if (Survivor) Survivor->CloseInventory();
        return FReply::Handled();
    }
    return Super::NativeOnKeyDown(Geometry, Event);
}

FReply UDeadlandsInventoryWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
    // Preview runs before focused child buttons/combo boxes can consume the key.
    if (Event.GetKey() == EKeys::Tab || Event.GetKey() == EKeys::Escape)
    {
        if (Survivor) Survivor->CloseInventory();
        return FReply::Handled();
    }
    return Super::NativeOnPreviewKeyDown(Geometry, Event);
}

void UDeadlandsInventoryWidget::ConsumeSelected()
{
    if (Survivor && ItemPicker)
        Survivor->RequestConsumeItem(FName(*ItemPicker->GetSelectedOption()));
}
void UDeadlandsInventoryWidget::DropSelected()
{
    if (Survivor && ItemPicker)
        Survivor->RequestDropItem(FName(*ItemPicker->GetSelectedOption()));
}
