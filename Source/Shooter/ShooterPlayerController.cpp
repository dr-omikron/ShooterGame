
#include "ShooterPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "ShooterCharacter.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/Character.h"

AShooterPlayerController::AShooterPlayerController()
{
	BaseTurnRate = 1.f;
	BaseLookUpRate = 1.f;
	HipTurnRate = 1.f;
	HipLookUpRate = 1.f;
	AimingTurnRate = 0.6f;
	AimingLookUpRate = 0.6f;
	ShooterCharacter = nullptr;
	HUDOverlay = nullptr;
}

void AShooterPlayerController::BeginPlay()
{
	Super::BeginPlay();

	check(ShooterContext);

	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());

	check(Subsystem);
	Subsystem->AddMappingContext(ShooterContext, 0);

	ShooterCharacter = Cast<AShooterCharacter>(GetCharacter());
	check(ShooterCharacter);

	if(HUDOverlayClass)
	{
		HUDOverlay = CreateWidget<UUserWidget>(this, HUDOverlayClass);
		if(HUDOverlay)
		{
			HUDOverlay->AddToViewport();
			HUDOverlay->SetVisibility(ESlateVisibility::Visible);
		}
	}
}

void AShooterPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
}

void AShooterPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(InputComponent);
	
	EnhancedInputComponent->BindAction(IA_MoveAction, ETriggerEvent::Triggered, this, &AShooterPlayerController::Move);
	EnhancedInputComponent->BindAction(IA_CameraAction, ETriggerEvent::Triggered, this, &AShooterPlayerController::CameraMovement);
	EnhancedInputComponent->BindAction(IA_JumpAction, ETriggerEvent::Started, this, &AShooterPlayerController::Jump);
	EnhancedInputComponent->BindAction(IA_JumpAction, ETriggerEvent::Completed, this, &AShooterPlayerController::StopJumping);
	EnhancedInputComponent->BindAction(IA_FireAction, ETriggerEvent::Started, this, &AShooterPlayerController::FirePressed);
	EnhancedInputComponent->BindAction(IA_FireAction, ETriggerEvent::Completed, this, &AShooterPlayerController::FireReleased);
	EnhancedInputComponent->BindAction(IA_Aiming, ETriggerEvent::Started, this, &AShooterPlayerController::StartAiming);
	EnhancedInputComponent->BindAction(IA_Aiming, ETriggerEvent::Completed, this, &AShooterPlayerController::StopAiming);
	EnhancedInputComponent->BindAction(IA_Select, ETriggerEvent::Started, this, &AShooterPlayerController::StartSelect);
	EnhancedInputComponent->BindAction(IA_Select, ETriggerEvent::Completed, this, &AShooterPlayerController::StopSelect);
	EnhancedInputComponent->BindAction(IA_Reload, ETriggerEvent::Started, this, &AShooterPlayerController::Reloading);
	EnhancedInputComponent->BindAction(IA_Crouch, ETriggerEvent::Started, this, &AShooterPlayerController::Crouching);
	EnhancedInputComponent->BindAction(IA_SelectDefaultWeapon, ETriggerEvent::Started, this, &AShooterPlayerController::SelectDefaultWeapon);
	EnhancedInputComponent->BindAction(IA_SelectWeapon1, ETriggerEvent::Started, this, &AShooterPlayerController::SelectWeapon1);
	EnhancedInputComponent->BindAction(IA_SelectWeapon2, ETriggerEvent::Started, this, &AShooterPlayerController::SelectWeapon2);
	EnhancedInputComponent->BindAction(IA_SelectWeapon3, ETriggerEvent::Started, this, &AShooterPlayerController::SelectWeapon3);
	EnhancedInputComponent->BindAction(IA_SelectWeapon4, ETriggerEvent::Started, this, &AShooterPlayerController::SelectWeapon4);
	EnhancedInputComponent->BindAction(IA_SelectWeapon5, ETriggerEvent::Started, this, &AShooterPlayerController::SelectWeapon5);
}

void AShooterPlayerController::Move(const FInputActionValue& InputActionValue)
{
	const FVector2d InputAxisVector = InputActionValue.Get<FVector2d>();
	const FRotator Rotation = GetControlRotation();
	const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);

	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	ShooterCharacter->AddMovementInput(ForwardDirection, InputAxisVector.Y);
	ShooterCharacter->AddMovementInput(RightDirection, InputAxisVector.X);
}

void AShooterPlayerController::CameraMovement(const FInputActionValue& InputActionValue)
{
	const FVector2d InputAxisVector = InputActionValue.Get<FVector2d>();
	SetLookRates();
	AddYawInput(InputAxisVector.X * BaseTurnRate);
	AddPitchInput(InputAxisVector.Y * BaseLookUpRate);
}

void AShooterPlayerController::SetLookRates()
{
	if(ShooterCharacter->GetAiming())
	{
		BaseTurnRate = AimingTurnRate;
		BaseLookUpRate = AimingLookUpRate;
	}
	else
	{
		BaseTurnRate = HipTurnRate;
		BaseLookUpRate = HipLookUpRate;
	}
}
