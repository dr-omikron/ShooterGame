
#include "ShooterAnimInstance.h"
#include "ShooterCharacter.h"
#include "Weapon.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"


UShooterAnimInstance::UShooterAnimInstance()
{
	ShooterCharacter = nullptr;
	Speed = 0.f;
	bIsInAir = false;
	bIsAccelerating = false;
	bAiming = false;
	MovementOffsetYaw = 0.f;
	LastMovementOffsetYaw = 0.f;
	TIPCharacterYaw = 0.f;
	TIPCharacterYawLastFrame = 0.f;
	RootYawOffset = 0.f;
	Pitch = 0.f;
	bIsReloading = false;
	bIsCrouching = false;
	OffsetState = EOffsetState::EOS_Hip;
	RotationCurve = 0.f;
	RotationCurveLastFrame = 0.f;
	CharacterYawDelta = 0.f;
	CharacterRotation = FRotator(0);
	CharacterRotationLastFrame = FRotator(0);
	RecoilWeight = 1.f;
	bTurningInPlace = false;
	bIsEquipping = false;
	EquippedWeaponType = EWeaponType::EWT_SubmachineGun;
	bShouldUseFABRIK = false;
}

void UShooterAnimInstance::UpdateAnimationProperties(float DeltaTime)
{
	if(ShooterCharacter == nullptr)
	{
		ShooterCharacter = Cast<AShooterCharacter>(TryGetPawnOwner());
	}
	if(ShooterCharacter)
	{
		bIsCrouching = ShooterCharacter->GetCrouching();
		bIsReloading = ShooterCharacter->GetCombatState() == ECombatStates::ECS_Reloading;
		bIsEquipping = ShooterCharacter->GetCombatState() == ECombatStates::ECS_Equipping;
		bShouldUseFABRIK = ShooterCharacter->GetCombatState() == ECombatStates::ECS_Unoccupied ||
			ShooterCharacter->GetCombatState() == ECombatStates::ECS_FireTimerInProgress;
		FVector Velocity = ShooterCharacter->GetVelocity();
		Velocity.Z = 0.f;
		Speed = Velocity.Size();

		bIsInAir = ShooterCharacter->GetCharacterMovement()->IsFalling();

		if(ShooterCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f)
		{
			bIsAccelerating = true;
		}
		else
		{
			bIsAccelerating = false;
		}

		const FRotator AimRotation = ShooterCharacter->GetBaseAimRotation();
		const FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(ShooterCharacter->GetVelocity());
		MovementOffsetYaw = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation).Yaw;
		if(ShooterCharacter->GetVelocity().Size() > 0.f)
		{
			LastMovementOffsetYaw = MovementOffsetYaw;
		}
		bAiming = ShooterCharacter->GetAiming();
		if(bIsReloading)
		{
			OffsetState = EOffsetState::EOS_Reloading;
		}
		else if (bIsInAir)
		{
			OffsetState = EOffsetState::EOS_InAir;
		}
		else if (ShooterCharacter->GetAiming())
		{
			OffsetState = EOffsetState::EOS_Aiming;
		}
		else
		{
			OffsetState = EOffsetState::EOS_Hip;
		}
		if(ShooterCharacter->GetEquippedWeapon())
		{
			EquippedWeaponType = ShooterCharacter->GetEquippedWeapon()->GetWeaponType();
		}
	}
	TurnInPlace();
	Lean(DeltaTime);
}

void UShooterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	ShooterCharacter = Cast<AShooterCharacter>(TryGetPawnOwner());
}

void UShooterAnimInstance::TurnInPlace()
{
	if(ShooterCharacter == nullptr) return;
	Pitch = ShooterCharacter->GetBaseAimRotation().Pitch;
	if(Speed > 0 || bIsInAir)
	{
		RootYawOffset = 0.f;
		TIPCharacterYaw = ShooterCharacter->GetActorRotation().Yaw;
		TIPCharacterYawLastFrame = TIPCharacterYaw;
		RotationCurve = 0.f;
		RotationCurveLastFrame = 0.f;
	}
	else
	{
		TIPCharacterYawLastFrame = TIPCharacterYaw;
		TIPCharacterYaw = ShooterCharacter->GetActorRotation().Yaw;
		const float YawDelta = TIPCharacterYaw - TIPCharacterYawLastFrame;
		RootYawOffset = UKismetMathLibrary::NormalizeAxis(RootYawOffset - YawDelta);
		const float Turning = GetCurveValue(TEXT("Turning"));
		if(Turning > 0)
		{
			bTurningInPlace = true;
			RotationCurveLastFrame = RotationCurve;
			RotationCurve = GetCurveValue(TEXT("Rotation"));
			const float DeltaRotation = RotationCurve - RotationCurveLastFrame;
			RootYawOffset > 0 ? RootYawOffset -= DeltaRotation : RootYawOffset += DeltaRotation;
			const float ABSRootYawOffset = FMath::Abs(RootYawOffset);
			if(ABSRootYawOffset > 90.f)
			{
				const float YawExcess = ABSRootYawOffset - 90.f;
				RootYawOffset > 0 ? RootYawOffset -= YawExcess : RootYawOffset += YawExcess;
			}
		}
		else
		{
			bTurningInPlace = false;
		}
	}

	if(bTurningInPlace)
	{
		if(bIsReloading || bIsEquipping)
		{
			RecoilWeight = 1.f;
		}
		else
		{
			RecoilWeight = 0.f;
		}
			
	}
	else
	{
		if(bIsCrouching)
		{
			if(bIsReloading || bIsEquipping)
			{
				RecoilWeight = 1.f;
			}
			else
			{
				RecoilWeight = 0.1f;
			}
		}
		else
		{
			if(bAiming || bIsReloading || bIsEquipping)
			{
				RecoilWeight = 1.f;
			}
			else
			{
				RecoilWeight = 0.5f;
			}
		}
	}
}

void UShooterAnimInstance::Lean(float DeltaTime)
{
	if(ShooterCharacter == nullptr) return;
	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = ShooterCharacter->GetActorRotation();
	const FRotator DeltaRotation = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
	const float Target = DeltaRotation.Yaw / DeltaTime;
	const float Interp = FMath::FInterpTo(CharacterYawDelta, Target, DeltaTime, 6.f);
	CharacterYawDelta = FMath::Clamp(Interp, -90.f, 90.f);
}


