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
    UCanvasPanelSlot* PromptCanvasSlot = Canvas->AddChildToCanvas(PromptText);
    PromptCanvasSlot->SetAnchors(FAnchors(0.5f, 0.58f));
    PromptCanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
    PromptCanvasSlot->SetAutoSize(true);
    VitalsDebugText = WidgetTree->ConstructWidget<UTextBlock>();
    VitalsDebugText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    UCanvasPanelSlot* VitalsCanvasSlot = Canvas->AddChildToCanvas(VitalsDebugText);
    VitalsCanvasSlot->SetAnchors(FAnchors(0.f, 0.f));
    VitalsCanvasSlot->SetPosition(FVector2D(20.f, 35.f));
    VitalsCanvasSlot->SetAutoSize(true);
}
void UDeadlandsInteractionPrompt::SetPrompt(const FString& Message)
{
    if (PromptText) PromptText->SetText(FText::FromString(Message));
}

void UDeadlandsInteractionPrompt::SetVitalsDebug(const FString& Message)
{
    if (VitalsDebugText) VitalsDebugText->SetText(FText::FromString(Message));
}
