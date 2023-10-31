// Copyright 2023 Sentya Anko

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "NativeGameplayTags.h"

#include "LyraWallRunStaminaMessage.generated.h"

USTRUCT(BlueprintType)
struct FLyraWallRunStaminaMessage
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<AActor> Instigator = nullptr;
	
	UPROPERTY(BlueprintReadWrite)
	float CurrentValue = 0;

	UPROPERTY(BlueprintReadWrite)
	float AddValuePerSec = 0;

	UPROPERTY(BlueprintReadWrite)
	float Duration = 0;

	UPROPERTY(BlueprintReadWrite)
	bool bFinished = false;
};

USTRUCT(BlueprintType)
struct FLyraWallRunEnabledMessage
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<AActor> Instigator = nullptr;

	UPROPERTY(BlueprintReadWrite)
	bool bEnabled = false;
};
