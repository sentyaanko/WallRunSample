// Copyright 2023 Sentya Anko


#include "Character/LyraWRCharacterMovementComponent.h"
#include "Character/LyraCharacter.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "Net/UnrealNetwork.h"

//Helper Macros

#if 1
#include "DrawDebugHelpers.h"

float MacroDuration = 10.f;
#define SLOG(x) GEngine->AddOnScreenDebugMessage(-1, MacroDuration? MacroDuration: -1.f, FColor::Yellow, x)
#define POINT(x, c) DrawDebugPoint(GetWorld(), x, 10, c, !MacroDuration, MacroDuration)
#define LINE(x1, x2, c) DrawDebugLine(GetWorld(), x1, x2, c, !MacroDuration, MacroDuration)
#define CAPSULE(x, c) DrawDebugCapsule(GetWorld(), x, CapHH(), CapR(), FQuat::Identity, c, !MacroDuration, MacroDuration)
#define SPHERE(x, r, c) DrawDebugSphere(GetWorld(), x, r, 12, c, !MacroDuration, MacroDuration)
#else
#define SLOG(x)
#define POINT(x, c)
#define LINE(x1, x2, c)
#define CAPSULE(x, c)
#define SPHERE(x, r, c)
#endif

#if 1
#define WALLRUN_SLOG		SLOG
#define WALLRUN_POINT		POINT
#define WALLRUN_LINE		LINE
#define WALLRUN_CAPSULE		CAPSULE
#else
#define WALLRUN_SLOG(x)
#define WALLRUN_POINT(x, c)
#define WALLRUN_LINE(x1, x2, c)
#define WALLRUN_CAPSULE(x, c)
#endif


ULyraWRCharacterMovementComponent::ULyraWRCharacterMovementComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, WallRunNormal(0.f)
{
	//Maximum distance character is allowed to lag behind server location when interpolating between updates.
	//更新の間を補間する際に、キャラクターがサーバーの位置から遅れることを許容する最大距離。
	//基底クラスのデフォルト値は  256.f 。
	NetworkMaxSmoothUpdateDistance = 92.f;

	//Maximum distance beyond which character is teleported to the new server location without any smoothing.
	//キャラクターがスムージングなしで新しいサーバー位置にテレポートされる最大距離。
	//基底クラスのデフォルト値は  384.f 。
	NetworkNoSmoothUpdateDistance = 140.f;
}

void ULyraWRCharacterMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
	//WallRun
	if (IsFalling())
	{
		TryWallRun();
	}

	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);
}

void ULyraWRCharacterMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{
	Super::PhysCustom(deltaTime, Iterations);

	switch (CustomMovementMode)
	{
	case CMOVE_WallRunLeft:
	case CMOVE_WallRunRight:
		PhysWallRun(deltaTime, Iterations);
		break;
	default:
		UE_LOG(LogTemp, Fatal, TEXT("Invalid Movement Mode"));
		break;
	}
}

void ULyraWRCharacterMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
	if (PreviousMovementMode == MOVE_Custom)
	{
		switch (PreviousCustomMode)
		{
		case CMOVE_WallRunLeft:
		case CMOVE_WallRunRight:
			WallRunNormal = FVector::ZeroVector;
			break;
		}
	}
}

float ULyraWRCharacterMovementComponent::GetMaxBrakingDeceleration() const
{
	if (MovementMode != MOVE_Custom)
	{
		return Super::GetMaxBrakingDeceleration();
	}
	switch (CustomMovementMode)
	{
	case CMOVE_WallRunLeft:
	case CMOVE_WallRunRight:
		return 0.f;
	default:
		UE_LOG(LogTemp, Fatal, TEXT("Invalid Movement Mode"));
		return -1.f;
	}
}

bool ULyraWRCharacterMovementComponent::DoJump(bool bReplayingMoves)
{
	//WallRun の状態
	//MovementMode を元に方向を調べるので、 DoJump の前に取得しておく
	auto WallRunStatus = GetWallRunStatus();
	if (Super::DoJump(bReplayingMoves))
	{
		if(WallRunStatus != EWallRunStatus::WRS_None)
		{
			// FCollisionQueryParams などの取得(CollisionShape はここでは使わないので省略)
			auto work = WallRun_InitWork(false);

			//WallRun 中にジャンプしたら、壁の法線方向に初速を与える
			if (WallRunCollision_LineTraceWall(work, WallRunStatus))
			{
				//FVector Normal2D(work.Hit.Normal.X, work.Hit.Normal.Y, 0.f);
				auto Normal2D = work.Hit.Normal.GetSafeNormal2D();
				if (!Normal2D.IsNearlyZero())
				{
					Velocity += Normal2D * WallRunJumpWallNormalInitialVelocity;
				}
			}
		}
		return true;
	}
	return false;
}

