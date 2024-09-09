
#include "ItemBase.h"

#include "ShooterCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Curves/CurveVector.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

AItemBase::AItemBase()
{
 	PrimaryActorTick.bCanEverTick = true;

	ItemMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Item Mesh"));
	SetRootComponent(ItemMesh);
	ItemCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("Item Collision"));
	ItemCollisionBox->SetupAttachment(ItemMesh);
	ItemCollisionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	ItemCollisionBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	PickupWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("Pickup Widget"));
	PickupWidget->SetupAttachment(GetRootComponent());
	AreaSphere = CreateDefaultSubobject<USphereComponent>("Area Sphere");
	AreaSphere->SetupAttachment(GetRootComponent());
	ItemName = "Default";
	ItemCount = 0;
	ItemRarity = EItemRarity::EIR_Common;
	ItemState = EItemState::EIS_Pickup;
	ItemType = EItemType::EIT_Ammo;
	ZCurveTime = 0.7f;
	ItemInterpStartLocation = FVector(0);
	CameraTargetLocation = FVector(0);
	bInterping = false;
	ItemInterpX = 0.f;
	ItemInterpY = 0.f;
	InterpInitialYawOffset = 0.f;
	InterpLocationIndex = 0;
	MaterialIndex = 0;
	bCanChangeCustomDepth = true;
	bCharacterInventoryFull = false;
	GlowAmount = 150.f;
	FresnelExponent = 3.f;
	FresnelReflectFraction = 4.f;
	PulseCurveTime = 5.f;
	SlotIndex = 0;
}

void AItemBase::BeginPlay()
{
	Super::BeginPlay();
	SetActiveStars();
	if(PickupWidget) PickupWidget->SetVisibility(false);
	if(AreaSphere)
	{
		AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &AItemBase::OnSphereOverlap);
		AreaSphere->OnComponentEndOverlap.AddDynamic(this, &AItemBase::OnSphereEndOverlap);
	}
	SetItemProperties(ItemState);
	InitializeCustomDepth();
	if(ItemState == EItemState::EIS_Pickup)
	{
		StartPulseTimer();
	}
}

void AItemBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	ItemInterp(DeltaTime);
	UpdatePulse();
}

void AItemBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	const FString RarityTablePath = FString(TEXT("/Script/Engine.DataTable'/Game/_Game/DataTables/DT_ItemRarity.DT_ItemRarity'"));
	const UDataTable* RarityTableObject = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *RarityTablePath));
	if(RarityTableObject)
	{
		FItemRarityTable* RarityRow = nullptr;
		switch (ItemRarity)
		{
		case EItemRarity::EIR_Damaged:
			RarityRow = RarityTableObject->FindRow<FItemRarityTable>(FName("Damaged"), TEXT(""));
			break;
		case EItemRarity::EIR_Common:
			RarityRow = RarityTableObject->FindRow<FItemRarityTable>(FName("Common"), TEXT(""));
			break;
		case EItemRarity::EIR_Uncommon:
			RarityRow = RarityTableObject->FindRow<FItemRarityTable>(FName("Uncommon"), TEXT(""));
			break;
		case EItemRarity::EIR_Rare:
			RarityRow = RarityTableObject->FindRow<FItemRarityTable>(FName("Rare"), TEXT(""));
			break;
		case EItemRarity::EIR_Legendary:
			RarityRow = RarityTableObject->FindRow<FItemRarityTable>(FName("Legendary"), TEXT(""));
			break;
		default:;
		}
		if(RarityRow)
		{
			GlowMaterialColor = RarityRow->GlowColor;
			LightWidgetColor = RarityRow->LightColor;
			DarkWidgetColor = RarityRow->DarkColor;
			NumbersOfStarsRarity = RarityRow->NumberOfStars;
			IconSlotWidgetBackground = RarityRow->IconBackground;
			if(GetItemMesh())
			{
				GetItemMesh()->SetCustomDepthStencilValue(RarityRow->CustomDepthStencil);
			}
		}
	}
	if(MaterialInstance)
	{
		DynamicMaterialInstance = UMaterialInstanceDynamic::Create(MaterialInstance, this);
		DynamicMaterialInstance->SetVectorParameterValue(TEXT("FresnelColor"), GlowMaterialColor);
		ItemMesh->SetMaterial(MaterialIndex, DynamicMaterialInstance);
		EnableGlowMaterial();
	}
}

void AItemBase::EnableCustomDepth()
{
	if(bCanChangeCustomDepth)
	{
		ItemMesh->SetRenderCustomDepth(true);
	}
}

void AItemBase::DisableCustomDepth()
{
	if(bCanChangeCustomDepth)
	{
		ItemMesh->SetRenderCustomDepth(false);
	}
}

void AItemBase::InitializeCustomDepth()
{
	DisableCustomDepth();
}


void AItemBase::EnableGlowMaterial() const
{
	if(DynamicMaterialInstance)
	{
		DynamicMaterialInstance->SetScalarParameterValue(TEXT("GlowBlendAlpha"), 1.f);
	}
}

