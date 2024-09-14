
#include "ShooterCharacter.h"

#include "Ammo.h"
#include "BulletHitInterface.h"
#include "Enemy.h"
#include "ItemBase.h"
#include "Weapon.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"

AShooterCharacter::AShooterCharacter()
{
 	PrimaryActorTick.bCanEverTick = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 240.f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->SocketOffset = FVector(0.f, 35.f, 80.f);
	CameraDefaultFOV = 0.f;
	CameraZoomedFOV = 25.f;
	CameraCurrentFOV = 0.f;
	ZoomInterpSpeed = 20.f;
	CrosshairSpreadMultiplayer = 0.f;
	CrosshairVelocityFactor = 0.f;
	CrosshairInAirFactor = 0.f;
	CrosshairAimFactor = 0.f;
	CrosshairShootingFactor = 0.f;
	ShootTimeDuration = 0.05f;
	CameraInterpElevation = 65.f;
	CameraInterpDistance = 250.f;
	OverlappedItemCount = 0;
	bFiringBullet = false;
	bAiming = false;
	bShouldFire = true;
	bFireButtonPressed = false;
	bShouldTraceForItems = false;
	Starting9mmAmmo = 85;
	StartingARAmmo = 120;
	CombatStates = ECombatStates::ECS_Unoccupied;
	bCrouching = false;
	bAimingButtonPressed = false;
	BaseMovementSpeed = 650.f;
	CrouchMovementSpeed = 300.f;
	StandingCapsuleHalfHeight = 88.f;
	CrouchingCapsuleHalfHeight = 44.f;
	BaseGroundFriction = 2.f;
	CrouchingGroundFriction = 100.f;
	bShouldPlayPickupSound = true;
	bShouldPlayEquipSound = true;
	PickupSoundResetTime = 0.2f;
	EquipSoundResetTime = 0.2f;
	HighlightedSlot = -1;
	
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
	HandSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("HandSceneComponent"));
	WeaponInterpComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Weapon Interpolation Component"));
	WeaponInterpComponent->SetupAttachment(GetFollowCamera());
	InterpComponent_01 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Component 1"));
	InterpComponent_01->SetupAttachment(GetFollowCamera());
	InterpComponent_02 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Component 2"));
	InterpComponent_02->SetupAttachment(GetFollowCamera());
	InterpComponent_03 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Component 3"));
	InterpComponent_03->SetupAttachment(GetFollowCamera());
	InterpComponent_04 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Component 4"));
	InterpComponent_04->SetupAttachment(GetFollowCamera());
	InterpComponent_05 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Component 5"));
	InterpComponent_05->SetupAttachment(GetFollowCamera());
	InterpComponent_06 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Component 6"));
	InterpComponent_06->SetupAttachment(GetFollowCamera());
	
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2;
}

void AShooterCharacter::BeginPlay()
{
	Super::BeginPlay();

	if(FollowCamera)
	{
		CameraDefaultFOV = GetFollowCamera()->FieldOfView;
		CameraCurrentFOV = CameraDefaultFOV;
	}
	EquipWeapon(SpawnDefaultWeapon());
	Inventory.Add(EquippedWeapon);
	EquippedWeapon->SetSlotIndex(0);
	EquippedWeapon->DisableCustomDepth();
	EquippedWeapon->DisableGlowMaterial();
	EquippedWeapon->SetCharacter(this);
	InitializeAmmoMap();
	GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;
	InitializeInterpLocations();
}

void AShooterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	CameraInterpZoom(DeltaTime);
	CalculateCrosshairSpread(DeltaTime);
	InterpCapsuleHalfHeight(DeltaTime);
	TraceForItems();
}

// Combat

void AShooterCharacter::FireWeapon()
{
	if(EquippedWeapon == nullptr) return;
	if(CombatStates != ECombatStates::ECS_Unoccupied) return;
	if(WeaponHasAmmo())
	{		
		PlayFireSound();
		SendBullet();
		PlayGunFireMontage();
		StartCrosshairBulletFire();
		EquippedWeapon->DecrementAmmo();
		StartFireTimer();
		if(EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Pistol)
		{
			EquippedWeapon->StartSlideTimer();
		}
	}
}

