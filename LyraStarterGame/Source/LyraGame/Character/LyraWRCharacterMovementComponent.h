// Copyright 2023 Sentya Anko

#pragma once

#include "CoreMinimal.h"
#include "Character/LyraCharacterMovementComponent.h"
#include "LyraWRCharacterMovementComponent.generated.h"


UENUM(BlueprintType)
enum ECustomMovementMode
{
	CMOVE_None			UMETA(Hidden),
	CMOVE_WallRunLeft	UMETA(DisplayName = "Wall Run Left"),
	CMOVE_WallRunRight	UMETA(DisplayName = "Wall Run Right"),
	CMOVE_MAX			UMETA(Hidden),
};

UENUM(BlueprintType)
enum class EWallRunStatus : uint8
{
	WRS_None			UMETA(DisplayName = "None"),
	WRS_Left			UMETA(DisplayName = "Left"),
	WRS_Right			UMETA(DisplayName = "Right"),
	WRS_MAX				UMETA(Hidden),
};



/**
 * 
 */
UCLASS()
class LYRAGAME_API ULyraWRCharacterMovementComponent : public ULyraCharacterMovementComponent
{
	GENERATED_BODY()
	
private:
	struct FWallRunCollisionWork
	{
		//固定値
		const FCollisionQueryParams IgnoreCharacterParams;
		const FCollisionShape CollisionShape;
		const float ScaledCapsuleRadius;
		const float ScaledCapsuleHalfHeight;
		//移動処理毎に更新する値
		FVector UpdatedComponentLocation;
		FVector UpdatedComponentRightVector;
		//コリジョン判定時に更新する値
		FHitResult Hit;
	};

public:
	ULyraWRCharacterMovementComponent(const FObjectInitializer& ObjectInitializer);

	//UCharacterMovementComponent interface begin{
protected:
	/** Update the character state in PerformMovement right before doing the actual position change */
	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;

	/** @note Movement update functions should only be called through StartNewPhysics()*/
	virtual void PhysCustom(float deltaTime, int32 Iterations) override;

	/** Called after MovementMode has changed. Base implementation does special handling for starting certain modes, then notifies the CharacterOwner. */
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;

public:
	/** Returns maximum deceleration for the current state when braking (ie when there is no acceleration). */
	virtual float GetMaxBrakingDeceleration() const override;

	/**
	 * Perform jump. Called by Character when a jump has been detected because Character->bPressedJump was true. Checks Character->CanJump().
	 * Note that you should usually trigger a jump through Character::Jump() instead.
	 * @param	bReplayingMoves: true if this is being done as part of replaying moves on a locally controlled client after a server correction.
	 * @return	True if the jump was triggered successfully.
	 */
	virtual bool DoJump(bool bReplayingMoves) override;

	/**
	 * Returns true if current movement state allows an attempt at jumping. Used by Character::CanJump().
	 */
	virtual bool CanAttemptJump() const override;

	//}UCharacterMovementComponent interface end

	//BEGIN UMovementComponent Interface
public:
	virtual float GetMaxSpeed() const override;

	//END UMovementComponent Interface




public:
	UFUNCTION(BlueprintPure) bool IsCustomMovementMode(ECustomMovementMode InCustomMovementMode)const;
	UFUNCTION(BlueprintPure) bool IsMovementMode(EMovementMode InMovementMode)const;

	//WallRun
	UFUNCTION(BlueprintPure, Category = "LyraWR|WallRun") EWallRunStatus GetWallRunStatus()const;
	UFUNCTION(BlueprintPure, Category = "LyraWR|WallRun") FVector GetWallRunNormal()const { return WallRunNormal; };

	//WallRun
private:
	bool TryWallRun();
	void PhysWallRun(float deltaTime, int32 Iterations);

	// @brief 現在の位置から LineTrace を行う。
	// @param ToEnd LineTrace 先を示すベクトル。
	// @return  LineTrace の結果。
	bool WallRunCollision_LineTrace(FWallRunCollisionWork& work, const FVector& ToEnd)const;

	// @brief 現在の位置から Sweep を行う。
	// @param ToEnd Sweep 先を示すベクトル。
	// @return  Sweep の結果。
	bool WallRunCollision_Sweep(FWallRunCollisionWork& work, const FVector& ToEnd)const;

	// @brief 床を LineTrace で探す。
	// @retval true 見つかった。
	// @retval false 見つからなかった。
	bool WallRunCollision_LineTraceFloor(FWallRunCollisionWork& work)const;