void AItemBase::DisableGlowMaterial() const
{
	if(DynamicMaterialInstance)
	{
		DynamicMaterialInstance->SetScalarParameterValue(TEXT("GlowBlendAlpha"), 0.f);
	}
}

void AItemBase::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if(OtherActor)
	{
		if(AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(OtherActor))
		{
			ShooterCharacter->IncrementOverlappedItemCount(1);
		}
	}
}

void AItemBase::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if(OtherActor)
	{
		if(AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(OtherActor))
		{
			ShooterCharacter->IncrementOverlappedItemCount(-1);
			ShooterCharacter->UnhighlightInventorySlot();
		}
	}
}

void AItemBase::SetActiveStars()
{
	for (int32 i = 0; i < 5; i++)
	{
		ActiveStars.Add(false);
	}
	switch (ItemRarity)
	{
	case EItemRarity::EIR_Damaged:
		ActiveStars[0] = true;
		break;
	case EItemRarity::EIR_Common:
		ActiveStars[0] = true;
		ActiveStars[1] = true;
		break;
	case EItemRarity::EIR_Uncommon:
		ActiveStars[0] = true;
		ActiveStars[1] = true;
		ActiveStars[2] = true;
		break;
	case EItemRarity::EIR_Rare:
		ActiveStars[0] = true;
		ActiveStars[1] = true;
		ActiveStars[2] = true;
		ActiveStars[3] = true;
		break;
	case EItemRarity::EIR_Legendary:
		ActiveStars[0] = true;
		ActiveStars[1] = true;
		ActiveStars[2] = true;
		ActiveStars[3] = true;
		ActiveStars[4] = true;
		break;
	default: ;
	}
}

void AItemBase::SetItemProperties(const EItemState State) const
{
	switch (State)
	{
	case EItemState::EIS_Pickup:
		ItemMesh->SetSimulatePhysics(false);
		ItemMesh->SetVisibility(true);
		ItemMesh->SetEnableGravity(false);
		ItemMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
		ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		AreaSphere->SetCollisionResponseToAllChannels(ECR_Overlap);
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		ItemCollisionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
		ItemCollisionBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		ItemCollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		break;
	case EItemState::EIS_Equipped:
		ItemMesh->SetSimulatePhysics(false);
		ItemMesh->SetVisibility(true);
		ItemMesh->SetEnableGravity(false);
		ItemMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
		ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		AreaSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ItemCollisionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
		ItemCollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		PickupWidget->SetVisibility(false);
		break;
	case EItemState::EIS_EquipInterping:
		PickupWidget->SetVisibility(false);
		ItemMesh->SetSimulatePhysics(false);
		ItemMesh->SetVisibility(true);
		ItemMesh->SetEnableGravity(false);
		ItemMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
		ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		AreaSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ItemCollisionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
		ItemCollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
	case EItemState::EIS_PickedUp:
		PickupWidget->SetVisibility(false);
		ItemMesh->SetSimulatePhysics(false);
		ItemMesh->SetVisibility(false);
		ItemMesh->SetEnableGravity(false);
		ItemMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
		ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		AreaSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ItemCollisionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
		ItemCollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
	case EItemState::EIS_Falling:
		ItemMesh->SetSimulatePhysics(true);
		ItemMesh->SetVisibility(true);
		ItemMesh->SetEnableGravity(true);
		ItemMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
		ItemMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		ItemMesh->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
		AreaSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ItemCollisionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
		ItemCollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;		
	default: ;
	}
}

void AItemBase::FinishInterping()
{
	bInterping = false;
	if(Character)
	{
		Character->IncrementInterpLocationItemCount(InterpLocationIndex, -1);
		Character->GetPickupItem(this);
		Character->UnhighlightInventorySlot();
	}
	SetActorScale3D(FVector(1.f, 1.f, 1.f));
	DisableGlowMaterial();
	bCanChangeCustomDepth = true;
	DisableCustomDepth();
}

void AItemBase::ItemInterp(float DeltaTime)
{
	if(!bInterping) return;
	if(Character && ItemZCurve)
	{
		const float ElapsedTime = GetWorldTimerManager().GetTimerElapsed(ItemInterpTimer);
		const float CurveValue = ItemZCurve->GetFloatValue(ElapsedTime);
		FVector ItemLocation = ItemInterpStartLocation;
		const FVector CameraInterpLocation = GetInterpLocation();
		const FVector ItemToCamera = FVector(0.f, 0.f, (CameraInterpLocation - ItemLocation).Z);
		const float DeltaZ = ItemToCamera.Size();
		const FVector CurrentLocation = GetActorLocation();
		const float InterpXValue = FMath::FInterpTo(CurrentLocation.X, CameraInterpLocation.X, DeltaTime, 30.f);
		const float InterpYValue = FMath::FInterpTo(CurrentLocation.Y, CameraInterpLocation.Y, DeltaTime, 30.f);
		ItemLocation.X = InterpXValue;
		ItemLocation.Y = InterpYValue;
		ItemLocation.Z += CurveValue * DeltaZ;
		SetActorLocation(ItemLocation, true,nullptr, ETeleportType::TeleportPhysics);
		const FRotator CameraRotation = Character->GetFollowCamera()->GetComponentRotation();
		const FRotator ItemRotation = FRotator(0.f, CameraRotation.Yaw + InterpInitialYawOffset, 0.f);
		SetActorRotation(ItemRotation, ETeleportType::TeleportPhysics);
		if(ItemScaleCurve)
		{
			const float ScaleCurveValue = ItemScaleCurve->GetFloatValue(ElapsedTime);
			SetActorScale3D(FVector(ScaleCurveValue, ScaleCurveValue, ScaleCurveValue));
		}
		
	}
}

