// Copyright Epic Games, Inc. All Rights Reserved.
// Copyright 2023 Sentya Anko

#include "WallRun/LyraWallRunStaminaComponent.h"
#include "LyraWallRunStaminaMessage.h"


#include "Kismet/KismetSystemLibrary.h"

#include "LyraLogChannels.h"
#include "System/LyraAssetManager.h"
#include "System/LyraGameData.h"
#include "LyraGameplayTags.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "AbilitySystem/LyraAbilitySystemComponent.h"
#include "Messages/LyraVerbMessage.h"
#include "Messages/LyraVerbMessageHelpers.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraWallRunStaminaComponent)

//------------------------------------------------------------------------------

#if LYRA_WALLRUN_STAMINA_IN_SAVED_MOVE

#elif LYRA_WALLRUN_STAMINA_IN_COMPONENT
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Ability_WallRun_Stamina_Message, "Ability.WallRun.Stamina.Message");

#elif LYRA_WALLRUN_STAMINA_IN_GA
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Ability_WallRun_Enabled_Message, "Ability.WallRun.Enabled.Message");

#endif


//-----------------------------------------------------------------------------


ULyraWallRunStaminaComponent::ULyraWallRunStaminaComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
#if LYRA_WALLRUN_STAMINA_IN_SAVED_MOVE
#elif LYRA_WALLRUN_STAMINA_IN_COMPONENT
	, Stamina()
#elif LYRA_WALLRUN_STAMINA_IN_GA
	, StaminaSettings()
#endif
	, LyraWRCharacterMovementComponent()
{
#if LYRA_WALLRUN_STAMINA_IN_SAVED_MOVE || LYRA_WALLRUN_STAMINA_IN_GA
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;
#elif LYRA_WALLRUN_STAMINA_IN_COMPONENT
	// このコンポーネントでスタミナ管理する場合は Tick 関数を有効にする。
	// Tick 関数内では（消費や回復など、時間で変化する）スタミナの更新を行っている。
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.bCanEverTick = true;
#endif
}

#if LYRA_WALLRUN_STAMINA_IN_SAVED_MOVE
#elif LYRA_WALLRUN_STAMINA_IN_COMPONENT
void ULyraWallRunStaminaComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	if (LyraWRCharacterMovementComponent)
	{
		auto func = [this](float CurrentValue, float AddValuePerSec, float Duration, bool bFinished)->void
			{
				FLyraWallRunStaminaMessage Message;
				Message.Instigator = GetOwner();
				Message.CurrentValue = CurrentValue;
				Message.AddValuePerSec = AddValuePerSec;
				Message.Duration = Duration;
				Message.bFinished = bFinished;
				UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(GetWorld());
				MessageSystem.BroadcastMessage(TAG_Ability_WallRun_Stamina_Message, Message);
			};
		auto bPreWallRunEnable = !Stamina.GetSaved().bOverheat;
		Stamina.OnUpdate(LyraWRCharacterMovementComponent->GetWallRunStatus() != EWallRunStatus::WRS_None, DeltaTime, func);
		auto bWallRunEnable = !Stamina.GetSaved().bOverheat;
		if (bWallRunEnable != bPreWallRunEnable)
		{
			LyraWRCharacterMovementComponent->SetWallRunEnable(bWallRunEnable);
		}
		else if (bWallRunEnable != LyraWRCharacterMovementComponent->IsWallRunEnable())
		{
			//値が戻されていた場合は設定し直す。
			LyraWRCharacterMovementComponent->SetWallRunEnable(bWallRunEnable);
		}
	}
}
#elif LYRA_WALLRUN_STAMINA_IN_GA
#endif

void ULyraWallRunStaminaComponent::BeginPlay()
{
	Super::BeginPlay();
}

void ULyraWallRunStaminaComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void ULyraWallRunStaminaComponent::OnRegister()
{
	Super::OnRegister();

	//UE_LOG(LogLyra, Log, TEXT("WallRun LyraWallRunStaminaComponent::OnRegister()"));
	if (auto OwnerCharacter = Cast<ACharacter>(GetOwner()))
	{
		LyraWRCharacterMovementComponent = Cast<ULyraWRCharacterMovementComponent>(OwnerCharacter->GetCharacterMovement());
	}
	if (!LyraWRCharacterMovementComponent)
	{
		UE_LOG(LogLyra, Warning, TEXT("WallRun LyraWallRunStaminaComponent::OnRegister() LyraWRCharacterMovementComponent is null."));
	}

#if LYRA_WALLRUN_STAMINA_IN_SAVED_MOVE
#elif LYRA_WALLRUN_STAMINA_IN_COMPONENT
	auto Owner = GetOwner();
	if (auto OwnerCharacter = Cast<ACharacter>(Owner))
	{
		OwnerCharacter->MovementModeChangedDelegate.AddDynamic(this, &ULyraWallRunStaminaComponent::OnMovementModeChange);
	}
#elif LYRA_WALLRUN_STAMINA_IN_GA
#endif
}

void ULyraWallRunStaminaComponent::OnUnregister()
{
#if LYRA_WALLRUN_STAMINA_IN_SAVED_MOVE
#elif LYRA_WALLRUN_STAMINA_IN_COMPONENT
	auto Owner = GetOwner();
	if (auto OwnerCharacter = Cast<ACharacter>(Owner))
	{
		OwnerCharacter->MovementModeChangedDelegate.RemoveDynamic(this, &ULyraWallRunStaminaComponent::OnMovementModeChange);
	}
#elif LYRA_WALLRUN_STAMINA_IN_GA
#endif

	LyraWRCharacterMovementComponent = nullptr;

	Super::OnUnregister();
}

const FAutoRecoverableAttributeSetting& ULyraWallRunStaminaComponent::GetWallRunSettings()const
{
	UE_LOG(LogLyra, Log, TEXT("WallRun LyraWallRunStaminaComponent::GetWallRunSettings()"));

#if LYRA_WALLRUN_STAMINA_IN_SAVED_MOVE
	static FAutoRecoverableAttributeSetting nullobject;
	if (!LyraWRCharacterMovementComponent)
	{
		//以下の場合などにここに来る
		//	* MovementComponent が WallRun 対応版ではない。
		//	* OnRegister() 呼び出し前にこの関数が呼ばれている。
		//		* SAVED_MOVE 版の場合、 Settings は MovementComponent 内で保持している。
		//		* そのため、すべてのコンポーネントの登録完了のタイミングである OnRegister() より前にこの関数を呼び出してはいけない。
		UE_LOG(LogLyra, Warning, TEXT("WallRun LyraWallRunStaminaComponent::GetWallRunSettings() LyraWRCharacterMovementComponent is null."));
		FDebug::DumpStackTraceToLog(ELogVerbosity::Warning);
	}
	return LyraWRCharacterMovementComponent ? LyraWRCharacterMovementComponent->GetWallRunSettings() : nullobject;
#elif LYRA_WALLRUN_STAMINA_IN_COMPONENT
	return Stamina.Settings;
#elif LYRA_WALLRUN_STAMINA_IN_GA
	return StaminaSettings;
#endif
}

#if LYRA_WALLRUN_STAMINA_IN_SAVED_MOVE
#elif LYRA_WALLRUN_STAMINA_IN_COMPONENT
void ULyraWallRunStaminaComponent::OnMovementModeChange(class ACharacter* Character, EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	if (auto cmc = Character->GetCharacterMovement())
	{
		auto CurrentMovementMode = cmc->MovementMode;
		auto CurrentCustomMode = cmc->CustomMovementMode;
		auto PrevIsWallRun = ULyraWRCharacterMovementComponent::IsWallRunMode(PrevMovementMode, PreviousCustomMode);
		auto CurrentIsWallRun = ULyraWRCharacterMovementComponent::IsWallRunMode(CurrentMovementMode, CurrentCustomMode);
		if (PrevIsWallRun != CurrentIsWallRun)
		{
			auto func = [this](float CurrentValue, float AddValuePerSec, float Duration, bool bFinished)->void
				{
					FLyraWallRunStaminaMessage Message;
					Message.Instigator = GetOwner();
					Message.CurrentValue = CurrentValue;
					Message.AddValuePerSec = AddValuePerSec;
					Message.Duration = Duration;
					Message.bFinished = bFinished;
					UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(GetWorld());
					MessageSystem.BroadcastMessage(TAG_Ability_WallRun_Stamina_Message, Message);
				};
			Stamina.OnStatusChanged(CurrentIsWallRun, func);
		}
	}
}
#elif LYRA_WALLRUN_STAMINA_IN_GA
#endif

