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
#include "Components/UniformGridPanel.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"

void UDeadlandsInventoryWidget::SetInventoryContext(ADeadlandsCharacter* Character, ADeadlandsContainer* Container)
{
    Survivor = Character;
    LootContainer = Container;
    SelectedId = NAME_None;
    bSelectedFromContainer = false;
    GridSignature.Empty();
    Refresh();
}

void UDeadlandsInventoryWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    SetIsFocusable(true);
    QuickSlots.SetNum(5);
    UBorder* Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
    WidgetTree->RootWidget = Canvas;
    UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(Border);
    PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
    PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
    PanelSlot->SetAutoSize(true);
    Border->SetPadding(FMargin(18.f));
    Border->SetBrushColor(FLinearColor(0.035f, 0.055f, 0.075f, 0.985f));
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
    Header = MakeText(TEXT("INVENTORY     /     TAB TO CLOSE"));
    Header->SetColorAndOpacity(FSlateColor(FLinearColor(0.86f, 0.72f, 0.43f)));
    PlayerList = MakeText(TEXT("SURVIVOR"));
    ContainerList = MakeText(TEXT("NEARBY LOOT"));
    ItemPicker = WidgetTree->ConstructWidget<UComboBoxString>();
    for (const TCHAR* Id : { TEXT("Food"), TEXT("Water"), TEXT("Medicine"), TEXT("Tools"), TEXT("Materials"), TEXT("Bandage"), TEXT("Antibiotics"),
                              TEXT("Ammo9mm"), TEXT("Ammo12g"), TEXT("Scrap"),
                              TEXT("Cloth"), TEXT("Backpack"), TEXT("Pistol") })
        ItemPicker->AddOption(FString(Id));
    ItemPicker->SetSelectedOption(TEXT("Food"));
    ItemPicker->SetVisibility(ESlateVisibility::Collapsed);
    Column->AddChildToVerticalBox(ItemPicker);
    UHorizontalBox* InventoryPanels = WidgetTree->ConstructWidget<UHorizontalBox>();
    Column->AddChildToVerticalBox(InventoryPanels);
    UVerticalBox* PlayerPanel = WidgetTree->ConstructWidget<UVerticalBox>();
    UVerticalBox* LootPanel = WidgetTree->ConstructWidget<UVerticalBox>();
    UHorizontalBoxSlot* LeftSlot = InventoryPanels->AddChildToHorizontalBox(PlayerPanel);
    UHorizontalBoxSlot* RightSlot = InventoryPanels->AddChildToHorizontalBox(LootPanel);
    LeftSlot->SetPadding(FMargin(0.f, 10.f, 12.f, 10.f));
    RightSlot->SetPadding(FMargin(12.f, 10.f, 0.f, 10.f));
    UTextBlock* LeftTitle = WidgetTree->ConstructWidget<UTextBlock>();
    UTextBlock* RightTitle = WidgetTree->ConstructWidget<UTextBlock>();
    LeftTitle->SetText(FText::FromString(TEXT("SURVIVOR  /  INVENTORY")));
    RightTitle->SetText(FText::FromString(TEXT("SEARCHING  /  CONTAINER")));
    LeftTitle->SetColorAndOpacity(FSlateColor(FLinearColor(0.75f, 0.85f, 0.92f)));
    RightTitle->SetColorAndOpacity(FSlateColor(FLinearColor(0.75f, 0.85f, 0.92f)));
    PlayerPanel->AddChildToVerticalBox(LeftTitle);
    LootPanel->AddChildToVerticalBox(RightTitle);
    USizeBox* PlayerFrame = WidgetTree->ConstructWidget<USizeBox>();
    USizeBox* LootFrame = WidgetTree->ConstructWidget<USizeBox>();
    PlayerFrame->SetWidthOverride(360.f);
    LootFrame->SetWidthOverride(360.f);
    PlayerFrame->SetHeightOverride(420.f);
    LootFrame->SetHeightOverride(420.f);
    PlayerPanel->AddChildToVerticalBox(PlayerFrame);
    LootPanel->AddChildToVerticalBox(LootFrame);
    UScrollBox* PlayerScroll = WidgetTree->ConstructWidget<UScrollBox>();
    UScrollBox* LootScroll = WidgetTree->ConstructWidget<UScrollBox>();
    PlayerFrame->SetContent(PlayerScroll);
    LootFrame->SetContent(LootScroll);
    PlayerGrid = WidgetTree->ConstructWidget<UUniformGridPanel>();
    LootGrid = WidgetTree->ConstructWidget<UUniformGridPanel>();
    PlayerScroll->AddChild(PlayerGrid);
    LootScroll->AddChild(LootGrid);
    ItemDetails = MakeText(TEXT("SELECT AN ITEM TO VIEW DETAILS"));
    QuickbarLabel = MakeText(TEXT("QUICK ACCESS  1-5"));
    ItemDetails->SetColorAndOpacity(FSlateColor(FLinearColor(0.72f, 0.84f, 0.90f)));
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
    UButton* TakeFiveButton = WidgetTree->ConstructWidget<UButton>();
    UTextBlock* TakeFiveLabel = WidgetTree->ConstructWidget<UTextBlock>();
    TakeFiveLabel->SetText(FText::FromString(TEXT(" TAKE 5 ")));
    TakeFiveButton->AddChild(TakeFiveLabel);
    Buttons->AddChildToHorizontalBox(TakeFiveButton);
    TakeFiveButton->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::TakeFive);
    UButton* PutFiveButton = WidgetTree->ConstructWidget<UButton>();
    UTextBlock* PutFiveLabel = WidgetTree->ConstructWidget<UTextBlock>();
    PutFiveLabel->SetText(FText::FromString(TEXT(" PUT 5 ")));
    PutFiveButton->AddChild(PutFiveLabel);
    Buttons->AddChildToHorizontalBox(PutFiveButton);
    PutFiveButton->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::PutFive);
    PutButton->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::PutOne);
    UButton* TakeStackButton = WidgetTree->ConstructWidget<UButton>();
    UButton* PutStackButton = WidgetTree->ConstructWidget<UButton>();
    Buttons->AddChildToHorizontalBox(TakeStackButton);
    Buttons->AddChildToHorizontalBox(PutStackButton);
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
    ConsumeLabel->SetText(FText::FromString(TEXT(" USE ")));
    ConsumeButton->AddChild(ConsumeLabel);
    Buttons->AddChildToHorizontalBox(ConsumeButton);
    ConsumeButton->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::ConsumeSelected);
    UButton* DropButton = WidgetTree->ConstructWidget<UButton>();
    UTextBlock* DropLabel = WidgetTree->ConstructWidget<UTextBlock>();
    DropLabel->SetText(FText::FromString(TEXT(" DROP ")));
    DropButton->AddChild(DropLabel);
    Buttons->AddChildToHorizontalBox(DropButton);
    DropButton->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::DropSelected);
    UButton* CraftToggle = WidgetTree->ConstructWidget<UButton>();
    UTextBlock* CraftToggleText = WidgetTree->ConstructWidget<UTextBlock>();
    CraftToggleText->SetText(FText::FromString(TEXT(" CRAFTING / REPAIR  [EXPAND] ")));
    CraftToggle->AddChild(CraftToggleText);
    Column->AddChildToVerticalBox(CraftToggle);
    CraftToggle->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::ToggleCraftActions);
    CraftActions = WidgetTree->ConstructWidget<UHorizontalBox>();
    Column->AddChildToVerticalBox(CraftActions);
    CraftActions->SetVisibility(ESlateVisibility::Collapsed);
    auto AddAction = [&](const TCHAR* Title)
    {
        UButton* Action = WidgetTree->ConstructWidget<UButton>();
        UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
        Label->SetText(FText::FromString(Title));
        Action->AddChild(Label);
        CraftActions->AddChildToHorizontalBox(Action);
        return Action;
    };
    UButton* BackpackButton = AddAction(TEXT(" EQUIP / UNEQUIP BACKPACK "));
    BackpackButton->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::EquipBackpack);
    UButton* BandageButton = AddAction(TEXT(" CRAFT BANDAGE (2 CLOTH) "));
    BandageButton->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::CraftBandage);
    UButton* ToolsButton = AddAction(TEXT(" CRAFT TOOLS (3 SCRAP + 2 MATERIALS) "));
    ToolsButton->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::CraftTools);
    UButton* MedicineButton = AddAction(TEXT(" CRAFT MEDICINE (2 BANDAGES + ANTIBIOTICS) "));
    MedicineButton->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::CraftMedicine);
    UButton* RepairButton = AddAction(TEXT(" REPAIR TOOLS (SCRAP + MATERIALS) "));
    RepairButton->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::RepairTools);
    UHorizontalBox* QuickAssign = WidgetTree->ConstructWidget<UHorizontalBox>();
    UHorizontalBox* QuickUse = WidgetTree->ConstructWidget<UHorizontalBox>();
    Column->AddChildToVerticalBox(QuickAssign);
    Column->AddChildToVerticalBox(QuickUse);
    for (int32 I = 0; I < 5; ++I)
    {
        UButton* Assign = WidgetTree->ConstructWidget<UButton>();
        UButton* Use = WidgetTree->ConstructWidget<UButton>();
        UTextBlock* AssignText = WidgetTree->ConstructWidget<UTextBlock>();
        UTextBlock* UseText = WidgetTree->ConstructWidget<UTextBlock>();
        AssignText->SetText(FText::FromString(FString::Printf(TEXT(" SET %d "), I + 1)));
        UseText->SetText(FText::FromString(FString::Printf(TEXT(" USE %d "), I + 1)));
        Assign->AddChild(AssignText);
        Use->AddChild(UseText);
        QuickAssign->AddChildToHorizontalBox(Assign);
        QuickUse->AddChildToHorizontalBox(Use);
        switch (I)
        {
            case 0: Assign->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::AssignQuickSlot1); Use->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::UseQuickSlot1); break;
            case 1: Assign->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::AssignQuickSlot2); Use->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::UseQuickSlot2); break;
            case 2: Assign->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::AssignQuickSlot3); Use->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::UseQuickSlot3); break;
            case 3: Assign->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::AssignQuickSlot4); Use->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::UseQuickSlot4); break;
            case 4: Assign->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::AssignQuickSlot5); Use->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::UseQuickSlot5); break;
        }
    }
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
    if (Survivor)
        PlayerList->SetText(FText::FromString(FString::Printf(TEXT("BACKPACK   %d/%d KG     |     EQUIPPED %s     |     BLEED %.0f     |     INFECTION %.0f     |     TOOLS %d%%"),
            FDeadlandsItems::TotalWeight(Survivor->GetItems()), Survivor->GetSupplyCapacity(), Survivor->HasEquippedBackpack() ? TEXT("ON") : TEXT("OFF"), Survivor->GetBleeding(), Survivor->GetInfection(), Survivor->GetToolDurability())));
    ContainerList->SetText(FText::FromString(LootContainer
        ? FString::Printf(TEXT("SEARCHING CONTAINER    /    %d ITEMS"), FDeadlandsItems::Total(LootContainer->GetItems()))
        : TEXT("NO CONTAINER OPEN")));
    // Rebuild only when inventory contents change; do not steal focus every tick.
    FString Signature = IsValid(LootContainer) ? TEXT("LOOT") : TEXT("SOLO");
    auto AppendSignature = [&](const TArray<FDeadlandsItemStack>& Stacks)
    {
        for (const auto& Stack : Stacks)
            Signature += FString::Printf(TEXT("|%s:%d"), *Stack.ItemId.ToString(), Stack.Quantity);
        Signature += TEXT(";");
    };
    if (Survivor) AppendSignature(Survivor->GetItems());
    if (LootContainer) AppendSignature(LootContainer->GetItems());
    if (Signature != GridSignature && PlayerGrid && LootGrid)
    {
        GridSignature = Signature;
        PlayerGrid->ClearChildren();
        LootGrid->ClearChildren();
        int32 Index = 0;
        auto BuildGrid = [&](UUniformGridPanel* Grid, const TArray<FDeadlandsItemStack>& Stacks)
        {
            for (const auto& Stack : Stacks)
            {
                if (Index >= (Grid == PlayerGrid ? 16 : 32)) break;
                UButton* Tile = WidgetTree->ConstructWidget<UButton>();
                UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>();
                Text->SetText(FText::FromString(FString::Printf(TEXT("%s  x%d"), *Stack.ItemId.ToString(), Stack.Quantity)));
                Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));
                Text->SetAutoWrapText(true);
                USizeBox* TileSize = WidgetTree->ConstructWidget<USizeBox>();
                TileSize->SetWidthOverride(108.f);
                TileSize->SetHeightOverride(96.f);
                UBorder* TileBackground = WidgetTree->ConstructWidget<UBorder>();
                TileBackground->SetPadding(FMargin(6.f));
                TileBackground->SetContent(Text);
                TileSize->SetContent(TileBackground);
                Tile->AddChild(TileSize);
                const bool bThisContainer = Grid == LootGrid;
                const bool bSelected = Stack.ItemId == SelectedId && bThisContainer == bSelectedFromContainer;
                const FString Rarity = FDeadlandsItems::Rarity(Stack.ItemId);
                const FLinearColor Base = Rarity == TEXT("RARE") ? FLinearColor(0.29f, 0.21f, 0.39f) : Rarity == TEXT("UNCOMMON") ? FLinearColor(0.17f, 0.31f, 0.29f) : FLinearColor(0.16f, 0.22f, 0.27f);
                Tile->SetBackgroundColor(bSelected ? FLinearColor(0.43f, 0.53f, 0.60f) : Base);
                Tile->SetToolTipText(FText::FromString(FString::Printf(TEXT("%s | %s | %d weight each | x%d"), *Stack.ItemId.ToString(), FDeadlandsItems::Rarity(Stack.ItemId), FDeadlandsItems::Weight(Stack.ItemId), Stack.Quantity)));
                Grid->AddChildToUniformGrid(Tile, (Index % 16) / 3, Index % 3);
                switch (Index)
                {
                    case 0: Tile->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::SelectSlot0); break;
                    case 1: Tile->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::SelectSlot1); break;
                    case 2: Tile->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::SelectSlot2); break;
                    case 3: Tile->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::SelectSlot3); break;
                    case 4: Tile->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::SelectSlot4); break;
                    case 5: Tile->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::SelectSlot5); break;
                    case 6: Tile->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::SelectSlot6); break;
                    case 7: Tile->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::SelectSlot7); break;
                    case 8: Tile->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::SelectSlot8); break;
                    case 9: Tile->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::SelectSlot9); break;
                    case 10: Tile->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::SelectSlot10); break;
                    case 11: Tile->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::SelectSlot11); break;
                    case 12: Tile->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::SelectSlot12); break;
                    case 13: Tile->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::SelectSlot13); break;
                    case 14: Tile->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::SelectSlot14); break;
                    case 15: Tile->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::SelectSlot15); break;
                    case 16: Tile->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::SelectSlot16); break;
                    case 17: Tile->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::SelectSlot17); break;
                    case 18: Tile->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::SelectSlot18); break;
                    case 19: Tile->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::SelectSlot19); break;
                    case 20: Tile->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::SelectSlot20); break;
                    case 21: Tile->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::SelectSlot21); break;
                    case 22: Tile->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::SelectSlot22); break;
                    case 23: Tile->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::SelectSlot23); break;
                    case 24: Tile->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::SelectSlot24); break;
                    case 25: Tile->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::SelectSlot25); break;
                    case 26: Tile->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::SelectSlot26); break;
                    case 27: Tile->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::SelectSlot27); break;
                    case 28: Tile->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::SelectSlot28); break;
                    case 29: Tile->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::SelectSlot29); break;
                    case 30: Tile->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::SelectSlot30); break;
                    case 31: Tile->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::SelectSlot31); break;
                }
                ++Index;
            }
            Index = 16;
        };
        if (Survivor) BuildGrid(PlayerGrid, Survivor->GetItems());
        Index = 16;
        if (LootContainer) BuildGrid(LootGrid, LootContainer->GetItems());
    }
    if (QuickbarLabel)
    {
        FString Text = TEXT("QUICK ACCESS");
        for (int32 I = 0; I < QuickSlots.Num(); ++I)
            Text += FString::Printf(TEXT("  [%d] %s"), I + 1, QuickSlots[I].IsNone() ? TEXT("-") : *QuickSlots[I].ToString());
        QuickbarLabel->SetText(FText::FromString(Text));
    }
    if (ItemDetails)
    {
        if (SelectedId.IsNone())
            ItemDetails->SetText(FText::FromString(TEXT("SELECT A TILE  |  E TRANSFER  |  DEL DROP  |  CTRL+1-5 ASSIGN")));
        else
        {
            int32 Count = 0;
            const TArray<FDeadlandsItemStack> Stacks = bSelectedFromContainer
                ? (LootContainer ? LootContainer->GetItems() : TArray<FDeadlandsItemStack>())
                : (Survivor ? Survivor->GetItems() : TArray<FDeadlandsItemStack>());
            for (const auto& Stack : Stacks)
                if (Stack.ItemId == SelectedId) Count += Stack.Quantity;
            const TCHAR* Hint = SelectedId == TEXT("Food") ? TEXT("Restores hunger")
                : SelectedId == TEXT("Water") ? TEXT("Restores thirst")
                : SelectedId == TEXT("Bandage") ? TEXT("Treats bleeding")
                : SelectedId == TEXT("Antibiotics") ? TEXT("Treats infection")
                : SelectedId == TEXT("Medicine") ? TEXT("Medical supply")
                : SelectedId == TEXT("Backpack") ? TEXT("Equip for +20 carry capacity")
                : SelectedId == TEXT("Tools") ? TEXT("Crafting / repair tools")
                : SelectedId == TEXT("Pistol") ? TEXT("Weapon")
                : TEXT("Survival resource");
            ItemDetails->SetText(FText::FromString(FString::Printf(TEXT("%s  x%d  |  %s  |  %d EACH  |  %s  |  %s"),
                *SelectedId.ToString(), Count, FDeadlandsItems::Rarity(SelectedId),
                FDeadlandsItems::Weight(SelectedId), bSelectedFromContainer ? TEXT("CONTAINER") : TEXT("BACKPACK"), Hint)));
        }
    }
    if (TakeButton) TakeButton->SetIsEnabled(IsValid(LootContainer) && !SelectedId.IsNone() && bSelectedFromContainer);
    if (PutButton) PutButton->SetIsEnabled(IsValid(LootContainer) && !SelectedId.IsNone() && !bSelectedFromContainer);
}