void AShooterCharacter::PlayFireSound() const
{
	if(EquippedWeapon->GetFireSound())
	{
		UGameplayStatics::PlaySound2D(this, EquippedWeapon->GetFireSound());
	}
}

void AShooterCharacter::SendBullet() const
{
	if(const USkeletalMeshSocket* BarrelSocket = EquippedWeapon->GetItemMesh()->GetSocketByName("BarrelSocket"))
	{
		const FTransform BarrelSocketTransform = BarrelSocket->GetSocketTransform(EquippedWeapon->GetItemMesh());
		if(EquippedWeapon->GetMuzzleFlash())
		{
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), EquippedWeapon->GetMuzzleFlash(), BarrelSocketTransform);
		}

		FHitResult BeamHitResult;
		if(GetBeamEndLocation(BarrelSocketTransform.GetLocation(), BeamHitResult))
		{
			if(BeamHitResult.GetActor())
			{
				if(IBulletHitInterface* BulletHitInterface = Cast<IBulletHitInterface>(BeamHitResult.GetActor()))
				{
					BulletHitInterface->BulletHit_Implementation(BeamHitResult);
				}
				if(AEnemy* HitEnemy = Cast<AEnemy>(BeamHitResult.GetActor()))
				{
					int32 Damage = 0;
					bool bHeadShot = false;
					if(BeamHitResult.BoneName.ToString() == HitEnemy->GetHeadName())
					{
						Damage = EquippedWeapon->GetHeadShotDamage();
						UGameplayStatics::ApplyDamage(BeamHitResult.GetActor(), Damage,
						GetController(), UGameplayStatics::GetPlayerCharacter(GetWorld(), 0), UDamageType::StaticClass());
						bHeadShot = true;
					}
					else
					{
						Damage = EquippedWeapon->GetDamage();
						UGameplayStatics::ApplyDamage(BeamHitResult.GetActor(), Damage,
						GetController(), UGameplayStatics::GetPlayerCharacter(GetWorld(), 0), UDamageType::StaticClass());
					}
					HitEnemy->ShowHitNumber(Damage, BeamHitResult.Location, bHeadShot);
				}
			}
			else
			{
				if(ImpactFlash)
				{
					UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactFlash, BeamHitResult.Location);
				}
			}
			if(BeamParticles)
			{
				if(UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), BeamParticles, BarrelSocketTransform.GetLocation()))
				{
					Beam->SetVectorParameter(FName("Target"), BeamHitResult.Location);
				}
			}
		}
	}
}

void AShooterCharacter::PlayGunFireMontage() const
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if(AnimInstance && HipFireMontage)
	{
		AnimInstance->Montage_Play(HipFireMontage);
		AnimInstance->Montage_JumpToSection(FName("StartFire"));
	}
}

void AShooterCharacter::ReloadWeapon()
{
	if(CombatStates != ECombatStates::ECS_Unoccupied) return;
	if(EquippedWeapon == nullptr) return;
	if(CarryingAmmo() && !EquippedWeapon->ClipIsFull())
	{
		if(bAiming) StopAiming();
		CombatStates = ECombatStates::ECS_Reloading;
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if(AnimInstance && ReloadMontage)
		{
			AnimInstance->Montage_Play(ReloadMontage);
			AnimInstance->Montage_JumpToSection(EquippedWeapon->GetReloadMontageSection());
		}
	}
}

bool AShooterCharacter::CarryingAmmo()
{
	if(EquippedWeapon == nullptr) return false;
	const auto AmmoType = EquippedWeapon->GetAmmoType();
	if(AmmoMap.Contains(AmmoType))
	{
		return AmmoMap[AmmoType] > 0;
	}
	return false;
}

void AShooterCharacter::StartFireTimer()
{
	if(!EquippedWeapon) return;
	CombatStates = ECombatStates::ECS_FireTimerInProgress;
	GetWorldTimerManager().SetTimer(AutoFireTimer, this, &AShooterCharacter::AutoFireReset, EquippedWeapon->GetAutoFireRate());
}

