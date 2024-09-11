
#include "Weapon.h"

AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = true;
	ThrowWeaponTime = 0.7f;
	bFalling = false;
	bMovingClip = false;
	Ammo = 30;
	MagazineCapacity = 30;
	WeaponType = EWeaponType::EWT_SubmachineGun;
	AmmoType = EAmmoType::EAT_9mm;
	ReloadMontageSection = FName(TEXT("ReloadSMG"));
	ClipBoneName = FName(TEXT("smg_clip"));
	PreviousMaterialIndex = 0;
	WeaponDataTable = nullptr;
	CrosshairMiddle = nullptr;
	CrosshairLeft = nullptr;
	CrosshairRight = nullptr;
	CrosshairBottom = nullptr;
	CrosshairTop = nullptr;
	AutoFireRate = 0.08f;
	MuzzleFlash = nullptr;
	FireSound = nullptr;
	SlideDisplacement = 0.f;
	SlideDisplacementTime = 0.1f;
	bMovingClip = false;
	MaxSlideDisplacement = 4.f;
}

void AWeapon::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if(GetItemState() == EItemState::EIS_Falling && bFalling)
	{
		const FRotator MeshRotation(0.f, GetItemMesh()->GetComponentRotation().Yaw, 0.f);
		GetItemMesh()->SetWorldRotation(MeshRotation, false, nullptr, ETeleportType::TeleportPhysics);
	}
	UpdateSlideDisplacement();
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();
	if(BoneToHide != FName(""))
	{
		GetItemMesh()->HideBoneByName(BoneToHide, PBO_None);
	}
}

void AWeapon::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	const FString WeaponTablePath = FString(TEXT("/Script/Engine.DataTable'/Game/_Game/DataTables/DT_Weapons.DT_Weapons'"));
	const UDataTable* WeaponTableObject = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *WeaponTablePath));
	if(WeaponTableObject)
	{
		FWeaponDataTable* WeaponDataRow = nullptr;
		switch (WeaponType)
		{
		case EWeaponType::EWT_SubmachineGun:
			WeaponDataRow = WeaponTableObject->FindRow<FWeaponDataTable>(FName("SubmachineGun"), TEXT(""));
			break;
		case EWeaponType::EAT_AssaultRifle:
			WeaponDataRow = WeaponTableObject->FindRow<FWeaponDataTable>(FName("AssaultRifle"), TEXT(""));
			break;
		case EWeaponType::EWT_Pistol:
			WeaponDataRow = WeaponTableObject->FindRow<FWeaponDataTable>(FName("Pistol"), TEXT(""));
		default:;
		}
		if(WeaponDataRow)
		{
			AmmoType = WeaponDataRow->AmmoType;
			Ammo = WeaponDataRow->WeaponAmmo;
			MagazineCapacity = WeaponDataRow->MagazineCapacity;
			SetPickupSound(WeaponDataRow->PickupSound);
			SetEquipSound(WeaponDataRow->EquipSound);
			GetItemMesh()->SetSkeletalMesh(WeaponDataRow->ItemMesh);
			SetItemName(WeaponDataRow->ItemName);
			SetIconItem(WeaponDataRow->InventoryIcon);
			SetIconAmmo(WeaponDataRow->AmmoIcon);
			SetMaterialInstance(WeaponDataRow->MaterialInstance);
			PreviousMaterialIndex = GetMaterialIndex();
			GetItemMesh()->SetMaterial(PreviousMaterialIndex, nullptr);
			SetMaterialIndex(WeaponDataRow->MaterialIndex);
			SetClipBoneName(WeaponDataRow->ClipBoneName);
			SetReloadMontageSection(WeaponDataRow->ReloadMontageSectionName);
			GetItemMesh()->SetAnimInstanceClass(WeaponDataRow->AnimationBlueprint);
			CrosshairMiddle = WeaponDataRow->CrosshairMiddle;
			CrosshairLeft = WeaponDataRow->CrosshairLeft;
			CrosshairRight = WeaponDataRow->CrosshairRight;
			CrosshairBottom = WeaponDataRow->CrosshairBottom;
			CrosshairTop = WeaponDataRow->CrosshairTop;
			AutoFireRate = WeaponDataRow->AutoFireRate;
			MuzzleFlash = WeaponDataRow->MuzzleFlash;
			FireSound = WeaponDataRow->FireSound;
			BoneToHide = WeaponDataRow->BoneToHide;
		}
		if(GetMaterialInstance())
		{
			SetDynamicMaterialInstance(UMaterialInstanceDynamic::Create(GetMaterialInstance(), this));  
			GetDynamicMaterialInstance()->SetVectorParameterValue(TEXT("FresnelColor"), GetGlowColor());
			GetItemMesh()->SetMaterial(GetMaterialIndex(), GetDynamicMaterialInstance());
			EnableGlowMaterial();
		}
	}
	
	
}

void AWeapon::ThrowWeapon()
{
	const FRotator MeshRotation(0.f, GetItemMesh()->GetComponentRotation().Yaw, 0.f);
	GetItemMesh()->SetWorldRotation(MeshRotation, false, nullptr, ETeleportType::TeleportPhysics);
	const FVector MeshForward( GetItemMesh()->GetForwardVector());
	const FVector MeshRight(GetItemMesh()->GetRightVector());
	FVector ImpulseDirection = MeshRight.RotateAngleAxis(-20.f, MeshForward);
	float RandomRotation = FMath::FRandRange(10.f, 30.f);
	ImpulseDirection = ImpulseDirection.RotateAngleAxis(RandomRotation, FVector(0.f, 0.f, 1.f));
	ImpulseDirection *= 5000.f;
	GetItemMesh()->AddImpulse(ImpulseDirection);
	bFalling = true;
	GetWorldTimerManager().SetTimer(ThrowWeaponTimer, this, &AWeapon::StopFalling, ThrowWeaponTime);
	EnableGlowMaterial();
}

void AWeapon::DecrementAmmo()
{
	if(Ammo - 1 <= 0)
	{
		Ammo = 0;
	}
	else
	{
		--Ammo;
	}
}

void AWeapon::ReloadAmmo(const int32 Amount)
{
	checkf(Ammo + Amount <= MagazineCapacity, TEXT("Attempted to reload with more magazine capacity!"));
	Ammo += Amount;
}

bool AWeapon::ClipIsFull() const
{
	return Ammo >= MagazineCapacity;
}

void AWeapon::FinishMovingSlide()
{
	bMovingClip = false;
}

void AWeapon::UpdateSlideDisplacement()
{
	if(SlideDisplacementCurve && bMovingSlide)
	{
		const float ElapsedTime = GetWorldTimerManager().GetTimerElapsed(SlideTimer);
		const float CurveValue = SlideDisplacementCurve->GetFloatValue(ElapsedTime);
		SlideDisplacement = CurveValue * MaxSlideDisplacement;
	}
}

void AWeapon::StartSlideTimer()
{
	bMovingSlide = true;
	GetWorldTimerManager().SetTimer(SlideTimer, this, &AWeapon::FinishMovingSlide, SlideDisplacementTime);
}

void AWeapon::StopFalling()
{
	bFalling = false;
	SetItemState(EItemState::EIS_Pickup);
	StartPulseTimer();
}

