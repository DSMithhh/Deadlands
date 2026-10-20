#pragma once
#include "CoreMinimal.h"
#include "DeadlandsItemStack.generated.h"

USTRUCT(BlueprintType)
struct FDeadlandsItemStack
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ItemId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0"))
    int32 Quantity = 0;
};

/** Prototype item catalogue: stable identifiers and fixed stack sizes. */
struct FDeadlandsItems
{
    static int32 MaxStack(FName Id)
    {
        if (Id == TEXT("Food") || Id == TEXT("Water")) return 5;
        if (Id == TEXT("Medicine")) return 3;
        if (Id == TEXT("Tools")) return 1;
        if (Id == TEXT("Materials")) return 10;
        return 0;
    }
    static bool Valid(const FDeadlandsItemStack& Stack)
    {
        return MaxStack(Stack.ItemId) > 0 && Stack.Quantity > 0;
    }
    static int32 Total(const TArray<FDeadlandsItemStack>& Stacks)
    {
        int32 Count = 0;
        for (const auto& Stack : Stacks) Count += Stack.Quantity;
        return Count;
    }
    static bool AddOne(TArray<FDeadlandsItemStack>& Stacks, FName Id, int32 Capacity)
    {
        const int32 Max = MaxStack(Id);
        if (!Max || Total(Stacks) >= Capacity) return false;
        for (auto& Stack : Stacks)
            if (Stack.ItemId == Id && Stack.Quantity < Max) { ++Stack.Quantity; return true; }
        FDeadlandsItemStack NewStack;
        NewStack.ItemId = Id;
        NewStack.Quantity = 1;
        Stacks.Add(NewStack);
        return true;
    }
    static bool RemoveOne(TArray<FDeadlandsItemStack>& Stacks, FName Id)
    {
        for (int32 i = 0; i < Stacks.Num(); ++i)
            if (Stacks[i].ItemId == Id && Stacks[i].Quantity > 0)
            {
                if (--Stacks[i].Quantity == 0) Stacks.RemoveAt(i);
                return true;
            }
        return false;
    }
};