bool ULyraWRCharacterMovementComponent::CanAttemptJump() const
{
	return Super::CanAttemptJump() || (GetWallRunStatus() != EWallRunStatus::WRS_None);
}

float ULyraWRCharacterMovementComponent::GetMaxSpeed() const
{
	if (MovementMode != MOVE_Custom)
	{
		return Super::GetMaxSpeed();
	}
	switch (CustomMovementMode)
	{
	case CMOVE_WallRunLeft:
	case CMOVE_WallRunRight:
		return MaxWallRunSpeed;
	default:
		UE_LOG(LogTemp, Fatal, TEXT("Invalid Movement Mode"));
		return -1.f;
	}
}

bool ULyraWRCharacterMovementComponent::IsCustomMovementMode(ECustomMovementMode InCustomMovementMode) const
{
	return MovementMode == MOVE_Custom && CustomMovementMode == InCustomMovementMode;
}

bool ULyraWRCharacterMovementComponent::IsMovementMode(EMovementMode InMovementMode) const
{
	return MovementMode == InMovementMode;
}

EWallRunStatus ULyraWRCharacterMovementComponent::GetWallRunStatus()const
{
	if (MovementMode == MOVE_Custom)
	{
		switch (CustomMovementMode)
		{
		case CMOVE_WallRunLeft:		return EWallRunStatus::WRS_Left;
		case CMOVE_WallRunRight:	return EWallRunStatus::WRS_Right;
		}
	}
	return EWallRunStatus::WRS_None;
}

bool ULyraWRCharacterMovementComponent::TryWallRun()
{
	if (!IsFalling())
		return false;

	//平面速度が足りない or 落下速度が速いと失敗
	if (!WallRun_IsEnoughVelocity(Velocity, true))
		return false;

	// FCollisionQueryParams などの取得(CollisionShape はここでは使わないので省略)
	auto work = WallRun_InitWork(false);

	//床が近いと失敗
	if (WallRunCollision_LineTraceFloor(work))
		return false;

	//壁が見つからないと失敗
	auto WallRunStatus = WallRunCollision_LineTraceWallAndUpdateIsRight(work, Velocity);
	if (WallRunStatus == EWallRunStatus::WRS_None)
		return false;

	//壁に投影した速度（つまりは壁に沿った速度）の平面速度が足りないと失敗
	auto ProjectedVelocity = FVector::VectorPlaneProject(Velocity, work.Hit.Normal);
	if (!WallRun_IsEnoughVelocity2D(ProjectedVelocity))
		return false;

	//Passed all conditions

	//Phys 関数のために Velocity を書き換えておく
	Velocity = ProjectedVelocity;
	Velocity.Z = FMath::Clamp(Velocity.Z, 0.f, MaxVerticalUpWallRunSpeed);
	SetMovementMode(MOVE_Custom, WallRunStatus == EWallRunStatus::WRS_Right ? CMOVE_WallRunRight : CMOVE_WallRunLeft);
	WallRunNormal = work.Hit.Normal;
	//	WALLRUN_SLOG("StartingWallRun");
	return true;
}

