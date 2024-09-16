
#include "Enemy.h"

#include "EnemyAIController.h"
#include "ShooterCharacter.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Blueprint/UserWidget.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Sound/SoundCue.h"

AEnemy::AEnemy():
	ImpactParticle(nullptr),
	HitMontage(nullptr),
	AttackMontage(nullptr),
	ImpactSound(nullptr),
	Health(100.f),
	MaxHealth(100.f),
	HealthBarDisplayTime(4.f),
	HitReactTimeMin(0.5f),
	HitReactTimeMax(3.f),
	HitNumberDestroyTime(1.5f),
	BehaviorTree(nullptr),
	bStunned(false),
	StunChance(0.5f),
	BaseDamage(20.f),
	AttackLFast(TEXT("AttackLFast")),
	AttackRFast(TEXT("AttackRFast")),
	AttackL(TEXT("AttackL")),
	AttackR(TEXT("AttackR")),
	bCanHitReact(true),
	LeftWeaponSocket(TEXT("FX_Trail_L_01")),
	RightWeaponSocket(TEXT("FX_Trail_R_01")),
	bCanAttack(true),
	ResetTimeAttack(1.f)
{
	PrimaryActorTick.bCanEverTick = true;
	AgroSphere = CreateDefaultSubobject<USphereComponent>(TEXT("Agro Sphere"));
	AgroSphere->SetupAttachment(GetRootComponent());
	CombatRangeSphere = CreateDefaultSubobject<USphereComponent>(TEXT("Combat Range Sphere"));
	CombatRangeSphere->SetupAttachment(GetRootComponent());
	LeftWeaponCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("Left Weapon Collision"));
	LeftWeaponCollision->SetupAttachment(GetMesh(), FName("LeftWeaponBone"));
	RightWeaponCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("Right Weapon Collision"));
	RightWeaponCollision->SetupAttachment(GetMesh(), FName("RightWeaponBone"));
}

void AEnemy::BeginPlay()
{
	Super::BeginPlay();

	AgroSphere->OnComponentBeginOverlap.AddDynamic(this, &AEnemy::AgroSphereOverlap);
	CombatRangeSphere->OnComponentBeginOverlap.AddDynamic(this, &AEnemy::CombatRangeOverlap);
	CombatRangeSphere->OnComponentEndOverlap.AddDynamic(this, &AEnemy::CombatRangeEndOverlap);
	LeftWeaponCollision->OnComponentBeginOverlap.AddDynamic(this, &AEnemy::LeftWeaponOverlap);
	RightWeaponCollision->OnComponentBeginOverlap.AddDynamic(this, &AEnemy::RightWeaponOverlap);

	LeftWeaponCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LeftWeaponCollision->SetCollisionObjectType(ECC_WorldDynamic);
	LeftWeaponCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	LeftWeaponCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	RightWeaponCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RightWeaponCollision->SetCollisionObjectType(ECC_WorldDynamic);
	RightWeaponCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	RightWeaponCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	
	EnemyAIController = Cast<AEnemyAIController>(GetController());

	if(EnemyAIController)
	{
		EnemyAIController->GetBlackboardComponent()->SetValueAsBool(FName(TEXT("CanAttack")), true);
		
		const FVector WorldPatrolPoint1 = UKismetMathLibrary::TransformLocation(GetActorTransform(), PatrolPoint1);
		const FVector WorldPatrolPoint2 = UKismetMathLibrary::TransformLocation(GetActorTransform(), PatrolPoint2);
		EnemyAIController->GetBlackboardComponent()->SetValueAsVector(TEXT("PatrolPoint1"), WorldPatrolPoint1);
		EnemyAIController->GetBlackboardComponent()->SetValueAsVector(TEXT("PatrolPoint2"), WorldPatrolPoint2);
		EnemyAIController->RunBehaviorTree(BehaviorTree);
	}
	
	
}

void AEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateHitNumbers();
}

