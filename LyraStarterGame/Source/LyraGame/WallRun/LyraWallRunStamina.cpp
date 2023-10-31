// Copyright 2023 Sentya Anko


#include "LyraWallRunStamina.h"

//------------------------------------------------------------------------------
FSavedAutoRecoverableAttribute::FSavedAutoRecoverableAttribute(float MaxValue)
	: bOverheat(false)
#if 0
	, bStatusChanged(false)
	, bStartConsume(false)
#endif
	, CurrentValue(MaxValue)
	, BaseValue(MaxValue)
	, TotalDeltaSeconds(0.f)
	, CurrentCooldownSeconds(0.f)
	, BaseCooldownSeconds(0.f)
	, TotalCooldownDeltaSeconds(0.f)
{
}

bool FSavedAutoRecoverableAttribute::CanCombineWith(const FSavedAutoRecoverableAttribute& lhs, const FSavedAutoRecoverableAttribute& rhs)
{
	//オーバーヒートの状態が異なる場合は結合しない
	if (lhs.bOverheat != rhs.bOverheat)
	{
		return false;
	}
	//値/クールダウンが増減しているかの状態が異なる場合は結合しない
	// Don't combine on changes to/from zero TotalDeltaSeconds.
	if ((lhs.TotalDeltaSeconds == 0.f) != (rhs.TotalDeltaSeconds == 0.f))
	{
		return false;
	}
	// Don't combine on changes to/from zero TotalCooldownDeltaSeconds.
	if ((lhs.TotalCooldownDeltaSeconds == 0.f) != (rhs.TotalCooldownDeltaSeconds == 0.f))
	{
		return false;
	}
	return true;
}

//------------------------------------------------------------------------------
FSafeAutoRecoverableAttribute::FSafeAutoRecoverableAttribute()
	: Settings()
	, Saved(Settings.MaxValue)
{
}

FSavedAutoRecoverableAttribute& FSafeAutoRecoverableAttribute::GetSaved()
{
	return Saved;
}

const FSavedAutoRecoverableAttribute& FSafeAutoRecoverableAttribute::GetSaved()const
{
	return Saved;
}