void ULyraWRCharacterMovementComponent::PhysWallRun(float deltaTime, int32 Iterations)
{
	// this code is copied from PhysWalking()

	if (deltaTime < MIN_TICK_TIME)
	{
		return;
	}

	if (!CharacterOwner || (!CharacterOwner->Controller && !bRunPhysicsWithNoController && !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity() && (CharacterOwner->GetLocalRole() != ROLE_SimulatedProxy)))
	{
		Acceleration = FVector::ZeroVector;
		Velocity = FVector::ZeroVector;
		return;
	}

	//CustomMovementMode から向きを取得
	const auto WallRunStatus = GetWallRunStatus();
	if (WallRunStatus == EWallRunStatus::WRS_None)
	{
		//向きが取れない場合は終わらす
		SetMovementMode(MOVE_Falling);
		StartNewPhysics(deltaTime, Iterations);
		return;
	}

	bJustTeleported = false;
	float remainingTime = deltaTime;

	// FCollisionQueryParams などの取得
	auto work = WallRun_InitWork(true);

	// Perform the move
#if 0 // PhysWalking() original
	while ((remainingTime >= MIN_TICK_TIME) && (Iterations < MaxSimulationIterations) && CharacterOwner && (CharacterOwner->Controller || bRunPhysicsWithNoController || HasAnimRootMotion() || CurrentRootMotion.HasOverrideVelocity() || (CharacterOwner->GetLocalRole() == ROLE_SimulatedProxy)))
#else
	//remove root motion
	while ((remainingTime >= MIN_TICK_TIME) && (Iterations < MaxSimulationIterations) && CharacterOwner && (CharacterOwner->Controller || bRunPhysicsWithNoController || (CharacterOwner->GetLocalRole() == ROLE_SimulatedProxy)))
#endif
	{
		Iterations++;
		bJustTeleported = false;
		const float timeTick = GetSimulationTimeStep(remainingTime, Iterations);
		remainingTime -= timeTick;

		work.UpdatedComponentLocation = UpdatedComponent->GetComponentLocation();
		work.UpdatedComponentRightVector = UpdatedComponent->GetRightVector();
		const auto OldLocation = work.UpdatedComponentLocation;

		//壁があるかチェック
		if (!WallRunCollision_IsWallFound(work, WallRunStatus, Acceleration))
		{
			SetMovementMode(MOVE_Falling);
			//移動処理前なので、 remainingTime と Iterations を消費前の値に戻す
			//StartNewPhysics(remainingTime, Iterations);
			StartNewPhysics(remainingTime+timeTick, Iterations-1);
			return;
		}
		//壁に沿う移動後、壁に寄る際に参照
		auto WallNormal = work.Hit.Normal;

		//速度不足で Falling に移行する際に値を戻せるように取っておく
		auto preAcceleration = Acceleration;
		auto preVelocity = Velocity;

		//Clamp Acceleration
		//Acceleration を壁に投影し、 Z 軸成分を消す
		Acceleration = FVector::VectorPlaneProject(Acceleration, WallNormal);
		Acceleration.Z = 0.f;

		//Apply acceralation
		//Acceleration と Velocity の更新
		CalcVelocity(timeTick, 0.f, false, GetMaxBrakingDeceleration());

		//Velocity を壁に投影する
		Velocity = FVector::VectorPlaneProject(Velocity, WallNormal);

		//移動方向と加速方向を元に、落下速度の係数を決める
		Velocity.Z += GetGravityZ() * WallRun_GetGravityWall(Acceleration, Velocity) * timeTick;

		//Velocity が WallRun できる値か
		if (!WallRun_IsEnoughVelocity(Velocity))
		{
			//加工前の値に戻す。基底クラスではこういったことをしていないのでおそらく不要だが念のため。
			Acceleration = preAcceleration;
			Velocity = preVelocity;
			
			SetMovementMode(MOVE_Falling);
			//移動処理前なので、 remainingTime と Iterations を消費前の値に戻す
			//StartNewPhysics(remainingTime, Iterations);
			StartNewPhysics(remainingTime+timeTick, Iterations-1);
			return;
		}

		//Compute move paramteters
		const auto Delta = timeTick * Velocity;
		const auto bZeroDelta = Delta.IsNearlyZero();
		if (bZeroDelta)
		{
			remainingTime = 0.f;
		}
		else
		{
#if 0 // delgoodie original
			FHitResult Hit;
			SafeMoveUpdatedComponent(Delta, UpdatedComponent->GetComponentQuat(), true, Hit);
			auto WallAttractionDelta = -WallNormal * WallRunAttractionVelocityScale * timeTick;
			SafeMoveUpdatedComponent(WallAttractionDelta, UpdatedComponent->GetComponentQuat(), true, Hit);
#else
			// 壁に押し付けてる都合上、壁と壁のエッジに詰まることがあるため、壁沿いに移動する前に壁から少しだけ離れる
			SafeMoveUpdatedComponent(WallNormal * (timeTick * WallRunAwayFromWallBeforeMoveingVelocityScale), UpdatedComponent->GetComponentQuat(), true, work.Hit);

			//壁に沿って移動する。
			SafeMoveUpdatedComponent(Delta, UpdatedComponent->GetComponentQuat(), true, work.Hit);

			//移動がブロックされている場合
			if (work.Hit.bBlockingHit)
			{
				//壁をぶつかったところに変更
				WallNormal = work.Hit.Normal;

				//予定していた移動量と実際の移動量を元に、ぶつかった壁沿いの移動量の算出
				const auto Delta2 = WallRun_CalcDeltaAfterBlocked(WallRunStatus, Delta, work.Hit.Location - OldLocation, work.Hit.Normal);
				if (!Delta2.IsNearlyZero())
				{
					//壁に沿って移動する。
					SafeMoveUpdatedComponent(Delta2, UpdatedComponent->GetComponentQuat(), true, work.Hit);
				}
			}

			//壁方向に押し付ける
			SafeMoveUpdatedComponent(-WallNormal * (timeTick * WallRunAttractionVelocityScale * work.ScaledCapsuleRadius), UpdatedComponent->GetComponentQuat(), true, work.Hit);

			//壁の法線を保存しておく
			WallRunNormal = WallNormal;
#endif
			work.UpdatedComponentLocation = UpdatedComponent->GetComponentLocation();
			work.UpdatedComponentRightVector = UpdatedComponent->GetRightVector();
		}

		// If we didn't move at all this iteration then abort (since future iterations will also be stuck).
		if (work.UpdatedComponentLocation == OldLocation)
		{
			//UE_LOG(LogTemp, Warning, TEXT("Not update location."));
			remainingTime = 0.f;
			break;
		}
		Velocity = (work.UpdatedComponentLocation - OldLocation) / timeTick;
	}

	if (WallRunCollision_IsFinished(work, Velocity, WallRunStatus))
	{
		SetMovementMode(MOVE_Falling);
	}
}