#if LYRA_WALLRUN_STAMINA_IN_SAVED_MOVE || LYRA_WALLRUN_STAMINA_IN_COMPONENT
void ULyraWallRunStaminaComponent::RequestSetWallRunEnable(bool bEnableWallRun)
{
	// GA で利用しているため、削除すると GA でエラーが発生してしまう。
	// 回避のため定義はしておく。
	UKismetSystemLibrary::PrintString(this, FString::Printf(TEXT("WallRun RequestSetWallRunEnable() is not work.")), false);
}

#elif LYRA_WALLRUN_STAMINA_IN_GA

void ULyraWallRunStaminaComponent::RequestSetWallRunEnable(bool bEnableWallRun)
{
	UKismetSystemLibrary::PrintString(this, FString::Printf(TEXT("WallRun RequestSetWallRunEnable()")), false);

	const auto LocallyControlled = Cast<ACharacter>(GetOwner())->IsLocallyControlled();
	const auto Authority = GetOwner()->HasAuthority();
	if (Authority)
	{
		// サーバー。自身の更新をする。
		LyraWRCharacterMovementComponent->SetWallRunEnable(bEnableWallRun);
		if (LocallyControlled)
		{
			//Listen サーバーが制御している Pawn
			//widget の更新も必要(GA でやるのでここではなにもしない)
		}
		else
		{
			// ローカル制御していない場合はクライアント RPC を呼ぶ。
			Client_SetWallRunEnable(bEnableWallRun);
		}
	}
	else
	{
		//クライアント
		if (LocallyControlled)
		{
			//自身で制御している Pawn 。
			if (bEnableWallRun)
			{
				//有効化は何もしない（ユーザー有利になる処理をクライアントにさせるのは信用ならないため、サーバーからの通知を待つ）。
			}
			else
			{
				//無効化は即時行う
				LyraWRCharacterMovementComponent->SetWallRunEnable(bEnableWallRun);

				//widget の更新も必要(GA でやるのでここではなにもしない)

				//Server RPC で連絡する。
				Server_SetWallRunEnable(bEnableWallRun);
			}
		}
		else
		{
			//通常、ここにはこない。
		}
	}
}

void ULyraWallRunStaminaComponent::Client_SetWallRunEnable_Implementation(bool bEnableWallRun)
{
	LyraWRCharacterMovementComponent->SetWallRunEnable(bEnableWallRun);

	//GA に有効状態の変更を通知する（スタミナ値をリフレッシュする）
	BroadcastWallRunEnableMessage(bEnableWallRun);

	//widget の更新も必要(GA でやるのでここではなにもしない)
}

void ULyraWallRunStaminaComponent::Server_SetWallRunEnable_Implementation(bool bEnableWallRun)
{
	UKismetSystemLibrary::PrintString(this, FString::Printf(TEXT("WallRun Server_SetWallRunEnable()")), false);

	const auto LocallyControlled = Cast<ACharacter>(GetOwner())->IsLocallyControlled();

	//サーバーが制御していないキャラクターのみ受け取る。
	if (!LocallyControlled)
	{
		//クライアントからの変更通知は（信用ならないので）無効化のみ受け取る。
		if (!bEnableWallRun)
		{
			LyraWRCharacterMovementComponent->SetWallRunEnable(bEnableWallRun);

			//GA に有効状態の変更を通知する（スタミナ値をリフレッシュする）
			BroadcastWallRunEnableMessage(bEnableWallRun);
		}
	}
}

void ULyraWallRunStaminaComponent::BroadcastWallRunEnableMessage(bool bEnableWallRun)
{
	//GA に連絡を入れる
	FLyraWallRunEnabledMessage Message;
	Message.Instigator = GetOwner();
	Message.bEnabled = bEnableWallRun;
	UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(GetWorld());
	MessageSystem.BroadcastMessage(TAG_Ability_WallRun_Enabled_Message, Message);
}

#endif
