// Copyright Epic Games, Inc. All Rights Reserved.
// Copyright 2023 Sentya Anko

#pragma once

#include "Components/GameFrameworkComponent.h"

#include "LyraWallRunStamina.h"
#include "LyraWRCharacterMovementComponent.h"
#include "LyraWallRunStaminaComponent.generated.h"

//	LYRA_WALLRUN_STAMINA_IN_COMPONENT
//		1 の場合
//			ULyraWallRunStaminaComponent に WallRun のスタミナに関する変数を保存する実装を有効にする。
//			LYRA_WALLRUN_STAMINA_IN_SAVED_MOVE が 0 の場合のみ利用可能。
#define LYRA_WALLRUN_STAMINA_IN_COMPONENT	0


//	LYRA_WALLRUN_STAMINA_IN_GA
//		1 の場合
//			ULyraWallRunStaminaComponent では設定用構造体の保持のみ行う実装を有効にする。
//			Gameplay Ability で WallRun のスタミナに関する変数を保存する場合に利用している。
//			LYRA_WALLRUN_STAMINA_IN_SAVED_MOVE が 0 の場合のみ利用可能。
#define LYRA_WALLRUN_STAMINA_IN_GA			0

//	LYRA_WALLRUN_STAMINA_IN_SAVED_MOVE の定義状態のチェック
#ifndef LYRA_WALLRUN_STAMINA_IN_SAVED_MOVE
	// LyraWRCharacterMovementComponent.h で定義しているはずです。
	#error LYRA_WALLRUN_STAMINA_IN_SAVED_MOVE is not defined.
#endif

// 設定の重複チェック
#if LYRA_WALLRUN_STAMINA_IN_SAVED_MOVE
	#if LYRA_WALLRUN_STAMINA_IN_COMPONENT
		#error LYRA_WALLRUN_STAMINA_IN_SAVED_MOVE and LYRA_WALLRUN_STAMINA_IN_COMPONENT cannot be activated at the same time.
	#elif LYRA_WALLRUN_STAMINA_IN_GA
		#error LYRA_WALLRUN_STAMINA_IN_SAVED_MOVE and LYRA_WALLRUN_STAMINA_IN_GA cannot be activated at the same time.
	#endif
#else
	#if LYRA_WALLRUN_STAMINA_IN_COMPONENT && LYRA_WALLRUN_STAMINA_IN_GA
		#error LYRA_WALLRUN_STAMINA_IN_COMPONENT and LYRA_WALLRUN_STAMINA_IN_GA cannot be activated at the same time.
	#endif
#endif


/**
 * ULyraWallRunStaminaComponent
 *
 *	An actor component used to handle anything related to WallRunStamina.
 */
UCLASS(Blueprintable, Meta=(BlueprintSpawnableComponent))
class LYRAGAME_API ULyraWallRunStaminaComponent : public UGameFrameworkComponent
{
	GENERATED_BODY()

public:

	ULyraWallRunStaminaComponent(const FObjectInitializer& ObjectInitializer);

public:
	//~UActorComponent interface
#if 1 // if LYRA_WALLRUN_STAMINA_IN_SAVED_MOVE
#elif 0 // if LYRA_WALLRUN_STAMINA_IN_COMPONENT
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
#endif
	virtual void BeginPlay()override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason)override;
protected:
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	//~End of UActorComponent interface

	//~UFUNCTION
	//~Blueprint callable functions
public:
	// Returns the WallRunStamina component if one exists on the specified actor.
	UFUNCTION(BlueprintPure, Category = "LyraWR|WallRunStamina")
	static ULyraWallRunStaminaComponent* FindWallRunStaminaComponent(const AActor* Actor) { return (Actor ? Actor->FindComponentByClass<ULyraWallRunStaminaComponent>() : nullptr); }

	// @brief Setting を取得する。
	// @return Settings 。
	UFUNCTION(BlueprintCallable, Category = "LyraWR|WallRunStamina")
	const FAutoRecoverableAttributeSetting& GetWallRunSettings()const;