void UDeadlandsInventoryWidget::TakeOne()
{
    if (Survivor && LootContainer && !SelectedId.IsNone() && bSelectedFromContainer)
        Survivor->RequestItemTransfer(LootContainer, SelectedId, true, 1);
}

void UDeadlandsInventoryWidget::PutOne()
{
    if (Survivor && LootContainer && !SelectedId.IsNone() && !bSelectedFromContainer)
        Survivor->RequestItemTransfer(LootContainer, SelectedId, false, 1);
}

void UDeadlandsInventoryWidget::TakeStack()
{
    if (Survivor && LootContainer && !SelectedId.IsNone() && bSelectedFromContainer)
    {
        const FName Id = SelectedId;
        Survivor->RequestItemTransfer(LootContainer, Id, true, FDeadlandsItems::MaxStack(Id));
    }
}

void UDeadlandsInventoryWidget::PutStack()
{
    if (Survivor && LootContainer && !SelectedId.IsNone() && !bSelectedFromContainer)
    {
        const FName Id = SelectedId;
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
    const FKey Key = Event.GetKey();
    const int32 QuickIndex = Key == EKeys::One ? 0 : Key == EKeys::Two ? 1 : Key == EKeys::Three ? 2 : Key == EKeys::Four ? 3 : Key == EKeys::Five ? 4 : INDEX_NONE;
    if (QuickIndex != INDEX_NONE)
    {
        if (Event.IsControlDown()) AssignQuickSlot(QuickIndex);
        else UseQuickSlot(QuickIndex);
        return FReply::Handled();
    }
    if (Key == EKeys::E && LootContainer && !SelectedId.IsNone())
    {
        if (bSelectedFromContainer) TakeStack(); else PutStack();
        return FReply::Handled();
    }
    if (Key == EKeys::Delete && !bSelectedFromContainer && !SelectedId.IsNone())
    {
        DropSelected();
        return FReply::Handled();
    }
    return Super::NativeOnPreviewKeyDown(Geometry, Event);
}

void UDeadlandsInventoryWidget::ConsumeSelected()
{
    if (Survivor && !SelectedId.IsNone() && !bSelectedFromContainer)
        Survivor->RequestConsumeItem(SelectedId);
}
void UDeadlandsInventoryWidget::DropSelected()
{
    if (Survivor && !SelectedId.IsNone() && !bSelectedFromContainer)
        Survivor->RequestDropItem(SelectedId);
}

void UDeadlandsInventoryWidget::EquipBackpack() { if (Survivor) Survivor->RequestEquipBackpack(); }
void UDeadlandsInventoryWidget::CraftBandage() { if (Survivor) Survivor->RequestCraft(TEXT("Bandage")); }
void UDeadlandsInventoryWidget::CraftTools() { if (Survivor) Survivor->RequestCraft(TEXT("Tools")); }
void UDeadlandsInventoryWidget::CraftMedicine() { if (Survivor) Survivor->RequestCraft(TEXT("Medicine")); }
void UDeadlandsInventoryWidget::RepairTools() { if (Survivor) Survivor->RequestRepairTools(); }

void UDeadlandsInventoryWidget::SelectIndex(int32 Index)
{
    if (!Survivor) return;
    const bool bContainer = Index >= 16;
    const auto Stacks = bContainer ? (LootContainer ? LootContainer->GetItems() : TArray<FDeadlandsItemStack>()) : Survivor->GetItems();
    const int32 LocalIndex = bContainer ? Index - 16 : Index;
    if (!Stacks.IsValidIndex(LocalIndex)) return;
    SelectedId = Stacks[LocalIndex].ItemId;
    bSelectedFromContainer = bContainer;
    if (ItemPicker) ItemPicker->SetSelectedOption(SelectedId.ToString());
    GridSignature.Empty();
    Refresh();
}
void UDeadlandsInventoryWidget::SelectSlot0() { SelectIndex(0); }
void UDeadlandsInventoryWidget::SelectSlot1() { SelectIndex(1); }
void UDeadlandsInventoryWidget::SelectSlot2() { SelectIndex(2); }
void UDeadlandsInventoryWidget::SelectSlot3() { SelectIndex(3); }
void UDeadlandsInventoryWidget::SelectSlot4() { SelectIndex(4); }
void UDeadlandsInventoryWidget::SelectSlot5() { SelectIndex(5); }
void UDeadlandsInventoryWidget::SelectSlot6() { SelectIndex(6); }
void UDeadlandsInventoryWidget::SelectSlot7() { SelectIndex(7); }
void UDeadlandsInventoryWidget::SelectSlot8() { SelectIndex(8); }
void UDeadlandsInventoryWidget::SelectSlot9() { SelectIndex(9); }
void UDeadlandsInventoryWidget::SelectSlot10() { SelectIndex(10); }
void UDeadlandsInventoryWidget::SelectSlot11() { SelectIndex(11); }
void UDeadlandsInventoryWidget::SelectSlot12() { SelectIndex(12); }
void UDeadlandsInventoryWidget::SelectSlot13() { SelectIndex(13); }
void UDeadlandsInventoryWidget::SelectSlot14() { SelectIndex(14); }
void UDeadlandsInventoryWidget::SelectSlot15() { SelectIndex(15); }

void UDeadlandsInventoryWidget::AssignQuickSlot(int32 SlotIndex)
{
    const bool bUsable = SelectedId == TEXT("Food") || SelectedId == TEXT("Water") ||
        SelectedId == TEXT("Medicine") || SelectedId == TEXT("Bandage") ||
        SelectedId == TEXT("Antibiotics") || SelectedId == TEXT("Backpack");
    if (QuickSlots.IsValidIndex(SlotIndex) && bUsable && !bSelectedFromContainer)
        QuickSlots[SlotIndex] = SelectedId;
    Refresh();
}
void UDeadlandsInventoryWidget::UseQuickSlot(int32 SlotIndex)
{
    if (Survivor && QuickSlots.IsValidIndex(SlotIndex) && !QuickSlots[SlotIndex].IsNone())
    {
        const FName Id = QuickSlots[SlotIndex];
        if (Id == TEXT("Backpack")) Survivor->RequestEquipBackpack();
        else if (Id == TEXT("Food") || Id == TEXT("Water") || Id == TEXT("Medicine") || Id == TEXT("Bandage") || Id == TEXT("Antibiotics")) Survivor->RequestConsumeItem(Id);
    }
}
void UDeadlandsInventoryWidget::AssignQuickSlot1() { AssignQuickSlot(0); }
void UDeadlandsInventoryWidget::UseQuickSlot1() { UseQuickSlot(0); }
void UDeadlandsInventoryWidget::AssignQuickSlot2() { AssignQuickSlot(1); }
void UDeadlandsInventoryWidget::UseQuickSlot2() { UseQuickSlot(1); }
void UDeadlandsInventoryWidget::AssignQuickSlot3() { AssignQuickSlot(2); }
void UDeadlandsInventoryWidget::UseQuickSlot3() { UseQuickSlot(2); }
void UDeadlandsInventoryWidget::AssignQuickSlot4() { AssignQuickSlot(3); }
void UDeadlandsInventoryWidget::UseQuickSlot4() { UseQuickSlot(3); }
void UDeadlandsInventoryWidget::AssignQuickSlot5() { AssignQuickSlot(4); }
void UDeadlandsInventoryWidget::UseQuickSlot5() { UseQuickSlot(4); }
void UDeadlandsInventoryWidget::SelectSlot16() { SelectIndex(16); }
void UDeadlandsInventoryWidget::SelectSlot17() { SelectIndex(17); }
void UDeadlandsInventoryWidget::SelectSlot18() { SelectIndex(18); }
void UDeadlandsInventoryWidget::SelectSlot19() { SelectIndex(19); }
void UDeadlandsInventoryWidget::SelectSlot20() { SelectIndex(20); }
void UDeadlandsInventoryWidget::SelectSlot21() { SelectIndex(21); }
void UDeadlandsInventoryWidget::SelectSlot22() { SelectIndex(22); }
void UDeadlandsInventoryWidget::SelectSlot23() { SelectIndex(23); }
void UDeadlandsInventoryWidget::SelectSlot24() { SelectIndex(24); }
void UDeadlandsInventoryWidget::SelectSlot25() { SelectIndex(25); }
void UDeadlandsInventoryWidget::SelectSlot26() { SelectIndex(26); }
void UDeadlandsInventoryWidget::SelectSlot27() { SelectIndex(27); }
void UDeadlandsInventoryWidget::SelectSlot28() { SelectIndex(28); }
void UDeadlandsInventoryWidget::SelectSlot29() { SelectIndex(29); }
void UDeadlandsInventoryWidget::SelectSlot30() { SelectIndex(30); }
void UDeadlandsInventoryWidget::SelectSlot31() { SelectIndex(31); }

void UDeadlandsInventoryWidget::TakeFive()
{
    if (Survivor && LootContainer && !SelectedId.IsNone() && bSelectedFromContainer)
        Survivor->RequestItemTransfer(LootContainer, SelectedId, true, 5);
}
void UDeadlandsInventoryWidget::PutFive()
{
    if (Survivor && LootContainer && !SelectedId.IsNone() && !bSelectedFromContainer)
        Survivor->RequestItemTransfer(LootContainer, SelectedId, false, 5);
}
\nvoid UDeadlandsInventoryWidget::ToggleCraftActions()\n{\n    if (CraftActions)\n        CraftActions->SetVisibility(CraftActions->GetVisibility() == ESlateVisibility::Collapsed ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);\n}\n