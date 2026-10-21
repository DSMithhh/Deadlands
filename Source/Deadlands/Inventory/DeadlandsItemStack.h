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

USTRUCT(BlueprintType)
struct FDeadlandsItemInventory
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FDeadlandsItemStack> Stacks;
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
        if (Id == TEXT("Bandage") || Id == TEXT("Antibiotics")) return 5;
        if (Id == TEXT("Ammo9mm") || Id == TEXT("Ammo12g")) return 30;
        if (Id == TEXT("Scrap") || Id == TEXT("Cloth")) return 20;
        if (Id == TEXT("Backpack") || Id == TEXT("Pistol") || Id == TEXT("Jacket") || Id == TEXT("Pants") || Id == TEXT("ArmorVest")) return 1;
        return 0;
    }
    // Whole-unit weights keep the existing save format and server transfer logic.
    static int32 Weight(FName Id)
    {
        if (Id == TEXT("Tools") || Id == TEXT("Backpack") || Id == TEXT("ArmorVest")) return 4;
        if (Id == TEXT("Jacket") || Id == TEXT("Pants")) return 2;
        if (Id == TEXT("Materials") || Id == TEXT("Scrap") || Id == TEXT("Pistol")) return 2;
        return 1;
    }
    static const TCHAR* Rarity(FName Id)
    {
        if (Id == TEXT("Pistol") || Id == TEXT("Antibiotics")) return TEXT("RARE");
        if (Id == TEXT("Backpack") || Id == TEXT("Medicine") || Id == TEXT("Jacket") || Id == TEXT("Pants")) return TEXT("UNCOMMON");
        if (Id == TEXT("ArmorVest")) return TEXT("RARE");
        return TEXT("COMMON");
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
    static int32 TotalWeight(const TArray<FDeadlandsItemStack>& Stacks)
    {
        int32 Sum = 0;
        for (const auto& Stack : Stacks)
            Sum += Stack.Quantity * Weight(Stack.ItemId);
        return Sum;
    }
    static bool AddOne(TArray<FDeadlandsItemStack>& Stacks, FName Id, int32 Capacity)
    {
        const int32 Max = MaxStack(Id);
        if (!Max || Capacity < Weight(Id) || TotalWeight(Stacks) > Capacity - Weight(Id)) return false;
        for (auto& Stack : Stacks)
            if (Stack.ItemId == Id && Stack.Quantity < Max) { ++Stack.Quantity; return true; }
        FDeadlandsItemStack NewStack;
        NewStack.ItemId = Id;
        NewStack.Quantity = 1;
        for (auto& Stack : Stacks)
            if (Stack.ItemId.IsNone() || Stack.Quantity <= 0) { Stack = NewStack; return true; }
        if (Stacks.Num() >= 16) return false;
        Stacks.Add(NewStack);
        return true;
    }
    static bool RemoveOne(TArray<FDeadlandsItemStack>& Stacks, FName Id)
    {
        for (int32 i = 0; i < Stacks.Num(); ++i)
            if (Stacks[i].ItemId == Id && Stacks[i].Quantity > 0)
            {
                if (--Stacks[i].Quantity == 0) { Stacks[i].ItemId = NAME_None; }
                while (!Stacks.IsEmpty() && Stacks.Last().ItemId.IsNone()) Stacks.Pop();
                return true;
            }
        return false;
    }
};
