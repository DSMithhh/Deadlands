#include "Interaction/DeadlandsInteractionPrompt.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
void UDeadlandsInteractionPrompt::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
    WidgetTree->RootWidget = Canvas;
    PromptText = WidgetTree->ConstructWidget<UTextBlock>();
    PromptText->SetText(FText::GetEmpty());
    PromptText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(PromptText);
    Slot->SetAnchors(FAnchors(0.5f, 0.58f));
    Slot->SetAlignment(FVector2D(0.5f, 0.5f));
    Slot->SetAutoSize(true);
}
void UDeadlandsInteractionPrompt::SetPrompt(const FString& Message)
{
    if (PromptText) PromptText->SetText(FText::FromString(Message));
}