bool AShooterCharacter::GetBeamEndLocation(const FVector& MuzzleSocketLocation, FHitResult& OutHitResult) const
{
	FVector OutBeamLocation;
	FHitResult CrosshairHitResult;
	if(TraceUnderCrosshair(CrosshairHitResult, OutBeamLocation))
	{
		OutBeamLocation = CrosshairHitResult.Location;
	}
	const FVector WeaponTraceStart(MuzzleSocketLocation);
	const FVector StartToEnd(OutBeamLocation - MuzzleSocketLocation);
	const FVector WeaponTraceEnd(MuzzleSocketLocation + StartToEnd + 1.25f);
	GetWorld()->LineTraceSingleByChannel(OutHitResult, WeaponTraceStart, WeaponTraceEnd, ECC_Visibility);
	if(!OutHitResult.bBlockingHit)
	{
		OutHitResult.Location = OutBeamLocation;
		return false;
	}
	return true;
}

void AShooterCharacter::AimingButtonPressed()
{
	bAimingButtonPressed = true;
	if(CombatStates != ECombatStates::ECS_Reloading && CombatStates != ECombatStates::ECS_Equipping)
	{
		Aim();
	}
}

void AShooterCharacter::AimingButtonReleased()
{
	bAimingButtonPressed = false;
	StopAiming();
}


void AShooterCharacter::Aim()
{
	bAiming = true;
	GetCharacterMovement()->MaxWalkSpeed = CrouchMovementSpeed;
}

void AShooterCharacter::StopAiming()
{
	bAiming = false;
	if(!bCrouching)
	{
		GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;
	}
}

void AShooterCharacter::FireButtonPressed()
{
	bFireButtonPressed = true;
	FireWeapon();
}

void AShooterCharacter::AutoFireReset()
{
	CombatStates = ECombatStates::ECS_Unoccupied;
	if(EquippedWeapon == nullptr) return;
	if(WeaponHasAmmo())
	{
		if(bFireButtonPressed && EquippedWeapon->GetAutomatic())
		{
			FireWeapon();
		}
	}
	else
	{
		ReloadWeapon();
	}
}

EPhysicalSurface AShooterCharacter::GetSurfaceType()
{
	FHitResult HitResult;
	const FVector Start = GetActorLocation();
	const FVector End = Start + FVector(0.f, 0.f, -400.f);
	FCollisionQueryParams QueryParams;
	QueryParams.bReturnPhysicalMaterial = true;
	GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, QueryParams);
	return UPhysicalMaterial::DetermineSurfaceType(HitResult.PhysMaterial.Get());
}

void AShooterCharacter::FinishReloading()
{
	CombatStates = ECombatStates::ECS_Unoccupied;
	if(bAimingButtonPressed) Aim();
	if(EquippedWeapon == nullptr) return;
	const auto AmmoType = EquippedWeapon->GetAmmoType();
	if(AmmoMap.Contains(EquippedWeapon->GetAmmoType()))
	{
		int32 CarriedAmmo = AmmoMap[AmmoType];
		const int32 MagEmptySpace = EquippedWeapon->GetMagazineCapacity() - EquippedWeapon->GetAmmo();
		if(MagEmptySpace > CarriedAmmo)
		{
			EquippedWeapon->ReloadAmmo(CarriedAmmo);
			CarriedAmmo = 0;
			AmmoMap.Add(AmmoType, CarriedAmmo);
		}
		else
		{
			EquippedWeapon->ReloadAmmo(MagEmptySpace);
			CarriedAmmo -= MagEmptySpace;
			AmmoMap.Add(AmmoType, CarriedAmmo);
		}
	}
}

void AShooterCharacter::FinishEquipping()
{
	CombatStates = ECombatStates::ECS_Unoccupied;
	if(bAimingButtonPressed) Aim();
}

void AShooterCharacter::GrabClip()
{
	if(EquippedWeapon == nullptr) return;
	if(HandSceneComponent == nullptr) return;
	const int32 ClipBoneIndex = EquippedWeapon->GetItemMesh()->GetBoneIndex(EquippedWeapon->GetClipBoneName());
	ClipTransform = EquippedWeapon->GetItemMesh()->GetBoneTransform(ClipBoneIndex);
	const FAttachmentTransformRules AttachmentRules(EAttachmentRule::KeepRelative, true);
	HandSceneComponent->AttachToComponent(GetMesh(), AttachmentRules, FName(TEXT("hand_l")));
	HandSceneComponent->SetWorldTransform(ClipTransform);
	EquippedWeapon->SetMovingClip(true);
}

