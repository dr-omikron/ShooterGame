

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AmmoType.h"
#include "ShooterCharacter.generated.h"

class AWeapon;
class AItemBase;
class USpringArmComponent;
class UCameraComponent;
class USoundCue;
class UParticleSystem;
class UAnimMontage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEquipItemDelegate, int32, CurrentSlotIndex, int32, NewSlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHighlightIconDelegate, int32, SlotIndex, bool, bPlayAnimation);

UENUM(BlueprintType)
enum class ECombatStates : uint8
{
	ECS_Unoccupied UMETA(DisplayName = "Unoccupied"),
	ECS_FireTimerInProgress UMETA(DisplayName = "FireTimerInProgress"),
	ECS_Reloading UMETA(DisplayName = "Reloading"),
	ECS_Equipping UMETA(DisplayName = "Equipping"),
	ECS_Stunned UMETA(DisplayName = "Stunned"),
	ECS_MAX UMETA(DisplayName = "DefaultMAX")
};

USTRUCT(BlueprintType)
struct FInterpLocation
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USceneComponent* SceneComponent = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 ItemCount = 0;
};

UCLASS()
class SHOOTER_API AShooterCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AShooterCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual void Jump() override;
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	
	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE int8 GetOverlappedItemCount() const { return OverlappedItemCount; }
	FORCEINLINE bool GetAiming() const { return bAiming; }
	FORCEINLINE ECombatStates GetCombatState() const { return CombatStates; }
	FORCEINLINE bool GetCrouching() const { return bCrouching; }
	FORCEINLINE bool ShouldPlayPickupSound() const { return bShouldPlayPickupSound; }
	FORCEINLINE bool ShouldPlayEquipSound() const { return bShouldPlayEquipSound; }
	FORCEINLINE AWeapon* GetEquippedWeapon() const { return EquippedWeapon; }
	FORCEINLINE USoundCue* GetMeleeImpactSound() const { return MeleeImpactSound; }
	FORCEINLINE UParticleSystem* GetBloodParticles() const { return BloodParticles; }
	FORCEINLINE float GetStunChance() const { return StunChance; }
		
	FInterpLocation GetInterpLocation(int32 Index) const;	

	UFUNCTION(BlueprintCallable)
	float GetCrosshairSpreadMultiplayer() const { return CrosshairSpreadMultiplayer; }
	//FVector GetCameraInterpLocation() const;
	void FireWeapon();
	bool GetBeamEndLocation(const FVector& MuzzleSocketLocation, FHitResult& OutHitResult) const;
	void AimingButtonPressed();
	void AimingButtonReleased();
	void FireButtonPressed();  
	void FireButtonReleased() { bFireButtonPressed = false; }
	void SelectButtonPressed();
	static void SelectButtonReleased(){};
	void ReloadButtonPressed();
	void CrouchButtonPressed();
	void DefaultWeaponButtonPressed();
	void Weapon1ButtonPressed();
	void Weapon2ButtonPressed();
	void Weapon3ButtonPressed();
	void Weapon4ButtonPressed();
	void Weapon5ButtonPressed();
	void IncrementOverlappedItemCount(int8 Amount);
	void GetPickupItem(AItemBase* Item);
	void Aim();
	void StopAiming();
	int32 GetInterpLocationIndex() const;
	void IncrementInterpLocationItemCount(int32 Index, int32 Amount);
	void StartPickupSoundTimer();
	void StartEquipSoundTimer();
	void UnhighlightInventorySlot();
	void Stun();
	
protected:
	UFUNCTION()
	void FinishCrosshairBulletFire();

	UFUNCTION()
	void AutoFireReset();

	UFUNCTION(BlueprintCallable)
	EPhysicalSurface GetSurfaceType();

	UFUNCTION(BlueprintCallable)
	void FinishReloading();

	UFUNCTION(BlueprintCallable)
	void FinishEquipping();

	UFUNCTION(BlueprintCallable)
	void EndStun();

	UFUNCTION(BlueprintCallable)
	void GrabClip();

	UFUNCTION(BlueprintCallable)
	void ReleaseClip() const;

	virtual void BeginPlay() override;
	void CameraInterpZoom(float DeltaTime);
	void CalculateCrosshairSpread(float DeltaTime);
	void StartCrosshairBulletFire();
	void StartFireTimer();
	bool TraceUnderCrosshair(FHitResult& OutHitResult, FVector& OutHitLocation) const;
	void TraceForItems();
	AWeapon* SpawnDefaultWeapon() const;
	void EquipWeapon(AWeapon* WeaponToEquip, const bool bSwapping = false);
	void DropWeapon() const;
	void SwapWeapon(AWeapon* WeaponToSwap);
	void InitializeAmmoMap();
	bool WeaponHasAmmo() const;
	void PlayFireSound() const;
	void SendBullet() const;
	void PlayGunFireMontage() const;
	void ReloadWeapon();
	bool CarryingAmmo();
	void InterpCapsuleHalfHeight(float DeltaTime) const;
	void PickupAmmo(class AAmmo* Ammo);
	void InitializeInterpLocations();
	void ResetPickupSoundTimer();
	void ResetEquipSoundTimer();
	void ExchangeInventoryItems(const int32 CurrentItemIndex, const int32 NewItemIndex);
	int32 GetEmptyInventorySlot();
	void HighlightInventorySlot();
	
		