	// @brief 左右の壁を LineTrace で探す。
	// @param WallRunStatus 左右。
	// @retval true 見つかった。
	// @retval false 見つからなかった。
	bool WallRunCollision_LineTraceWall(FWallRunCollisionWork& work, EWallRunStatus WallRunStatus)const;

	// @brief 左右の壁を Sweepで探す。
	// @param WallRunStatus 左右。
	// @retval true 見つかった。
	// @retval false 見つからなかった。
	bool WallRunCollision_SweepWall(FWallRunCollisionWork& work, EWallRunStatus WallRunStatus)const;

	// @brief 左右の壁を LineTraceで探し、見つかったら進行方向が壁側を向いているかを調べる。
	// @param WallRunStatus 左右。
	// @param v 速度ベクトル。
	// @retval EWallRunStatus::WRS_None 見つからなかった。
	// @retval EWallRunStatus::WRS_Left 左にあった。
	// @retval EWallRunStatus::WRS_Right 右にあった。
	EWallRunStatus WallRunCollision_LineTraceWallAndCheckVelocity(FWallRunCollisionWork& work, EWallRunStatus WallRunStatus, const FVector& v)const;

	// @brief 壁があるか左右の順に調べる。
	// @param v 速度ベクトル。
	// @retval EWallRunStatus::WRS_None 見つからなかった。
	// @retval EWallRunStatus::WRS_Left 左にあった。
	// @retval EWallRunStatus::WRS_Right 右にあった。
	EWallRunStatus WallRunCollision_LineTraceWallAndUpdateIsRight(FWallRunCollisionWork& work, const FVector& v)const;

	// @brief 指定された方向に壁があるか調べる。
	// @param WallRunStatus 左右。
	// @param a 加速度ベクトル。
	// @retval true 見つかった。
	// @retval false 見つからなかった。
	bool WallRunCollision_IsWallFound(FWallRunCollisionWork& work, EWallRunStatus WallRunStatus, const FVector& a)const;

	// @brief WallRun を終わらすかを調べる。
	// 具体的には速度が十分か、床がないか、壁があるか、を調べる。
	// @param v 速度ベクトル。
	// @param WallRunStatus 左右。
	// @retval true 終わらす。
	// @retval false 続ける。
	bool WallRunCollision_IsFinished(FWallRunCollisionWork& work, const FVector& v, EWallRunStatus WallRunStatus)const;

	// @brief 壁を探す際のトレース先へのベクトルを取得する。
	// @param WallRunStatus 左右。 None を渡すと ZeroVector を返す。
	// @return トレース先へのベクトル長。
	float WallRun_CalcToWall(float ScaledCapsuleRadius, EWallRunStatus WallRunStatus)const;

	// @brief 現在の加速ベクトルが壁から離れる値かを調べる。
	// @param a 加速度ベクトル。
	// @param WallNormal 壁の法線。
	// @retval true 離れる。
	// @retval false 離れない。
	bool WallRun_IsPullAway(const FVector& a, const FVector& WallNormal)const;

	// @brief 渡されたベクトルが WallRun できる値かを調べる。
	// @param v 調べる値。
	// @param verbose デバッグ用。そのうち消す。
	// @retval true 出来る。
	// @retval false 出来ない。
	bool WallRun_IsEnoughVelocity(const FVector& v, bool verbose = true)const;

	// @brief 渡されたベクトルの平面成分が WallRun できる値かを調べる。
	// @param v 調べる値。
	// @param verbose デバッグ用。そのうち消す。
	// @retval true 出来る。
	// @retval false 出来ない。
	bool WallRun_IsEnoughVelocity2D(const FVector& v, bool verbose = true)const;

	// @brief 渡された加速度と速度を元に壁に吸い付く力(重力係数への係数)を算出する。
	// @param a 加速度。
	// @param v 速度。
	// @return 係数。重力加速度に掛け合わせることを想定している。
	float WallRun_GetGravityWall(const FVector& a, const FVector& v)const;