inline bool ULyraWRCharacterMovementComponent::WallRunCollision_LineTrace(FWallRunCollisionWork& work, const FVector& ToEnd)const
{
	if (ToEnd.IsNearlyZero())
		return false;
	return GetWorld()->LineTraceSingleByProfile(work.Hit, work.UpdatedComponentLocation, work.UpdatedComponentLocation + ToEnd, "BlockAll", work.IgnoreCharacterParams);
}

inline bool ULyraWRCharacterMovementComponent::WallRunCollision_Sweep(FWallRunCollisionWork& work, const FVector& ToEnd)const
{
	if (ToEnd.IsNearlyZero())
		return false;
	return GetWorld()->SweepSingleByProfile(work.Hit, work.UpdatedComponentLocation, work.UpdatedComponentLocation + ToEnd, UpdatedComponent->GetComponentQuat(), "BlockAll", work.CollisionShape, work.IgnoreCharacterParams);
}

inline bool ULyraWRCharacterMovementComponent::WallRunCollision_LineTraceFloor(FWallRunCollisionWork& work) const
{
	auto scale = work.ScaledCapsuleHalfHeight + MinWallRunHeight * 0.5f;
	if (FMath::IsNearlyZero(scale))
		return false;
	return WallRunCollision_LineTrace(work, FVector::DownVector * scale);
}

inline bool ULyraWRCharacterMovementComponent::WallRunCollision_LineTraceWall(FWallRunCollisionWork& work, EWallRunStatus WallRunStatus) const
{
	check(WallRunStatus != EWallRunStatus::WRS_None);

	auto scale = WallRun_CalcToWall(work.ScaledCapsuleRadius, WallRunStatus);
	if (FMath::IsNearlyZero(scale))
		return false;
	return WallRunCollision_LineTrace(work, work.UpdatedComponentRightVector * scale);
}

inline bool ULyraWRCharacterMovementComponent::WallRunCollision_SweepWall(FWallRunCollisionWork& work, EWallRunStatus WallRunStatus)const
{
	check(WallRunStatus != EWallRunStatus::WRS_None);

	auto scale = WallRun_CalcToWall(work.ScaledCapsuleRadius, WallRunStatus);
	if (FMath::IsNearlyZero(scale))
		return false;
	return WallRunCollision_Sweep(work, work.UpdatedComponentRightVector * scale);
}

inline EWallRunStatus ULyraWRCharacterMovementComponent::WallRunCollision_LineTraceWallAndCheckVelocity(FWallRunCollisionWork& work, EWallRunStatus WallRunStatus, const FVector& v)const
{
	check(!v.IsNearlyZero());
	check(WallRunStatus != EWallRunStatus::WRS_None);

	if (WallRunCollision_LineTraceWall(work, WallRunStatus))
	{
		//WALLRUN_LINE(work.Hit.TraceStart, work.Hit.TraceEnd, FColor::Green);
		if ((v | work.Hit.Normal) < 0)
		{
			return WallRunStatus;
		}
	}
	//WALLRUN_LINE(work.Hit.TraceStart, work.Hit.TraceEnd, FColor::Red);
	return EWallRunStatus::WRS_None;
}

