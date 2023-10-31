// Copyright Epic Games, Inc. All Rights Reserved.
// Copyright 2023 Sentya Anko
// Copy from AbilityTask_WaitMovementModeChange.h

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AbilityTask_WaitCustomMovementModeChange.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMovementModeChangedDelegate, int, Status);
DECLARE_DYNAMIC_DELEGATE_RetVal_FourParams(int, FCheckMovementModeChangedDelegate, EMovementMode, NewMovementMode, uint8, NewCustomMode, EMovementMode, PrevMovementMode, uint8, PreviousCustomMode);

/*
	任意の MovementMode になったときに任意の処理を行うための AbilityTask です。
	任意の MovementMode になったかは MovementMode が変更された時に CheckStatus を呼び出し、戻り値が非 0 かで判断します。
	任意の処理は OnChange を呼び出すことで行います。その際、 CheckStatus を引数に渡しています。
	そうすことで、どのような変化があったかをユーザーの任意の整数値で定義し、 CheckStatus で行った MovementMode の判定結果を OnChange で利用できるようにしています。
	CheckStatus が設定されていない場合、 MovementMode が変更されると OnChange が引数 0 で呼び出されます。
*/
UCLASS()
class UAbilityTask_WaitCustomMovementModeChange : public UAbilityTask
{
	GENERATED_BODY()

public:
	UAbilityTask_WaitCustomMovementModeChange(const FObjectInitializer& ObjectInitializer);


	//~UAbilityTask interface
public:
	virtual void OnDestroy(bool AbilityEnded) override;

	//~End of UAbilityTask interface

	//~UGameplayTask interface
protected:
	virtual void Activate() override;

	//~End of UGameplayTask interface

public:
	// @brief 任意の値に変更されたときに実行されるデリゲート。
	UPROPERTY(BlueprintAssignable)
	FMovementModeChangedDelegate	OnChange;

	/** Wait until movement mode changes (E.g., landing) */
	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE", DisplayName = "WaitCustomMovementModeChange"))
	static UAbilityTask_WaitCustomMovementModeChange* CreateWaitMovementModeChange(UGameplayAbility* OwningAbility, FCheckMovementModeChangedDelegate IsChanged, bool TriggerOnce = true);

private:
	// @brief MovementMode 変更時に呼び出されるデリゲート。
	// @param Character 変更があったキャラクター
	// @param PrevMovementMode 変更前の MovementMode
	// @param PreviousCustomMode 変更前の CustomMovementMode
	UFUNCTION()
	void OnMovementModeChange(class ACharacter* Character, EMovementMode PrevMovementMode, uint8 PreviousCustomMode);

private:
	// @brief MovementMode が任意の値になったかをチェックするデリゲート。
	FCheckMovementModeChangedDelegate CheckStatus;

	// @brief 一度だけトリガーするか。
	bool TriggerOnce;

	// @brief MovementMode を監視しているキャラクター。
	TWeakObjectPtr<class ACharacter>	MyCharacter;
};

