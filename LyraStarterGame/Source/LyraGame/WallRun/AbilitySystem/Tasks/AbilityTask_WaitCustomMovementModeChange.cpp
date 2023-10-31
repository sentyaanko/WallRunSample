// Copyright Epic Games, Inc. All Rights Reserved.
// Copyright 2023 Sentya Anko
// Copy from AbilityTask_WaitMovementModeChange.cpp

#include "AbilityTask_WaitCustomMovementModeChange.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AbilityTask_WaitCustomMovementModeChange)

UAbilityTask_WaitCustomMovementModeChange::UAbilityTask_WaitCustomMovementModeChange(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UAbilityTask_WaitCustomMovementModeChange* UAbilityTask_WaitCustomMovementModeChange::CreateWaitMovementModeChange(class UGameplayAbility* OwningAbility, FCheckMovementModeChangedDelegate IsChanged, bool TriggerOnce)
{
	UAbilityTask_WaitCustomMovementModeChange* MyObj = NewAbilityTask<UAbilityTask_WaitCustomMovementModeChange>(OwningAbility);
	MyObj->CheckStatus = IsChanged;
	MyObj->TriggerOnce = TriggerOnce;
	return MyObj;
}

void UAbilityTask_WaitCustomMovementModeChange::Activate()
{
	if (auto Character = Cast<ACharacter>(GetAvatarActor()))
	{
		Character->MovementModeChangedDelegate.AddDynamic(this, &UAbilityTask_WaitCustomMovementModeChange::OnMovementModeChange);
		MyCharacter = Character;
	}

	SetWaitingOnAvatar();
}

void UAbilityTask_WaitCustomMovementModeChange::OnMovementModeChange(ACharacter * Character, EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	if (Character)
	{
		if (UCharacterMovementComponent *MoveComp = Cast<UCharacterMovementComponent>(Character->GetMovementComponent()))
		{
			if (ShouldBroadcastAbilityTaskDelegates())
			{
				int status = 0;
				if(CheckStatus.IsBound())
				{
					status = CheckStatus.Execute(MoveComp->MovementMode, MoveComp->CustomMovementMode, PrevMovementMode, PreviousCustomMode);
					if(!status)
					{
						return;
					}
				}
				{
					FScopedPredictionWindow ScopedPrediction(AbilitySystemComponent.Get(), IsPredictingClient());
					OnChange.Broadcast(status);
				}
				if (TriggerOnce)
				{
					EndTask();
				}
				return;
			}
		}
	}
}

void UAbilityTask_WaitCustomMovementModeChange::OnDestroy(bool AbilityEnded)
{
	if (MyCharacter.IsValid())
	{
		MyCharacter->MovementModeChangedDelegate.RemoveDynamic(this, &UAbilityTask_WaitCustomMovementModeChange::OnMovementModeChange);
	}

	Super::OnDestroy(AbilityEnded);
}