void AShooterCharacter::ReleaseClip() const
{
	EquippedWeapon->SetMovingClip(false);
}

AWeapon* AShooterCharacter::SpawnDefaultWeapon() const
{
	if(DefaultWeaponClass)
	{
		return GetWorld()->SpawnActor<AWeapon>(DefaultWeaponClass);
	}
	return nullptr;
}

void AShooterCharacter::EquipWeapon(AWeapon* WeaponToEquip, const bool bSwapping)
{
	if(WeaponToEquip)
	{
		const USkeletalMeshSocket* HandSocket = GetMesh()->GetSocketByName(FName("RightHandSocket"));
		if(HandSocket)
		{
			HandSocket->AttachActor(WeaponToEquip, GetMesh());
		}
		if(EquippedWeapon == nullptr)
		{
			EquipItemDelegate.Broadcast(-1, WeaponToEquip->GetSlotIndex());
		}
		else if(!bSwapping)
		{
			EquipItemDelegate.Broadcast(EquippedWeapon->GetSlotIndex(), WeaponToEquip->GetSlotIndex());
		}
		EquippedWeapon = WeaponToEquip;
		EquippedWeapon->SetItemState(EItemState::EIS_Equipped);
	}
}

void AShooterCharacter::DropWeapon() const
{
	if(EquippedWeapon)
	{
		const FDetachmentTransformRules DetachmentTransformRules(EDetachmentRule::KeepWorld, true);
		EquippedWeapon->GetItemMesh()->DetachFromComponent(DetachmentTransformRules);
		EquippedWeapon->SetItemState(EItemState::EIS_Falling);
		EquippedWeapon->ThrowWeapon();
	}
}

void AShooterCharacter::SwapWeapon(AWeapon* WeaponToSwap)
{
	if(Inventory.Num() - 1 >= EquippedWeapon->GetSlotIndex())
	{
		Inventory[EquippedWeapon->GetSlotIndex()] = WeaponToSwap;
		WeaponToSwap->SetSlotIndex(EquippedWeapon->GetSlotIndex());
	}
	DropWeapon();
	EquipWeapon(WeaponToSwap, true);
	TraceHitItem = nullptr;
	TraceHitItemLastFrame = nullptr;
}

void AShooterCharacter::InitializeAmmoMap()
{
	AmmoMap.Add(EAmmoType::EAT_9mm, Starting9mmAmmo);
	AmmoMap.Add(EAmmoType::EAT_AR, StartingARAmmo);
}

bool AShooterCharacter::WeaponHasAmmo() const
{
	if(EquippedWeapon == nullptr) return false;
	return EquippedWeapon->GetAmmo() > 0;
}

void AShooterCharacter::ReloadButtonPressed()
{
	ReloadWeapon();
}

//Movement

void AShooterCharacter::CrouchButtonPressed()
{
	if(!GetCharacterMovement()->IsFalling())
	{
		bCrouching = !bCrouching;
	}
	if(bCrouching)
	{
		GetCharacterMovement()->MaxWalkSpeed = CrouchMovementSpeed;
		GetCharacterMovement()->GroundFriction = CrouchingGroundFriction;
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;
		GetCharacterMovement()->GroundFriction = BaseGroundFriction;
	}
}

void AShooterCharacter::DefaultWeaponButtonPressed()
{
	if(EquippedWeapon->GetSlotIndex() == 0) return;
	ExchangeInventoryItems(EquippedWeapon->GetSlotIndex(), 0);
}

void AShooterCharacter::Weapon1ButtonPressed()
{
	if(EquippedWeapon->GetSlotIndex() == 1) return;
	ExchangeInventoryItems(EquippedWeapon->GetSlotIndex(), 1);
}

void AShooterCharacter::Weapon2ButtonPressed()
{
	if(EquippedWeapon->GetSlotIndex() == 2) return;
	ExchangeInventoryItems(EquippedWeapon->GetSlotIndex(), 2);
}

void AShooterCharacter::Weapon3ButtonPressed()
{
	if(EquippedWeapon->GetSlotIndex() == 3) return;
	ExchangeInventoryItems(EquippedWeapon->GetSlotIndex(), 3);
}