void AEnemy::UpdateHitNumbers()
{
	for(const auto& HitPair : HitNumbers)
	{
		UUserWidget* HitNumber = HitPair.Key;
		const FVector Location = HitPair.Value;
		FVector2D ScreenPosition(0.f);
		UGameplayStatics::ProjectWorldToScreen(GetWorld()->GetFirstPlayerController(), Location, ScreenPosition);
		HitNumber->SetPositionInViewport(ScreenPosition);
	}
}

void AEnemy::DoDamage(AShooterCharacter* DamagedActor)
{
	UGameplayStatics::ApplyDamage(DamagedActor, BaseDamage, EnemyAIController, this, UDamageType::StaticClass());
	if(DamagedActor->GetMeleeImpactSound())
	{
		UGameplayStatics::PlaySoundAtLocation(this, DamagedActor->GetMeleeImpactSound(), GetActorLocation());
	}
}

void AEnemy::SpawnBlood(const AShooterCharacter* DamagedActor, const FName SocketName) const
{
	if(const USkeletalMeshSocket* TipSocket = GetMesh()->GetSocketByName(SocketName))
	{
		const FTransform SocketTransform = TipSocket->GetSocketTransform(GetMesh());
		if(DamagedActor->GetBloodParticles())
		{
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), DamagedActor->GetBloodParticles(), SocketTransform);
		}
	}
}

void AEnemy::StunCharacter(AShooterCharacter* DamagedActor)
{
	if(DamagedActor)
	{
		if(FMath::FRandRange(0.f, 1.f) <= DamagedActor->GetStunChance())
		{
			DamagedActor->Stun();
		}
	}
}

void AEnemy::ResetCanAttack()
{
	bCanAttack = true;
	if(EnemyAIController)
	{
		EnemyAIController->GetBlackboardComponent()->SetValueAsBool(FName(TEXT("CanAttack")), bCanAttack);
	}
}

void AEnemy::ActivateRightWeapon() const
{
	RightWeaponCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

void AEnemy::DeactivateRightWeapon() const
{
	RightWeaponCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AEnemy::ActivateLeftWeapon() const
{
	LeftWeaponCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

void AEnemy::DeactivateLeftWeapon() const
{
	LeftWeaponCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AEnemy::AgroSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                               UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if(OtherActor == nullptr) return;
	if(const auto Character = Cast<AShooterCharacter>(OtherActor))
	{
		if(EnemyAIController)
		{
			EnemyAIController->GetBlackboardComponent()->SetValueAsObject(TEXT("Target"), Character);
		}
	}
}

void AEnemy::CombatRangeOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if(OtherActor == nullptr) return;
	if(Cast<AShooterCharacter>(OtherActor))
	{
		bInAttackRange = true;
		if(EnemyAIController)
		{
			EnemyAIController->GetBlackboardComponent()->SetValueAsBool(TEXT("InAttackRange"), bInAttackRange);
		}
	}
}

void AEnemy::CombatRangeEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if(OtherActor == nullptr) return;
	if(Cast<AShooterCharacter>(OtherActor))
	{
		bInAttackRange = false;
		if(EnemyAIController)
		{
			EnemyAIController->GetBlackboardComponent()->SetValueAsBool(TEXT("InAttackRange"), bInAttackRange);
		}
	}
}

void AEnemy::LeftWeaponOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if(OtherActor == nullptr) return;
	if(const auto Character = Cast<AShooterCharacter>(OtherActor))
	{
		DoDamage(Character);
		SpawnBlood(Character, LeftWeaponSocket);
		StunCharacter(Character);
	}
}

void AEnemy::RightWeaponOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if(OtherActor == nullptr) return;
	if(const auto Character = Cast<AShooterCharacter>(OtherActor))
	{
		DoDamage(Character);
		SpawnBlood(Character, RightWeaponSocket);
		StunCharacter(Character);
	}
}