private:
	// Camera

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	float CameraDefaultFOV;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	float CameraZoomedFOV;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;	

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	float ZoomInterpSpeed;

	// Crosshair
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crosshair", meta = (AllowPrivateAccess = "true"))
	float CrosshairSpreadMultiplayer;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crosshair", meta = (AllowPrivateAccess = "true"))
	float CrosshairVelocityFactor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crosshair", meta = (AllowPrivateAccess = "true"))
	float CrosshairInAirFactor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crosshair", meta = (AllowPrivateAccess = "true"))
	float CrosshairAimFactor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crosshair", meta = (AllowPrivateAccess = "true"))
	float CrosshairShootingFactor;

	//Combat
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	AWeapon* EquippedWeapon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AWeapon> DefaultWeaponClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	AItemBase* TraceHitItem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UParticleSystem* ImpactFlash;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UParticleSystem* BeamParticles;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* HipFireMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* ReloadMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* EquipMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UAnimMontage* HitReactMontage;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	bool bAiming;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	ECombatStates CombatStates;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	FTransform ClipTransform;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	USceneComponent* HandSceneComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	float Health;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	float MaxHealth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	USoundCue* MeleeImpactSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UParticleSystem* BloodParticles;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	float StunChance;
	
	//Items
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Items", meta = (AllowPrivateAccess = "true"))
	AItemBase* TraceHitItemLastFrame;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Items", meta = (AllowPrivateAccess = "true"))
	float CameraInterpDistance;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Items", meta = (AllowPrivateAccess = "true"))
	float CameraInterpElevation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Items", meta = (AllowPrivateAccess = "true"))
	TMap<EAmmoType, int32> AmmoMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Items", meta = (AllowPrivateAccess = "true"))
	int32 Starting9mmAmmo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Items", meta = (AllowPrivateAccess = "true"))
	int32 StartingARAmmo;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Items", meta = (AllowPrivateAccess = "true"))
	USceneComponent* InterpComponent_01;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Items", meta = (AllowPrivateAccess = "true"))
	USceneComponent* InterpComponent_02;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Items", meta = (AllowPrivateAccess = "true"))
	USceneComponent* InterpComponent_03;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Items", meta = (AllowPrivateAccess = "true"))
	USceneComponent* InterpComponent_04;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Items", meta = (AllowPrivateAccess = "true"))
	USceneComponent* InterpComponent_05;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Items", meta = (AllowPrivateAccess = "true"))
	USceneComponent* InterpComponent_06;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Items", meta = (AllowPrivateAccess = "true"))
	USceneComponent* WeaponInterpComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Items", meta = (AllowPrivateAccess = "true"))
	TArray<FInterpLocation> InterpLocations;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Items", meta = (AllowPrivateAccess = "true"))
	float PickupSoundResetTime;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Items", meta = (AllowPrivateAccess = "true"))
	float EquipSoundResetTime;

	//Inventory
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (AllowPrivateAccess = "true"))
	TArray<AItemBase*> Inventory;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (AllowPrivateAccess = "true"))
	int32 HighlightedSlot;

	UPROPERTY(BlueprintAssignable, Category = "Delegates", meta = (AllowPrivateAccess = "true"))
	FEquipItemDelegate EquipItemDelegate;

	UPROPERTY(BlueprintAssignable, Category = "Delegates", meta = (AllowPrivateAccess = "true"))
	FHighlightIconDelegate HighlightIconDelegate;
	
	//Movement
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	bool bCrouching;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	float BaseMovementSpeed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	float CrouchMovementSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	float StandingCapsuleHalfHeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	float CrouchingCapsuleHalfHeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	float BaseGroundFriction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	float CrouchingGroundFriction;
	
	//Private

	const int32 INVENTORY_CAPACITY = 6;
	float CameraCurrentFOV;
	float ShootTimeDuration;
	float CurrentCapsuleHalfHeight;
	FTimerHandle CrosshairShootTimer;
	FTimerHandle AutoFireTimer;
	FTimerHandle PickupSoundTimer;
	FTimerHandle EquipSoundTimer;
	bool bFireButtonPressed;
	bool bShouldFire;
	bool bShouldTraceForItems;
	bool bFiringBullet;
	bool bAimingButtonPressed;
	bool bShouldPlayPickupSound;
	bool bShouldPlayEquipSound;
	int8 OverlappedItemCount;
};