void AShooterCharacter::Weapon4ButtonPressed()
{
	if(EquippedWeapon->GetSlotIndex() == 4) return;
	ExchangeInventoryItems(EquippedWeapon->GetSlotIndex(), 4);
}

void AShooterCharacter::Weapon5ButtonPressed()
{
	if(EquippedWeapon->GetSlotIndex() == 5) return;
	ExchangeInventoryItems(EquippedWeapon->GetSlotIndex(), 5);
}

void AShooterCharacter::Jump()
{
	if(bCrouching)
	{
		bCrouching = false;
		GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;
	}
	else
	{
		ACharacter::Jump();
	}
}


void AShooterCharacter::InterpCapsuleHalfHeight(float DeltaTime) const
{
	float TargetCapsuleHalfHeight;
	if(bCrouching)
	{
		TargetCapsuleHalfHeight = CrouchingCapsuleHalfHeight;
	}
	else
	{
		TargetCapsuleHalfHeight = StandingCapsuleHalfHeight;
	}
	const float InterpHalfHeight = FMath::FInterpTo(GetCapsuleComponent()->GetScaledCapsuleHalfHeight(), TargetCapsuleHalfHeight, DeltaTime, 10.f);
	const float DeltaCapsuleHalfHeight = InterpHalfHeight - GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FVector MeshOffset(0.f, 0.f, -DeltaCapsuleHalfHeight);
	GetMesh()->AddLocalOffset(MeshOffset);
	GetCapsuleComponent()->SetCapsuleHalfHeight(InterpHalfHeight);
}

// Items

void AShooterCharacter::SelectButtonPressed()
{
	if(CombatStates != ECombatStates::ECS_Unoccupied) return;
	if(TraceHitItem)
	{
		TraceHitItem->StartItemCurve(this, true);
		TraceHitItem = nullptr;
	}
}

void AShooterCharacter::GetPickupItem(AItemBase* Item)
{
	Item->PlayEquipSound();
	if(const auto Weapon = Cast<AWeapon>(Item))
	{
		if(Inventory.Num() < INVENTORY_CAPACITY)
		{
			UE_LOG(LogTemp, Warning, TEXT("%i"), Inventory.Num())
			Weapon->SetSlotIndex(Inventory.Num());
			Inventory.Add(Weapon);
			Weapon->SetItemState(EItemState::EIS_PickedUp);
		}
		else
		{
			SwapWeapon(Weapon);
		}
	}
	if(const auto Ammo = Cast<AAmmo>(Item))
	{
		PickupAmmo(Ammo);
	}
}

void AShooterCharacter::PickupAmmo(AAmmo* Ammo)
{
	if(AmmoMap.Find(Ammo->GetAmmoType()))
	{
		int32 AmmoCount = AmmoMap[Ammo->GetAmmoType()];
		AmmoCount += Ammo->GetItemCount();
		AmmoMap[Ammo->GetAmmoType()] = AmmoCount;
	}
	if(EquippedWeapon->GetAmmoType() == Ammo->GetAmmoType())
	{
		if(EquippedWeapon->GetAmmo() == 0)
		{
			ReloadWeapon();
		}
	}
	Ammo->Destroy();
}

void AShooterCharacter::InitializeInterpLocations()
{
	const FInterpLocation WeaponLocation(WeaponInterpComponent, 0);
	InterpLocations.Add(WeaponLocation);
	InterpLocations.Add(FInterpLocation(InterpComponent_01, 0));
	InterpLocations.Add(FInterpLocation(InterpComponent_02, 0));
	InterpLocations.Add(FInterpLocation(InterpComponent_03, 0));
	InterpLocations.Add(FInterpLocation(InterpComponent_04, 0));
	InterpLocations.Add(FInterpLocation(InterpComponent_05, 0));
	InterpLocations.Add(FInterpLocation(InterpComponent_06, 0));
}

FInterpLocation AShooterCharacter::GetInterpLocation(const int32 Index) const
{
	if(Index <= InterpLocations.Num())
	{
		return InterpLocations[Index];
	}
	return FInterpLocation();
}

int32 AShooterCharacter::GetInterpLocationIndex() const
{
	int32 LowestIndex = 1;
	int32 LowestCount = INT_MAX;
	for(int32 i = 1; i < InterpLocations.Num(); i++)
	{
		if(InterpLocations[i].ItemCount < LowestCount)
		{
			LowestIndex = i;
			LowestCount = InterpLocations[i].ItemCount;
		}
	}
	return LowestIndex;
}