inline EWallRunStatus ULyraWRCharacterMovementComponent::WallRunCollision_LineTraceWallAndUpdateIsRight(FWallRunCollisionWork& work, const FVector& v)const
{
	check(!v.IsNearlyZero());

	if (WallRunCollision_LineTraceWallAndCheckVelocity(work, EWallRunStatus::WRS_Left, v) != EWallRunStatus::WRS_None)
		return EWallRunStatus::WRS_Left;
	if (WallRunCollision_LineTraceWallAndCheckVelocity(work, EWallRunStatus::WRS_Right, v) != EWallRunStatus::WRS_None)
		return EWallRunStatus::WRS_Right;
	return EWallRunStatus::WRS_None;
}

inline bool ULyraWRCharacterMovementComponent::WallRunCollision_IsWallFound(FWallRunCollisionWork& work, EWallRunStatus WallRunStatus, const FVector& a) const
{
	check(WallRunStatus != EWallRunStatus::WRS_None);

#if 0 // delgoodie original
	WallRunCollision_LineTraceWall(work, WallRunStatus);
#else
	//エッジの対応のため、ライントレースではなくカプセルの Sweep を使う
	WallRunCollision_SweepWall(work, WallRunStatus);
#endif

	//壁が見つからないか
	if (!work.Hit.IsValidBlockingHit())
	{
		//UE_LOG(LogTemp, Warning, TEXT("Wall Not Found."));
		return false;
	}
	//加速が壁から離れる向きか
	//work.Hit.IsValidBlockingHit() はここに来る時点でtrueなのでチェックしない
	else if (/* work.Hit.IsValidBlockingHit() && */ WallRun_IsPullAway(a, work.Hit.Normal))
	{
		//UE_LOG(LogTemp, Warning, TEXT("Pull Away."));
		return false;
	}

	return true;
}

inline bool ULyraWRCharacterMovementComponent::WallRunCollision_IsFinished(FWallRunCollisionWork& work, const FVector& v, EWallRunStatus WallRunStatus) const
{
	check(WallRunStatus != EWallRunStatus::WRS_None);
	
	//速度が足りないか
	if (!WallRun_IsEnoughVelocity2D(v))
	{
		return true;
	}
	//床が近いか
	else if (WallRunCollision_LineTraceFloor(work))
	{
		//UE_LOG(LogTemp, Warning, TEXT("Floor is near."));
		return true;
	}
	//壁がないか
	else if (!WallRunCollision_LineTraceWall(work, WallRunStatus))
	{
		//UE_LOG(LogTemp, Warning, TEXT("Wall not found."));
		return true;
	}
	return false;
}

inline float ULyraWRCharacterMovementComponent::WallRun_CalcToWall(float ScaledCapsuleRadius, EWallRunStatus WallRunStatus)const
{
	check(WallRunStatus != EWallRunStatus::WRS_None);

	if (FMath::IsNearlyZero(ScaledCapsuleRadius))
		return 0.f;

	//壁を探す際の End までの Vector 長を算出する
	return ScaledCapsuleRadius * WallRunRadiusScaleForWallScanDistance * ((WallRunStatus == EWallRunStatus::WRS_Right) ? 1.f : -1.f);
}


inline bool ULyraWRCharacterMovementComponent::WallRun_IsPullAway(const FVector& a, const FVector& WallNormal) const
{
	check(!WallNormal.IsNearlyZero());

	//加速度がない場合は false を返す（壁から離れない）
	if (a.IsNearlyZero())
		return false;

	//壁から離れる限界角の正弦 = 1: 壁から離れる, 0: 壁に沿う, -1: 壁に向かう
	const auto SinPullAwayAngle = FMath::Sin(FMath::DegreesToRadians(WallRunPullAwayAngle));

	//加速方向と壁の法線の内積(=余弦) = 1: 壁から離れようと加速している, 0: 壁に沿って加速している,  -1: 壁に向かって加速している
	//cos と sin を比較しているが 「sin(x) = cos(PI/2 - x)」なので「壁と成す角の sin = 法線と成す角の cos」というのを利用している。
	//要は 加速方向と壁がなす角が WallRunPullAwayAngle より大きいなら true を返す。
	return (float)(a.GetSafeNormal() | WallNormal) > SinPullAwayAngle;
}