FVector AItemBase::GetInterpLocation() const
{
	if(Character == nullptr) return FVector(0.f);
	UE_LOG(LogTemp, Warning, TEXT("%d"), ItemType)
	switch (ItemType)
	{
	case EItemType::EIT_Weapon:
		return Character->GetInterpLocation(0).SceneComponent->GetComponentLocation();
	case EItemType::EIT_Ammo:
		UE_LOG(LogTemp, Warning, TEXT("%s"), *Character->GetInterpLocation(InterpLocationIndex).SceneComponent->GetComponentLocation().ToString())
		return Character->GetInterpLocation(InterpLocationIndex).SceneComponent->GetComponentLocation();
	default: ;
	}
	return FVector(0.f);
}

void AItemBase::PlayPickupSound(const bool bForcePlaySound) const
{
	if(Character)
	{
		if(bForcePlaySound)
		{
			if(PickupSound)
			{
				UGameplayStatics::PlaySound2D(this, PickupSound);
			}
		}
		else if(Character->ShouldPlayPickupSound())
		{
			Character->StartPickupSoundTimer();
			if(PickupSound)
			{
				UGameplayStatics::PlaySound2D(this, PickupSound);
			}
		}
	}
}

void AItemBase::ResetPulseTimer()
{
	StartPulseTimer();
}

void AItemBase::StartPulseTimer()
{
	if(ItemState == EItemState::EIS_Pickup)
	{
		GetWorldTimerManager().SetTimer(PulseTimer, this, &AItemBase::ResetPulseTimer, PulseCurveTime);
	}
}

void AItemBase::UpdatePulse() const
{
	float ElapsedTime = 0.f;
	FVector CurveValue = FVector(0.f);
	switch (ItemState)
	{
	case EItemState::EIS_Pickup:
		if(PulseCurve)
		{
			ElapsedTime = GetWorldTimerManager().GetTimerElapsed(PulseTimer);
			CurveValue = PulseCurve->GetVectorValue(ElapsedTime);
		}
		break;
	case EItemState::EIS_EquipInterping:
		if(InterpingPulseCurve)
		{
			ElapsedTime = GetWorldTimerManager().GetTimerElapsed(ItemInterpTimer);
			CurveValue = InterpingPulseCurve->GetVectorValue(ElapsedTime);
		}
		break;
	default:;
	}
	if(DynamicMaterialInstance)
	{
		DynamicMaterialInstance->SetScalarParameterValue(TEXT("GlowAmount"), CurveValue.X * GlowAmount);
		DynamicMaterialInstance->SetScalarParameterValue(TEXT("FresnelExponent"), CurveValue.Y * FresnelExponent);
		DynamicMaterialInstance->SetScalarParameterValue(TEXT("FresnelReflectFraction"), CurveValue.Z * FresnelReflectFraction);
	}
}

void AItemBase::PlayEquipSound(const bool bForcePlaySound) const
{
	if(Character)
	{
		if(bForcePlaySound)
		{
			if(EquipSound)
			{
				UGameplayStatics::PlaySound2D(this, EquipSound);
			}	
		}
		else if(Character->ShouldPlayEquipSound())
		{
			Character->StartEquipSoundTimer();
			if(EquipSound)
			{
				UGameplayStatics::PlaySound2D(this, EquipSound);
			}
		}
	}
}

void AItemBase::SetItemState(const EItemState State)
{
	ItemState = State;
	SetItemProperties(State);	
}

void AItemBase::StartItemCurve(AShooterCharacter* Char, const bool bForcePlaySound)
{
	Character = Char;
	InterpLocationIndex = Character->GetInterpLocationIndex();
	Character->IncrementInterpLocationItemCount(InterpLocationIndex, 1);
	PlayPickupSound(bForcePlaySound);
	ItemInterpStartLocation = GetActorLocation();
	bInterping = true;
	SetItemState(EItemState::EIS_EquipInterping);
	GetWorldTimerManager().ClearTimer(PulseTimer);
	GetWorldTimerManager().SetTimer(ItemInterpTimer, this, &AItemBase::FinishInterping, ZCurveTime);
	const float CameraRotationYaw = Character->GetFollowCamera()->GetComponentRotation().Yaw;
	const float ItemRotationYaw = GetActorRotation().Yaw;
	InterpInitialYawOffset = ItemRotationYaw - CameraRotationYaw;
	bCanChangeCustomDepth = false;
}
