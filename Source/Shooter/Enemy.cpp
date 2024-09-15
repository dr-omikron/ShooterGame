
#include "Enemy.h"

#include "EnemyAIController.h"
#include "ShooterCharacter.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Blueprint/UserWidget.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Sound/SoundCue.h"

AEnemy::AEnemy():
	ImpactParticle(nullptr),
	HitMontage(nullptr),
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
	bCanHitReact(true),
	AttackMontage(nullptr),
	AttackLFast(TEXT("AttackLFast")),
	AttackRFast(TEXT("AttackRFast")),
	AttackL(TEXT("AttackL")),
	AttackR(TEXT("AttackR"))
{
	PrimaryActorTick.bCanEverTick = true;
	AgroSphere = CreateDefaultSubobject<USphereComponent>(TEXT("Agro Sphere"));
	AgroSphere->SetupAttachment(GetRootComponent());
	CombatRangeSphere = CreateDefaultSubobject<USphereComponent>(TEXT("Combat Range Sphere"));
	CombatRangeSphere->SetupAttachment(GetRootComponent());
}

void AEnemy::BeginPlay()
{
	Super::BeginPlay();

	AgroSphere->OnComponentBeginOverlap.AddDynamic(this, &AEnemy::AgroSphereOverlap);
	CombatRangeSphere->OnComponentBeginOverlap.AddDynamic(this, &AEnemy::CombatRangeOverlap);
	CombatRangeSphere->OnComponentEndOverlap.AddDynamic(this, &AEnemy::CombatRangeEndOverlap);

	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	EnemyAIController = Cast<AEnemyAIController>(GetController());

	if(EnemyAIController)
	{
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

void AEnemy::AgroSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if(OtherActor == nullptr) return;
	if(const auto Character = Cast<AShooterCharacter>(OtherActor))
	{
		EnemyAIController->GetBlackboardComponent()->SetValueAsObject(TEXT("Target"), Character);
	}
}

void AEnemy::CombatRangeOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if(OtherActor == nullptr) return;
	if(auto ShooterCharacter = Cast<AShooterCharacter>(OtherActor))
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
	if(auto ShooterCharacter = Cast<AShooterCharacter>(OtherActor))
	{
		bInAttackRange = false;
		if(EnemyAIController)
		{
			EnemyAIController->GetBlackboardComponent()->SetValueAsBool(TEXT("InAttackRange"), bInAttackRange);
		}
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

void AEnemy::PlayAttackMontage(const FName Section, const float PlayRate) const
{
	if(UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance(); AnimInstance && AttackMontage)
	{
		AnimInstance->Montage_Play(AttackMontage, PlayRate);
		AnimInstance->Montage_JumpToSection(Section, AttackMontage);
	}
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