void AShooterCharacter::IncrementInterpLocationItemCount(const int32 Index, const int32 Amount)
{
	if(Amount < -1 || Amount > 1) return;
	if(InterpLocations.Num() >= Index)
	{
		InterpLocations[Index].ItemCount += Amount;
	}
}

void AShooterCharacter::IncrementOverlappedItemCount(const int8 Amount)
{
	if(OverlappedItemCount + Amount <= 0)
	{
		OverlappedItemCount = 0;
		bShouldTraceForItems = false;
	}
	else
	{
		OverlappedItemCount += Amount;
		bShouldTraceForItems = true;
	}
}

void AShooterCharacter::TraceForItems()
{
	if(bShouldTraceForItems)
	{
		FHitResult ItemTraceResult;
		FVector HitLocation;
		if(TraceUnderCrosshair(ItemTraceResult, HitLocation))
		{
			TraceHitItem = Cast<AItemBase>(ItemTraceResult.GetActor());
			if(const auto TraceHitWeapon = Cast<AWeapon>(TraceHitItem))
			{
				if(HighlightedSlot == -1)
				{
					HighlightInventorySlot();
				}
			}
			else
			{
				if(HighlightedSlot != -1)
				{
					UnhighlightInventorySlot();
				}
			}
			if(TraceHitItem && TraceHitItem->GetItemState() == EItemState::EIS_EquipInterping)
			{
				TraceHitItem = nullptr;
			}
			if(TraceHitItem && TraceHitItem->GetPickupWidget())
			{
				TraceHitItem->GetPickupWidget()->SetVisibility(true);
				TraceHitItem->EnableCustomDepth();
				if(Inventory.Num() >= INVENTORY_CAPACITY)
				{
					TraceHitItem->SetCharacterInventoryFull(true);
				}
				else
				{
					TraceHitItem->SetCharacterInventoryFull(false);
				}
			}
			if(TraceHitItemLastFrame)
			{
				if(TraceHitItem != TraceHitItemLastFrame)
				{
					TraceHitItemLastFrame->GetPickupWidget()->SetVisibility(false);
					TraceHitItemLastFrame->DisableCustomDepth();
				}
			}
			TraceHitItemLastFrame = TraceHitItem;
		}
	}
	else if (TraceHitItemLastFrame)
	{
		TraceHitItemLastFrame->GetPickupWidget()->SetVisibility(false);
		TraceHitItemLastFrame->DisableCustomDepth();
	}
}

//Camera

/*
FVector AShooterCharacter::GetCameraInterpLocation() const
{
	const FVector CameraWorldLocation = FollowCamera->GetComponentLocation();
	const FVector CameraForward = FollowCamera->GetForwardVector();
	return CameraWorldLocation + CameraForward * CameraInterpDistance + FVector(0.f, 0.f, CameraInterpElevation);
}
*/

void AShooterCharacter::CameraInterpZoom(float DeltaTime)
{
	if(bAiming)
	{
		CameraCurrentFOV = FMath::FInterpTo(CameraCurrentFOV, CameraZoomedFOV, DeltaTime, ZoomInterpSpeed);
	}
	else
	{
		CameraCurrentFOV = FMath::FInterpTo(CameraCurrentFOV, CameraDefaultFOV, DeltaTime, ZoomInterpSpeed);
	}
	GetFollowCamera()->SetFieldOfView(CameraCurrentFOV);
}

void AShooterCharacter::StartPickupSoundTimer()
{
	bShouldPlayPickupSound = false;
	GetWorldTimerManager().SetTimer(PickupSoundTimer, this, &AShooterCharacter::ResetPickupSoundTimer, PickupSoundResetTime);
}

void AShooterCharacter::StartEquipSoundTimer()
{
	bShouldPlayEquipSound = false;
	GetWorldTimerManager().SetTimer(EquipSoundTimer, this, &AShooterCharacter::ResetEquipSoundTimer, EquipSoundResetTime);
}

void AShooterCharacter::ResetPickupSoundTimer()
{
	bShouldPlayPickupSound = true;
}