void FSafeAutoRecoverableAttribute::OnUpdate(bool bConsume, float DeltaSeconds, TFunctionRef<void(float,float,float,bool)> Notify)
{
#if 0
	//クールダウンの更新。
	if (Saved.bStatusChanged)
	{
		Saved.CurrentCooldownSeconds = Saved.bStartConsume ? 0.f: Settings.CooldownTime;
		Saved.BaseCooldownSeconds = Saved.CurrentCooldownSeconds;
		/*Saved().*/Saved.TotalCooldownDeltaSeconds = 0.f;

		/*Saved().*/Saved.TotalDeltaSeconds = 0.f;
		Saved.BaseValue = Saved.CurrentValue;

		//実行状態が変わったので連絡をする
		if(Saved.bStartConsume)
		{
			//消費開始
			const auto AddValuePerSec = -Settings.Consume;
			const auto Duration = (Settings.MinValue - Saved.CurrentValue) / AddValuePerSec;
			Notify(Saved.CurrentValue, AddValuePerSec, Duration, false);
		}
		else
		{
			//現在値の fix
			Notify(Saved.CurrentValue, 0, 0, false);
		}

		Saved.bStatusChanged = false;
		Saved.bStartConsume = false;
	}
	else 
#endif
	if (Saved.CurrentCooldownSeconds != 0.f)
	{
		/*Saved().*/Saved.TotalCooldownDeltaSeconds += DeltaSeconds;
		Saved.CurrentCooldownSeconds = FMath::Max(0.f, Saved.BaseCooldownSeconds - /*Saved().*/Saved.TotalCooldownDeltaSeconds);
		if (Saved.CurrentCooldownSeconds == 0.f)
		{
			//値のクリア
			/*Saved().*/Saved.TotalCooldownDeltaSeconds = 0.f;

			//クールダウンが終わったので回復開始の連絡をする
			const auto AddValuePerSec = Saved.bOverheat ? Settings.RecoverOverheat : Settings.RecoverDefault;
			const auto Duration = (Settings.MaxValue - Saved.CurrentValue) / AddValuePerSec;
			Notify(Saved.CurrentValue, AddValuePerSec, Duration, false);
		}
	}

	//消費しているか。
	if (bConsume)
	{
		// 値を減らす。
		if (Saved.CurrentValue > Settings.MinValue)
		{
			/*Saved().*/Saved.TotalDeltaSeconds += DeltaSeconds;

			Saved.CurrentValue = FMath::Max(Settings.MinValue, Saved.BaseValue - /*Saved().*/Saved.TotalDeltaSeconds * Settings.Consume);
			if (Saved.CurrentValue == Settings.MinValue)
			{
				/*Saved().*/Saved.TotalDeltaSeconds = 0.f;
				Saved.BaseValue = Saved.CurrentValue;

				// 値が尽きたらオーバーヒートし、クールダウンを設定する。
				Saved.bOverheat = true;
				Saved.CurrentCooldownSeconds = Settings.CooldownTime;
				Saved.BaseCooldownSeconds = Saved.CurrentCooldownSeconds;
				/*Saved().*/Saved.TotalCooldownDeltaSeconds = 0.f;

				//連絡をする
				Notify(Saved.CurrentValue, 0, 0, true);
			}
		}
	}
	else if (Saved.CurrentCooldownSeconds == 0.f)
	{
		//消費しておらず、クールダウン中でもない。値を回復させる。
		if (Saved.CurrentValue < Settings.MaxValue)
		{
			/*Saved().*/Saved.TotalDeltaSeconds += DeltaSeconds;

			Saved.CurrentValue = FMath::Min(Settings.MaxValue, Saved.BaseValue + /*Saved().*/Saved.TotalDeltaSeconds * (Saved.bOverheat ? Settings.RecoverOverheat : Settings.RecoverDefault));
			if (Saved.CurrentValue == Settings.MaxValue)
			{
				/*Saved().*/Saved.TotalDeltaSeconds = 0.f;
				Saved.BaseValue = Saved.CurrentValue;

				//回復しきった連絡をする。オーバーヒート状態が終了かどうかと同値になるのでそもまま渡す。
				Notify(Saved.CurrentValue, 0, 0, Saved.bOverheat);

				// オーバーヒート中だったら解除する。
				if (Saved.bOverheat)
				{
					Saved.bOverheat = false;
				}
			}
		}
	}
}

void FSafeAutoRecoverableAttribute::OnStatusChanged(bool bConsume, TFunctionRef<void(float, float, float, bool)> Notify)
{
#if 0
	Saved.bStatusChanged = true;
	Saved.bStartConsume = bConsume;
#else
	Saved.CurrentCooldownSeconds = bConsume ? 0.f : Settings.CooldownTime;
	Saved.BaseCooldownSeconds = Saved.CurrentCooldownSeconds;
	/*Saved().*/Saved.TotalCooldownDeltaSeconds = 0.f;

	/*Saved().*/Saved.TotalDeltaSeconds = 0.f;
	Saved.BaseValue = Saved.CurrentValue;

	//実行状態が変わったので連絡をする
	if (bConsume)
	{
		//消費開始
		const auto AddValuePerSec = -Settings.Consume;
		const auto Duration = (Settings.MinValue - Saved.CurrentValue) / AddValuePerSec;
		Notify(Saved.CurrentValue, AddValuePerSec, Duration, false);
	}
	else
	{
		//オーバーヒート時の通知はすでにしているのでオーバーヒートでない場合のみ連絡する。
		if (!Saved.bOverheat)
		{
			//現在値の fix
			Notify(Saved.CurrentValue, 0, 0, false);
		}
	}
#endif
}