	// @brief 予定していた移動ベクトルと実際の移動ベクトルを元に、ぶつかった壁沿いの移動ベクトルを算出する。
	// @param WallRunStatus 左右。 None を渡すと ZeroVector を返す。
	// @param Delta 予定していた移動ベクトル。
	// @param DeltaN 実際の移動ベクトル。
	// @param WallNormal ぶつかった壁の法線。
	// @return 移動したい、ぶつかった壁に沿った移動ベクトル。
	FVector WallRun_CalcDeltaAfterBlocked(EWallRunStatus WallRunStatus, const FVector& Delta, const FVector& DeltaN, const FVector& WallNormal)const;

	// @brief FWallRunCollisionWork の初期化を行う
	// @param IsInitCollisionShape CollisionShape の初期化を行うか
	// @return FWallRunCollisionWork
	FWallRunCollisionWork WallRun_InitWork(bool IsInitCollisionShape)const;

	//Helper
private:
	// @brief オーナーのカプセルの半径を取得する
	// @return カプセルの半径
	float CapR()const;

	// @brief オーナーのカプセルの HalfHeight を取得する
	// @return カプセルの HalfHeight
	float CapHH()const;

	// @brief オーナー自身とその子を無視するクエリパラメータを取得する
	// @return クエリパラメータ
	FCollisionQueryParams GetIgnoreCharacterParams() const;

	//WallRun
private:
	// @brief 壁の法線。 WallRun していないときは ZeroVector になる。
	FVector WallRunNormal;


	//WallRun
protected:
	// Velocity の下限[cm/s]。
	// これを下回ると WallRun を中断する。
	UPROPERTY(EditDefaultsOnly, Category = "LyraWR|WallRun") float MinWallRunSpeed = 200.f;

	// Velocity の上限[cm/s]。
	// WallRun 中の GetMaxSpeed() はこの値になる。
	UPROPERTY(EditDefaultsOnly, Category = "LyraWR|WallRun") float MaxWallRunSpeed = 800.f;

	// 上昇速度の上限[cm/s]。
	// WallRun 開始の際、上下の速度は clamp(0, この値 ) される。
	UPROPERTY(EditDefaultsOnly, Category = "LyraWR|WallRun") float MaxVerticalUpWallRunSpeed = 200.f;

	// 下降速度の上限[cm/s]。
	// これを上回る速度で下降すると WallRun を中断する。
	UPROPERTY(EditDefaultsOnly, Category = "LyraWR|WallRun") float MaxVerticalDownWallRunSpeed = 400.f;

	// 壁から離れる角度[degree]。
	// 壁と Acceleration のなす角がこれより広いと WallRun を中断する。
	UPROPERTY(EditDefaultsOnly, Category = "LyraWR|WallRun") float WallRunPullAwayAngle = 60.f;

	// WallRun 中に壁方向に発生する速度を求める際に使用する係数。
	// 速度 = カプセルの半径 * この値。
	// 0.3 程あると半径 8 m の円柱を走り続けられる。
	// WallRun 開始直後の壁に張り付くまでの時間にも影響するので、 1 程度はあったほうが良い。
	UPROPERTY(EditDefaultsOnly, Category = "LyraWR|WallRun") float WallRunAttractionVelocityScale = 2.f;

	// WallRun に必要な床までの距離[cm]。
	// これより床が近いと WallRun を中断する。
	UPROPERTY(EditDefaultsOnly, Category = "LyraWR|WallRun") float MinWallRunHeight = 50.f;

	// WallRun 中にジャンプした際の、壁の法線方向の初速[cm/s]。
	UPROPERTY(EditDefaultsOnly, Category = "LyraWR|WallRun") float WallRunJumpWallNormalInitialVelocity = 200.f;

	// WallRun 中に壁沿いの移動前に行う壁の法線方向への移動速度[cm/s]。
	// この値が 0.03 で 30 fps の 1 フレーム換算で 1 mm 程移動することになる。
	// 壁と壁のつなぎ目への引っ掛かり対策。
	UPROPERTY(EditDefaultsOnly, Category = "LyraWR|WallRun") float WallRunAwayFromWallBeforeMoveingVelocityScale = 0.03f;

	// WallRun の壁を探す距離を求める際に使用する係数。
	// 距離 = カプセルの半径 * この値。
	UPROPERTY(EditDefaultsOnly, Category = "LyraWR|WallRun") float WallRunRadiusScaleForWallScanDistance = 2.f;

	// WallRun 中に発生する重力加速度の係数。
	// 速度と加速度の余弦がパラメータとなる。
	UPROPERTY(EditDefaultsOnly, Category = "LyraWR|WallRun") UCurveFloat* WallRunGravityScaleCurve;
};
