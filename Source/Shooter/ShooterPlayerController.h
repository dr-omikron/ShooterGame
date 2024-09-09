
#pragma once

#include "CoreMinimal.h"
#include "ShooterCharacter.h"
#include "GameFramework/PlayerController.h"
#include "ShooterPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
struct FInputActionValue;
class AShooterCharacter;

UCLASS()
class SHOOTER_API AShooterPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	AShooterPlayerController();
	
protected:
	virtual void BeginPlay() override;
	virtual void PlayerTick(float DeltaTime) override;
	virtual void SetupInputComponent() override;
	
private:
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputMappingContext> ShooterContext;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> IA_MoveAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> IA_CameraAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> IA_JumpAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> IA_FireAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> IA_Aiming;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> IA_Select;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> IA_Reload;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> IA_Crouch;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> IA_SelectDefaultWeapon;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> IA_SelectWeapon1;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> IA_SelectWeapon2;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> IA_SelectWeapon3;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> IA_SelectWeapon4;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> IA_SelectWeapon5;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	float BaseTurnRate;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	float BaseLookUpRate;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	float HipTurnRate;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	float HipLookUpRate;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	float AimingTurnRate;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	float AimingLookUpRate;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character", meta = (AllowPrivateAccess = "true"))
	AShooterCharacter* ShooterCharacter;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widgets", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UUserWidget> HUDOverlayClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Widgets", meta = (AllowPrivateAccess = "true"))
	UUserWidget* HUDOverlay;
	
	void Move(const FInputActionValue& InputActionValue);
	void CameraMovement(const FInputActionValue& InputActionValue);
	void Jump() { ShooterCharacter->Jump(); }
	void StopJumping() { ShooterCharacter->StopJumping(); }
	void Fire() { ShooterCharacter->FireWeapon(); }
	void FirePressed() { ShooterCharacter->FireButtonPressed(); }
	void FireReleased() { ShooterCharacter->FireButtonReleased(); }
	void StartAiming() { ShooterCharacter->AimingButtonPressed(); }
	void StopAiming() { ShooterCharacter->AimingButtonReleased(); }
	void SetLookRates();
	void StartSelect() { ShooterCharacter->SelectButtonPressed(); }
	void StopSelect() { ShooterCharacter->SelectButtonReleased(); }
	void Reloading() { ShooterCharacter->ReloadButtonPressed(); }
	void Crouching() { ShooterCharacter->CrouchButtonPressed(); }
	void SelectDefaultWeapon() { ShooterCharacter->DefaultWeaponButtonPressed(); }
	void SelectWeapon1() { ShooterCharacter->Weapon1ButtonPressed(); }
	void SelectWeapon2() { ShooterCharacter->Weapon2ButtonPressed(); }
	void SelectWeapon3() { ShooterCharacter->Weapon3ButtonPressed(); }
	void SelectWeapon4() { ShooterCharacter->Weapon4ButtonPressed(); }
	void SelectWeapon5() { ShooterCharacter->Weapon5ButtonPressed(); }
};