void AEnemy::StoreHitNumber(UUserWidget* HitNumber, const FVector Location)
{
	HitNumbers.Add(HitNumber, Location);
	FTimerHandle HitNumberTimer;
	FTimerDelegate HitNumberDelegate;
	HitNumberDelegate.BindUFunction(this, FName("DestroyHitNumber"), HitNumber);
	GetWorldTimerManager().SetTimer(HitNumberTimer, HitNumberDelegate, HitNumberDestroyTime, false);
}

void AEnemy::PlayHitMontage(const FName Section, const float PlayRate)
{
	if(bCanHitReact)
	{
		if(UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance(); AnimInstance && HitMontage)
		{
			AnimInstance->Montage_Play(HitMontage, PlayRate);
			AnimInstance->Montage_JumpToSection(Section, HitMontage);
		}
		bCanHitReact = false;
		const float HitReactTime = FMath::FRandRange(HitReactTimeMin, HitReactTimeMax);
		GetWorldTimerManager().SetTimer(HitReactTimer, this, &AEnemy::ResetHitReactTimer, HitReactTime);
	}
}

void AEnemy::SetStunned(const bool Stunned)
{
	bStunned = Stunned;
	EnemyAIController->GetBlackboardComponent()->SetValueAsBool(TEXT("Stunned"), bStunned);
}

void AEnemy::PlayAttackMontage(const FName Section, const float PlayRate)
{
	if(UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance(); AnimInstance && AttackMontage)
	{
		AnimInstance->Montage_Play(AttackMontage, PlayRate);
		AnimInstance->Montage_JumpToSection(Section, AttackMontage);
	}
	bCanAttack = false;
	GetWorldTimerManager().SetTimer(AttackWaitTimer, this, &AEnemy::ResetCanAttack, ResetTimeAttack);
	if(EnemyAIController)
	{
		EnemyAIController->GetBlackboardComponent()->SetValueAsBool(FName(TEXT("CanAttack")), bCanAttack);
	}
}

FName AEnemy::GetAttackSectionName() const
{
	FName SectionName;
	switch (FMath::RandRange(1, 4))
	{
	case 1:
		SectionName = AttackLFast;
		break;
	case 2:
		SectionName = AttackRFast;
		break;
	case 3:
		SectionName = AttackL;
		break;
	case 4:
		SectionName = AttackR;
		break;
	default:;
	}
	return SectionName;
}

void AEnemy::ResetHitReactTimer()
{
	bCanHitReact = true;
}

void AEnemy::DestroyHitNumber(UUserWidget* HitNumber)
{
	HitNumbers.Remove(HitNumber);
	HitNumber->RemoveFromParent();
}

void AEnemy::ShowHealthBar_Implementation()
{
	GetWorldTimerManager().ClearTimer(HeathBarTimer);
	GetWorldTimerManager().SetTimer(HeathBarTimer, this, &AEnemy::HideHealthBar, HealthBarDisplayTime);
}


void AEnemy::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AEnemy::BulletHit_Implementation(FHitResult HitResult)
{
	IBulletHitInterface::BulletHit_Implementation(HitResult);
	if(ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	}
	if(ImpactParticle)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticle, HitResult.Location, FRotator(0.f), true);
	}
	ShowHealthBar();
	if(const float Stunned = FMath::FRandRange(0.f, 1.f); Stunned <= StunChance)
	{
		PlayHitMontage(FName("HitReactFront"));
		SetStunned(true);
	}
	
}

float AEnemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator,
	AActor* DamageCauser)
{
	Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if(EnemyAIController)
	{
		EnemyAIController->GetBlackboardComponent()->SetValueAsObject(FName(TEXT("Target")), DamageCauser);
	}
	
	if(Health - DamageAmount <= 0.f)
	{
		Health = 0.f;
		Die();
	}
	else
	{
		Health -= DamageAmount;
	}
	
	return DamageAmount;
}


void AEnemy::Die()
{
	HideHealthBar();
}
