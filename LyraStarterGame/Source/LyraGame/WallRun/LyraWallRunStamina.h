// Copyright 2023 Sentya Anko

#pragma once

#include "CoreMinimal.h"
#include "LyraWallRunStamina.generated.h"


// @brief 自動回復する属性の設定
USTRUCT(BlueprintType)
struct FAutoRecoverableAttributeSetting
{
	GENERATED_BODY()

	// @brief Consume 時の秒間減少量
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)	float Consume = 40.f;

	// @brief 通常時の秒間増加量
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)	float RecoverDefault = 30.f;

	// @brief オーバーヒート時の秒間増加量
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)	float RecoverOverheat = 20.f;

	// @brief Consume をやめた後に増加し始めるまでの時間
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)	float CooldownTime = 1.f;

	// @brief 最小値
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)	float MinValue = 0.f;

	// @brief 最大値
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)	float MaxValue = 100.f;
};

// @brief FSavedMove に持たせるための構造体
//USTRUCT()
struct FSavedAutoRecoverableAttribute
{
	FSavedAutoRecoverableAttribute(float MaxValue = 0.f);

	// @brief オーバーヒート中か
	uint8 bOverheat : 1;

#if 0
	// @brief ステータスの変更があったか
	uint8 bStatusChanged : 1;

	// @brief ステータスの変更で減少が開始したか
	uint8 bStartConsume : 1;

#endif

	// @brief 現在値
	float CurrentValue;

	// @brief 増加/減少が始まった際の値
	float BaseValue;

	// @brief 増加/減少が始まってからの経過時間
	float TotalDeltaSeconds;
	
	// @brief 現在の増加開始までの待機時間
	float CurrentCooldownSeconds;

	// @brief 増加開始までの待機が始まった際の待機時間
	float BaseCooldownSeconds;

	// @brief 増加開始までの待機が始まってからの経過時間
	float TotalCooldownDeltaSeconds;

	// @brief 2 つの FSavedAutoRecoverableAttribute が結合可能か
	// @param lhs 左辺値
	// @param rhs 右辺値
	static bool CanCombineWith(const FSavedAutoRecoverableAttribute& lhs, const FSavedAutoRecoverableAttribute& rhs);
};

// @brief FSavedAutoRecoverableAttribute の操作を行うための構造体
USTRUCT(BlueprintType)
struct FSafeAutoRecoverableAttribute
{
	GENERATED_BODY()

	// @brief 設定
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FAutoRecoverableAttributeSetting Settings;

	// @brief 現在値
	FSavedAutoRecoverableAttribute Saved;

	FSafeAutoRecoverableAttribute();

	// @brief Saved の参照を取得するための関数。
	FSavedAutoRecoverableAttribute& GetSaved();

	// @brief Saved の const 参照を取得するための関数。
	const FSavedAutoRecoverableAttribute& GetSaved()const;

	// @brief 更新処理。
	// @param bConsume 消費する状態か。
	// @param DeltaSeconds 前回からの更新時間。
	// @param Notify 状態変更を(主に widget に)知らせるデリゲート。
	//		float CurrentValue		現在値。
	//		float AddValuePerSec	時間ごとの増加値。
	//		float Duration			期間。
	//		bool bFinished			オーバーヒートした or オーバーヒートから回復した。
	void OnUpdate(bool bConsume, float DeltaSeconds, TFunctionRef<void(float,float,float,bool)> Notify);

	// @brief 状態変更処理。
	// @param bConsume 消費する状態か。
	// @param Notify 状態変更を(主に widget に)知らせるデリゲート。
	//		float CurrentValue		現在値。
	//		float AddValuePerSec	時間ごとの増加値。
	//		float Duration			期間。
	//		bool bFinished			オーバーヒートした or オーバーヒートから回復した。
	void OnStatusChanged(bool bConsume, TFunctionRef<void(float, float, float, bool)> Notify);
};