#if 1 // if LYRA_WALLRUN_STAMINA_IN_SAVED_MOVE || LYRA_WALLRUN_STAMINA_IN_COMPONENT
	// @brief GA 用のダミー関数。
	UFUNCTION(BlueprintCallable, Category = "LyraWR|WallRunStamina") void RequestSetWallRunEnable(bool bEnableWallRun);

#elif 0 // if LYRA_WALLRUN_STAMINA_IN_GA
	// @brief WallRun の実行可能状態の変更を行い、 RPC でサーバー/クライアントと同期する。
	// @param bEnableWallRun WallRun が実行可能かどうか。
	UFUNCTION(BlueprintCallable, Category = "LyraWR|WallRunStamina") void RequestSetWallRunEnable(bool bEnableWallRun);

#endif

	//~End Blueprint callable functions

	//~RPC functions
private:
#if 1 // if LYRA_WALLRUN_STAMINA_IN_SAVED_MOVE || LYRA_WALLRUN_STAMINA_IN_COMPONENT
#elif 0 // if LYRA_WALLRUN_STAMINA_IN_GA
	// @brief クライアント RPC による WallRun の実行可能状態の変更。
	// @param bEnableWallRun WallRun が実行可能かどうか。
	UFUNCTION(Client, Reliable) void Client_SetWallRunEnable(bool bEnableWallRun);

	// @brief Server RPC による WallRun の実行可能状態の変更。
	// @param bEnableWallRun WallRun が実行可能かどうか。（bool を受け取るが、サーバーが受け入れるのは false のみ。）
	UFUNCTION(Server, Reliable) void Server_SetWallRunEnable(bool bEnableWallRun);
#endif

	//~End RPC functions

	//~delegate functions
private:
#if 1 // if LYRA_WALLRUN_STAMINA_IN_SAVED_MOVE
#elif 0 //if LYRA_WALLRUN_STAMINA_IN_COMPONENT
	// @brief MovementMode の変更時のデリゲート用関数。
	// @param Character 変更があったキャラクター。
	// @param PrevMovementMode 変更前の MovementMode 。
	// @param PreviousCustomMode 変更前の CustomMovementMode 。
	UFUNCTION() void OnMovementModeChange(class ACharacter* Character, EMovementMode PrevMovementMode, uint8 PreviousCustomMode);

#elif 0 // if LYRA_WALLRUN_STAMINA_IN_GA
#endif

	//~End delegate functions
	//~End UFUNCTION

private:
#if 1 // if LYRA_WALLRUN_STAMINA_IN_SAVED_MOVE
#elif 0 // if LYRA_WALLRUN_STAMINA_IN_COMPONENT
#elif 0 // if LYRA_WALLRUN_STAMINA_IN_GA
	// @brief 実行可能状態を GA に連絡。
	// @param bEnableWallRun WallRun が実行可能かどうか。
	void BroadcastWallRunEnableMessage(bool bEnableWallRun);

#endif

	//~UPROPERTY
	//~Blueprint accessible properties
protected:
#if 1 // if LYRA_WALLRUN_STAMINA_IN_SAVED_MOVE
#elif 0 // LYRA_WALLRUN_STAMINA_IN_COMPONENT
	// @brief スタミナの状況。設定用の構造体を内包するので、 Blueprint で設定可能にしている。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "LyraWR|WallRunStamina")
	FSafeAutoRecoverableAttribute	Stamina;

#elif 0 // LYRA_WALLRUN_STAMINA_IN_GA
	// @brief スタミナの設定。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "LyraWR|WallRunStamina")
	FAutoRecoverableAttributeSetting	StaminaSettings;

#endif
	//~End Blueprint accessible properties

private:
	// @brief MovementComponent へのキャッシュ。
	UPROPERTY()
	TObjectPtr<class ULyraWRCharacterMovementComponent> LyraWRCharacterMovementComponent;

	//~End UPROPERTY

};

