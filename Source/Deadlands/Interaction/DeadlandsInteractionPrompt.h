#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DeadlandsInteractionPrompt.generated.h"
class UTextBlock;
UCLASS()
class DEADLANDS_API UDeadlandsInteractionPrompt : public UUserWidget
{
    GENERATED_BODY()
public:
    virtual void NativeOnInitialized() override;
    void SetPrompt(const FString& Message);
private:
    UPROPERTY(Transient) TObjectPtr<UTextBlock> PromptText;
};
