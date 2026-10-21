#include "Inventory/DeadlandsInventoryWidget.h"
#include "Inventory/DeadlandsInventoryTile.h"
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
    EquipmentSignature.Empty();
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
    Header = MakeText(TEXT("DEADLANDS  /  INVENTORY  |  [B] BUILD  [F] REPAIR  [X] DISMANTLE  [TAB] CLOSE"));
    Header->SetColorAndOpacity(FSlateColor(FLinearColor(0.86f, 0.72f, 0.43f)));
    PlayerList = MakeText(TEXT("SURVIVOR"));
    ContainerList = MakeText(TEXT("NEARBY LOOT"));
    ItemPicker = WidgetTree->ConstructWidget<UComboBoxString>();
    for (const TCHAR* Id : { TEXT("Food"), TEXT("Water"), TEXT("Medicine"), TEXT("Tools"), TEXT("Materials"), TEXT("Bandage"), TEXT("Antibiotics"),
                              TEXT("Ammo9mm"), TEXT("Ammo12g"), TEXT("Scrap"),
                              TEXT("Cloth"), TEXT("Backpack"), TEXT("Pistol"), TEXT("Jacket"), TEXT("Pants"), TEXT("ArmorVest") })
        ItemPicker->AddOption(FString(Id));
    ItemPicker->SetSelectedOption(TEXT("Food"));
    ItemPicker->SetVisibility(ESlateVisibility::Collapsed);
    Column->AddChildToVerticalBox(ItemPicker);
    UHorizontalBox* InventoryPanels = WidgetTree->ConstructWidget<UHorizontalBox>();
    Column->AddChildToVerticalBox(InventoryPanels);
    UVerticalBox* PlayerPanel = WidgetTree->ConstructWidget<UVerticalBox>();
    UVerticalBox* LootPanel = WidgetTree->ConstructWidget<UVerticalBox>();
    UVerticalBox* BodyPanel = WidgetTree->ConstructWidget<UVerticalBox>();
    UHorizontalBoxSlot* LeftSlot = InventoryPanels->AddChildToHorizontalBox(PlayerPanel);
    UHorizontalBoxSlot* BodySlot = InventoryPanels->AddChildToHorizontalBox(BodyPanel);
    UHorizontalBoxSlot* RightSlot = InventoryPanels->AddChildToHorizontalBox(LootPanel);
    BodySlot->SetPadding(FMargin(3.f, 10.f, 3.f, 10.f));
    LeftSlot->SetPadding(FMargin(0.f, 10.f, 12.f, 10.f));
    RightSlot->SetPadding(FMargin(12.f, 10.f, 0.f, 10.f));
    UTextBlock* LeftTitle = WidgetTree->ConstructWidget<UTextBlock>();
    UTextBlock* RightTitle = WidgetTree->ConstructWidget<UTextBlock>();
    LeftTitle->SetText(FText::FromString(TEXT("SURVIVOR  /  INVENTORY")));
    RightTitle->SetText(FText::FromString(TEXT("SEARCHING  /  CONTAINER")));
    LeftTitle->SetColorAndOpacity(FSlateColor(FLinearColor(0.75f, 0.85f, 0.92f)));
    RightTitle->SetColorAndOpacity(FSlateColor(FLinearColor(0.75f, 0.85f, 0.92f)));
    PlayerPanel->AddChildToVerticalBox(LeftTitle);
    UTextBlock* BodyTitle = WidgetTree->ConstructWidget<UTextBlock>();
    BodyTitle->SetText(FText::FromString(TEXT("EQUIPMENT")));
    BodyTitle->SetColorAndOpacity(FSlateColor(FLinearColor(.86f,.72f,.43f)));
    BodyPanel->AddChildToVerticalBox(BodyTitle);
    UTextBlock* HeadLabel = WidgetTree->ConstructWidget<UTextBlock>();
    HeadLabel->SetText(FText::FromString(TEXT("  [ HEAD ]")));
    HeadLabel->SetJustification(ETextJustify::Center);
    BodyPanel->AddChildToVerticalBox(HeadLabel);
    EquipmentGrid = WidgetTree->ConstructWidget<UUniformGridPanel>();
    BodyPanel->AddChildToVerticalBox(EquipmentGrid);
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
    EquipmentLabel = MakeText(TEXT("EQUIPMENT  /  JACKET: EMPTY  |  PANTS: EMPTY  |  VEST: EMPTY"));
    EquipmentLabel->SetColorAndOpacity(FSlateColor(FLinearColor(.86f,.72f,.43f)));
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
    TakeFiveButton->SetVisibility(ESlateVisibility::Collapsed);
    UButton* PutFiveButton = WidgetTree->ConstructWidget<UButton>();
    UTextBlock* PutFiveLabel = WidgetTree->ConstructWidget<UTextBlock>();
    PutFiveLabel->SetText(FText::FromString(TEXT(" PUT 5 ")));
    PutFiveButton->AddChild(PutFiveLabel);
    Buttons->AddChildToHorizontalBox(PutFiveButton);
    PutFiveButton->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::PutFive);
    PutFiveButton->SetVisibility(ESlateVisibility::Collapsed);
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
    TakeStackButton->SetVisibility(ESlateVisibility::Collapsed);
    PutStackButton->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::PutStack);
    PutStackButton->SetVisibility(ESlateVisibility::Collapsed);
    UButton* ConsumeButton = WidgetTree->ConstructWidget<UButton>();
    UseButton = ConsumeButton;
    UTextBlock* ConsumeLabel = WidgetTree->ConstructWidget<UTextBlock>();
    ConsumeLabel->SetText(FText::FromString(TEXT(" USE ")));
    ConsumeButton->AddChild(ConsumeLabel);
    Buttons->AddChildToHorizontalBox(ConsumeButton);
    ConsumeButton->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::ConsumeSelected);
    UButton* DropButton = WidgetTree->ConstructWidget<UButton>();
    DropActionButton = DropButton;
    UTextBlock* DropLabel = WidgetTree->ConstructWidget<UTextBlock>();
    DropLabel->SetText(FText::FromString(TEXT(" DROP ")));
    DropButton->AddChild(DropLabel);
    Buttons->AddChildToHorizontalBox(DropButton);
    DropButton->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::DropSelected);
    UButton* TakeAllButton = WidgetTree->ConstructWidget<UButton>();
    UTextBlock* TakeAllLabel = WidgetTree->ConstructWidget<UTextBlock>();
    TakeAllLabel->SetText(FText::FromString(TEXT(" TAKE ALL ")));
    TakeAllButton->AddChild(TakeAllLabel);
    Buttons->AddChildToHorizontalBox(TakeAllButton);
    TakeAllButton->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::TakeAll);
    StatusLabel = MakeText(TEXT("DRAG TO MOVE  |  SHIFT+DRAG STACK  |  DOUBLE-CLICK TRANSFER"));
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
    UHorizontalBox* EquipmentActions = WidgetTree->ConstructWidget<UHorizontalBox>();
    Column->AddChildToVerticalBox(EquipmentActions);
    auto AddEquipmentAction = [&](const TCHAR* Title)
    {
        UButton* Action = WidgetTree->ConstructWidget<UButton>();
        UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
        Label->SetText(FText::FromString(Title));
        Action->AddChild(Label);
        EquipmentActions->AddChildToHorizontalBox(Action);
        return Action;
    };
    UButton* BackpackButton = AddEquipmentAction(TEXT(" BACKPACK "));
    BackpackButton->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::EquipBackpack);
    UButton* JacketButton = AddEquipmentAction(TEXT(" JACKET "));
    JacketButton->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::ToggleJacket);
    UButton* PantsButton = AddEquipmentAction(TEXT(" PANTS "));
    PantsButton->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::TogglePants);
    UButton* VestButton = AddEquipmentAction(TEXT(" ARMOR VEST "));
    VestButton->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::ToggleArmor);
    UButton* BandageButton = AddAction(TEXT(" BANDAGE "));
    BandageButton->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::CraftBandage);
    UButton* ToolsButton = AddAction(TEXT(" TOOLS "));
    ToolsButton->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::CraftTools);
    UButton* MedicineButton = AddAction(TEXT(" MEDICINE "));
    MedicineButton->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::CraftMedicine);
    UButton* RepairButton = AddAction(TEXT(" REPAIR "));
    RepairButton->OnClicked.AddDynamic(this, &UDeadlandsInventoryWidget::RepairTools);
    UHorizontalBox* QuickAssign = WidgetTree->ConstructWidget<UHorizontalBox>();
    UHorizontalBox* QuickUse = WidgetTree->ConstructWidget<UHorizontalBox>();
    Column->AddChildToVerticalBox(QuickAssign);
    Column->AddChildToVerticalBox(QuickUse);
    QuickAssign->SetVisibility(ESlateVisibility::Collapsed);
    QuickUse->SetVisibility(ESlateVisibility::Collapsed);
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
        auto BuildGrid = [&](UUniformGridPanel* Grid, const TArray<FDeadlandsItemStack>& Stacks, bool bContainer)
        {
            for (int32 Index = 0; Index < 16; ++Index)
            {
                const FDeadlandsItemStack Stack = Stacks.IsValidIndex(Index) ? Stacks[Index] : FDeadlandsItemStack();
                UDeadlandsInventoryTile* Tile = CreateWidget<UDeadlandsInventoryTile>(GetOwningPlayer(), UDeadlandsInventoryTile::StaticClass());
                if (!Tile) continue;
                Tile->Configure(this, bContainer, Index, Stack.ItemId, Stack.Quantity);
                Tile->SetSelected(!Stack.ItemId.IsNone() && Stack.ItemId == SelectedId && bContainer == bSelectedFromContainer);
                Grid->AddChildToUniformGrid(Tile, Index / 3, Index % 3);
            }
        };
        if (Survivor) BuildGrid(PlayerGrid, Survivor->GetItems(), false);
        if (LootContainer) BuildGrid(LootGrid, LootContainer->GetItems(), true);
    }
    if (EquipmentLabel && Survivor)
        EquipmentLabel->SetText(FText::FromString(FString::Printf(
            TEXT("EQUIPMENT  |  JACKET: %s (+4 KG)  |  PANTS: %s (+2 KG)  |  VEST: %s (30%% PROTECTION)"),
            Survivor->IsGearEquipped(TEXT("Jacket")) ? TEXT("ON") : TEXT("OFF"),
            Survivor->IsGearEquipped(TEXT("Pants")) ? TEXT("ON") : TEXT("OFF"),
            Survivor->IsGearEquipped(TEXT("ArmorVest")) ? TEXT("ON") : TEXT("OFF"))));
    if (Survivor && EquipmentGrid)
    {
        const FName Slots[4] = {
            Survivor->HasEquippedBackpack() ? FName(TEXT("Backpack")) : NAME_None,
            Survivor->IsGearEquipped(TEXT("Jacket")) ? FName(TEXT("Jacket")) : NAME_None,
            Survivor->IsGearEquipped(TEXT("Pants")) ? FName(TEXT("Pants")) : NAME_None,
            Survivor->IsGearEquipped(TEXT("ArmorVest")) ? FName(TEXT("ArmorVest")) : NAME_None
        };
        FString NewSignature;
        for (const FName Id : Slots) NewSignature += Id.ToString() + TEXT("|");
        if (NewSignature != EquipmentSignature)
        {
            EquipmentSignature = NewSignature;
            EquipmentGrid->ClearChildren();
            for (int32 Index = 0; Index < 4; ++Index)
            {
                UDeadlandsInventoryTile* Tile = CreateWidget<UDeadlandsInventoryTile>(GetOwningPlayer(), UDeadlandsInventoryTile::StaticClass());
                if (Tile)
                {
                    Tile->ConfigureEquipment(this, Index, Slots[Index]);
                    EquipmentGrid->AddChildToUniformGrid(Tile, Index, 0);
                }
            }
        }
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
    if (TakeButton) { TakeButton->SetIsEnabled(IsValid(LootContainer) && !SelectedId.IsNone() && bSelectedFromContainer);
        TakeButton->SetVisibility(IsValid(LootContainer) && bSelectedFromContainer && !SelectedId.IsNone() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed); }
    if (PutButton) { PutButton->SetIsEnabled(IsValid(LootContainer) && !SelectedId.IsNone() && !bSelectedFromContainer);
        PutButton->SetVisibility(IsValid(LootContainer) && !bSelectedFromContainer && !SelectedId.IsNone() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed); }
    if (UseButton) UseButton->SetVisibility(SelectedId == TEXT("Food") || SelectedId == TEXT("Water") ||
        SelectedId == TEXT("Medicine") || SelectedId == TEXT("Bandage") || SelectedId == TEXT("Antibiotics") ||
        SelectedId == TEXT("Backpack") || SelectedId == TEXT("Jacket") || SelectedId == TEXT("Pants") ||
        SelectedId == TEXT("ArmorVest") ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    if (DropActionButton) DropActionButton->SetVisibility(!SelectedId.IsNone() && !bSelectedFromContainer ?
        ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    if (StatusLabel) StatusLabel->SetText(FText::FromString(StatusMessage.IsEmpty() ?
        TEXT("DRAG: MOVE ONE  |  SHIFT+DRAG: STACK  |  DOUBLE-CLICK: TRANSFER") : StatusMessage));
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
    if (Survivor && !SelectedId.IsNone())
    {
        if (bSelectedFromContainer && IsValid(LootContainer)) Survivor->RequestUseContainerItem(LootContainer, SelectedId);
        else if (!bSelectedFromContainer)
        {
            if (SelectedId == TEXT("Backpack")) Survivor->RequestEquipBackpack();
            else if (SelectedId == TEXT("Jacket") || SelectedId == TEXT("Pants") || SelectedId == TEXT("ArmorVest")) Survivor->RequestToggleEquipment(SelectedId);
            else Survivor->RequestConsumeItem(SelectedId);
        }
    }
}
void UDeadlandsInventoryWidget::DropSelected()
{
    if (Survivor && !SelectedId.IsNone() && !bSelectedFromContainer)
        Survivor->RequestDropItem(SelectedId);
}

void UDeadlandsInventoryWidget::EquipBackpack() { if (Survivor) Survivor->RequestEquipBackpack(); }
void UDeadlandsInventoryWidget::ToggleJacket() { if (Survivor) Survivor->RequestToggleEquipment(TEXT("Jacket")); }
void UDeadlandsInventoryWidget::TogglePants() { if (Survivor) Survivor->RequestToggleEquipment(TEXT("Pants")); }
void UDeadlandsInventoryWidget::ToggleArmor() { if (Survivor) Survivor->RequestToggleEquipment(TEXT("ArmorVest")); }
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

void UDeadlandsInventoryWidget::ToggleCraftActions()
{
    if (CraftActions)
        CraftActions->SetVisibility(CraftActions->GetVisibility() == ESlateVisibility::Collapsed ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void UDeadlandsInventoryWidget::SelectTile(bool bContainer, int32 Index)
{
 if(!Survivor)return;
 const TArray<FDeadlandsItemStack> Stacks=bContainer?(LootContainer?LootContainer->GetItems():TArray<FDeadlandsItemStack>()):Survivor->GetItems();
 SelectedId=Stacks.IsValidIndex(Index)?Stacks[Index].ItemId:NAME_None;
 bSelectedFromContainer=bContainer;
 // Do not rebuild the tile grid during mouse-down: destroying the source widget
 // cancels Slate's drag detection before the cursor can move.
 auto HighlightGrid = [&](UUniformGridPanel* Grid, bool bGridContainer)
 {
  if (!Grid) return;
  for (int32 TileIndex = 0; TileIndex < Grid->GetChildrenCount(); ++TileIndex)
   if (UDeadlandsInventoryTile* Tile = Cast<UDeadlandsInventoryTile>(Grid->GetChildAt(TileIndex)))
    Tile->SetSelected(TileIndex == Index && bGridContainer == bContainer && !SelectedId.IsNone());
 };
 HighlightGrid(PlayerGrid, false);
 HighlightGrid(LootGrid, true);
 Refresh();
}
void UDeadlandsInventoryWidget::TransferTile(bool bContainer,FName ItemId,int32 Quantity)
{
 if(!Survivor||!IsValid(LootContainer)||ItemId.IsNone())return;
 Survivor->RequestItemTransfer(LootContainer,ItemId,bContainer,FMath::Clamp(Quantity,1,30));
 StatusMessage=FString::Printf(TEXT("%s %s x%d"),bContainer?TEXT("TAKING"):TEXT("STORING"),*ItemId.ToString(),Quantity);
}
bool UDeadlandsInventoryWidget::DropTile(bool bSourceContainer,int32 SourceIndex,FName ItemId,bool bStack,bool bDestinationContainer,int32 DestinationIndex)
{
 if(!Survivor||ItemId.IsNone()||SourceIndex<0||DestinationIndex<0||DestinationIndex>=16)return false;
 const TArray<FDeadlandsItemStack> Source=bSourceContainer?(LootContainer?LootContainer->GetItems():TArray<FDeadlandsItemStack>()):Survivor->GetItems();
 if(!Source.IsValidIndex(SourceIndex)||Source[SourceIndex].ItemId!=ItemId)return false;
 if(bSourceContainer!=bDestinationContainer)
 {
  if(!IsValid(LootContainer))return false;
  TransferTile(bSourceContainer,ItemId,bStack?Source[SourceIndex].Quantity:1);
  return true;
 }
 if(SourceIndex==DestinationIndex)return true;
 if(!bSourceContainer||IsValid(LootContainer))
 {
  Survivor->RequestReorderItem(bSourceContainer,LootContainer,SourceIndex,DestinationIndex);
  StatusMessage=TEXT("REARRANGING ITEMS");
  return true;
 }
 return false;
}
void UDeadlandsInventoryWidget::TakeAll()
{
 if(!Survivor||!IsValid(LootContainer))return;
 TArray<FDeadlandsItemStack> Stacks=LootContainer->GetItems();
 for(const auto& Stack:Stacks)
  if(FDeadlandsItems::Valid(Stack))
   Survivor->RequestItemTransfer(LootContainer,Stack.ItemId,true,FMath::Min(Stack.Quantity,30));
 StatusMessage=TEXT("TAKE ALL REQUESTED - LIMITED BY CAPACITY");
}

bool UDeadlandsInventoryWidget::IsEquipped(FName ItemId) const
{
 if(!Survivor)return false;
 return ItemId==TEXT("Backpack")?Survivor->HasEquippedBackpack():Survivor->IsGearEquipped(ItemId);
}
bool UDeadlandsInventoryWidget::UnequipToInventory(FName ItemId)
{
 if(!Survivor||!IsEquipped(ItemId))return false;
 if(ItemId==TEXT("Backpack"))Survivor->RequestEquipBackpack();
 else Survivor->RequestToggleEquipment(ItemId);
 StatusMessage=FString::Printf(TEXT("UNEQUIPPING %s"),*ItemId.ToString());
 return true;
}
bool UDeadlandsInventoryWidget::DropOnEquipment(FName ItemId,bool bFromContainer,bool bFromEquipment,int32 EquipmentSlot)
{
 if(!Survivor||ItemId.IsNone()||bFromEquipment)return false;
 const FName Expected=EquipmentSlot==0?FName(TEXT("Backpack")):
  EquipmentSlot==1?FName(TEXT("Jacket")):
  EquipmentSlot==2?FName(TEXT("Pants")):
  EquipmentSlot==3?FName(TEXT("ArmorVest")):NAME_None;
 if(ItemId!=Expected||IsEquipped(ItemId))
 {
  StatusMessage=TEXT("WRONG SLOT OR ALREADY EQUIPPED");
  return false;
 }
 if(bFromContainer)
 {
  if(!IsValid(LootContainer))return false;
  const TArray<FDeadlandsItemStack> Items=LootContainer->GetItems();
  bool bFound=false;
  for(const auto& Stack:Items)if(Stack.ItemId==ItemId&&Stack.Quantity>0){bFound=true;break;}
  if(!bFound)return false;
  Survivor->RequestUseContainerItem(LootContainer,ItemId);
 }
 else
 {
  const TArray<FDeadlandsItemStack> Items=Survivor->GetItems();
  bool bFound=false;
  for(const auto& Stack:Items)if(Stack.ItemId==ItemId&&Stack.Quantity>0){bFound=true;break;}
  if(!bFound)return false;
  if(ItemId==TEXT("Backpack"))Survivor->RequestEquipBackpack();
  else Survivor->RequestToggleEquipment(ItemId);
 }
 StatusMessage=FString::Printf(TEXT("EQUIPPING %s"),*ItemId.ToString());
 return true;
}