inline bool ULyraWRCharacterMovementComponent::WallRun_IsEnoughVelocity(const FVector& v, bool verbose) const
{
	//平面速度が足りないか
	if (!WallRun_IsEnoughVelocity2D(v, verbose))
	{
		return false;
	}
	//落下速度が速すぎるか
	else if (v.Z < -MaxVerticalDownWallRunSpeed)
	{
		//if (verbose)
		//	UE_LOG(LogTemp, Warning, TEXT("Too Heigh Fall Down Velocity."));
		return false;
	}
	return true;
}

inline bool ULyraWRCharacterMovementComponent::WallRun_IsEnoughVelocity2D(const FVector& v, bool verbose)const
{
	const auto SizeSquared2D = v.SizeSquared2D();

	// 0 に近い場合は MinWallRunSpeed と比較せずにそのまま返す
	if (FMath::IsNearlyZero(SizeSquared2D))
		return false;

	//v の平面速度が WallRun できる値か
	const auto SquaredMinWallRunSpeed = pow(MinWallRunSpeed, 2);
	if (SizeSquared2D < SquaredMinWallRunSpeed)
	{
		//if (verbose)
		//	UE_LOG(LogTemp, Warning, TEXT("Too Low Velocity."));
		return false;
	}
	return true;
}

inline float ULyraWRCharacterMovementComponent::WallRun_GetGravityWall(const FVector& a, const FVector& v) const
{
	//a , v 共に zero vector を許容する。

	//移動方向と加速方向を元に、落下速度の係数を決める（カーブの設定により逆向きほど大きい）

	//加速方向と移動方向の内積(=余弦) = 1: 同じ方向, 0: 垂直方向 , -1: 逆方向
	auto TangentAccel = (float)(a.GetSafeNormal() | v.GetSafeNormal2D());
	auto bVelUp = v.Z > 0.f;
	if (WallRunGravityScaleCurve)
	{
		// C_WallRunGravityScale の設定により逆向きほど大きい値を返す
		return WallRunGravityScaleCurve->GetFloatValue(bVelUp ? 0.f : TangentAccel);
	}
	else
	{
		//カーブがない場合は逆方向なら 0.4f 、順方向なら 0.f を返す。
		return bVelUp ? 0.f : ((TangentAccel < 0) ? 0.4f : 0.f);
	}
}

inline FVector ULyraWRCharacterMovementComponent::WallRun_CalcDeltaAfterBlocked(EWallRunStatus WallRunStatus, const FVector& Delta, const FVector& DeltaN, const FVector& WallNormal) const
{
	check(WallRunStatus != EWallRunStatus::WRS_None);

	//予定していた移動量と実際の移動量を元に、ぶつかった壁沿いの移動量の算出

	//予定していた移動量と実際の移動量の差から残りの移動量を算出(平面成分のみ)
	auto Alpha = (float)(Delta.Size2D() - DeltaN.Size2D());

	//壁沿い平面ベクトルを作り、移動できなかった移動量をかけ、左右の向きを整える
	//auto Delta2 = WallNormal.Cross(FVector::UpVector).GetSafeNormal2D() * Alpha * ((WallRunStatus == EWallRunStatus::WRS_Right) ? -1.f : 1.f);
	auto Delta2 = FVector::UpVector.Cross(WallNormal).GetSafeNormal2D() * Alpha * ((WallRunStatus == EWallRunStatus::WRS_Right) ? 1.f : -1.f);

	//Z成分は残りをそのまま使う
	Delta2.Z = Delta.Z - DeltaN.Z;

	return Delta2;
}

inline ULyraWRCharacterMovementComponent::FWallRunCollisionWork ULyraWRCharacterMovementComponent::WallRun_InitWork(bool IsInitCollisionShape)const
{
	return {
		GetIgnoreCharacterParams(), 
		IsInitCollisionShape ? CharacterOwner->GetCapsuleComponent()->GetCollisionShape() : FCollisionShape(),
		CapR(), 
		CapHH(), 
		UpdatedComponent->GetComponentLocation(), 
		UpdatedComponent->GetRightVector(), 
		{} 
	};
}


float ULyraWRCharacterMovementComponent::CapR() const
{
	return CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius();
}

float ULyraWRCharacterMovementComponent::CapHH() const
{
	return CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
}

FCollisionQueryParams ULyraWRCharacterMovementComponent::GetIgnoreCharacterParams() const
{
	FCollisionQueryParams Params;
	TArray<AActor*> ChildrenActors;
	CharacterOwner->GetAllChildActors(ChildrenActors);
	Params.AddIgnoredActors(ChildrenActors);
	Params.AddIgnoredActor(CharacterOwner);
	return Params;
}