void AShooterCharacter::ResetEquipSoundTimer()
{
	bShouldPlayEquipSound = true;
}

void AShooterCharacter::ExchangeInventoryItems(const int32 CurrentItemIndex, const int32 NewItemIndex)
{
	const bool bCanExchangeItems = CurrentItemIndex != NewItemIndex &&
		NewItemIndex < Inventory.Num() &&
		CombatStates == ECombatStates::ECS_Unoccupied || CombatStates == ECombatStates::ECS_Equipping;
	if(bCanExchangeItems)
	{
		if(bAiming)
		{
			StopAiming();
		}
		const auto OldEquippedWeapon = EquippedWeapon;
		const auto NewWeapon = Cast<AWeapon>(Inventory[NewItemIndex]);
		EquipWeapon(NewWeapon);
		OldEquippedWeapon->SetItemState(EItemState::EIS_PickedUp);
		NewWeapon->SetItemState(EItemState::EIS_Equipped);
		CombatStates = ECombatStates::ECS_Equipping;
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if(AnimInstance && EquipMontage)
		{
			AnimInstance->Montage_Play(EquipMontage, 1.f);
			AnimInstance->Montage_JumpToSection(FName("Equip"));
		}
		NewWeapon->PlayEquipSound(true);
	}
}

int32 AShooterCharacter::GetEmptyInventorySlot()
{
	for(int32 i = 0; i < Inventory.Num(); i++)
	{
		if(Inventory[i] == nullptr)
		{
			return i;
		}
	}
	if(Inventory.Num() < INVENTORY_CAPACITY)
	{
		return Inventory.Num();
	}
	return -1;
}

void AShooterCharacter::HighlightInventorySlot()
{
	const int32 EmptySlot = GetEmptyInventorySlot();
	HighlightIconDelegate.Broadcast(EmptySlot, true);
	HighlightedSlot = EmptySlot;
}

void AShooterCharacter::UnhighlightInventorySlot()
{
	HighlightIconDelegate.Broadcast(HighlightedSlot, false);
	HighlightedSlot = -1;
}

//Crosshair

void AShooterCharacter::CalculateCrosshairSpread(float DeltaTime)
{
	const FVector2D WalkSpeedRange(0.f, 600.f);
	const FVector2D VelocityMultiplayerRange(0.f, 1.f);
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;
	CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplayerRange, Velocity.Size());

	if(GetCharacterMovement()->IsFalling())
	{
		CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2.25f, DeltaTime, 2.25f);
	}
	else
	{
		CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0.f, DeltaTime, 30.f);
	}

	if(bAiming)
	{
		CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.6f, DeltaTime, 30.f);
	}
	else
	{
		CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.f, DeltaTime, 30.f);
	}

	if(bFiringBullet)
	{
		CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.3f, DeltaTime, 60.f);
	}
	else
	{
		CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.f, DeltaTime, 60.f);
	}

	CrosshairSpreadMultiplayer = 0.5f + CrosshairVelocityFactor + CrosshairInAirFactor - CrosshairAimFactor + CrosshairShootingFactor;
}

void AShooterCharacter::StartCrosshairBulletFire()
{
	bFiringBullet = true;
	GetWorldTimerManager().SetTimer(CrosshairShootTimer, this, &AShooterCharacter::FinishCrosshairBulletFire, ShootTimeDuration);
}

void AShooterCharacter::FinishCrosshairBulletFire()
{
	bFiringBullet = false;
}

bool AShooterCharacter::TraceUnderCrosshair(FHitResult& OutHitResult, FVector& OutHitLocation) const
{
	FVector2D ViewportSize;
	if(GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}
	const FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;
	const bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(UGameplayStatics::GetPlayerController(this, 0),
	                                                                     CrosshairLocation, CrosshairWorldPosition, CrosshairWorldDirection);
	if(bScreenToWorld)
	{
		const FVector Start(CrosshairWorldPosition);
		const FVector End(CrosshairWorldPosition + CrosshairWorldDirection * 50000);
		GetWorld()->LineTraceSingleByChannel(OutHitResult, Start, End, ECC_Visibility);
		if(OutHitResult.bBlockingHit)
		{
			OutHitLocation = OutHitResult.Location;
			return true;
		}
	}
	return false;
}







