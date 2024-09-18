
#include "Explosive.h"

#include "Components/SphereComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

AExplosive::AExplosive():
	ExplosionParticle(nullptr),
	ExplosionSound(nullptr),
	Damage(100.f)
{
	PrimaryActorTick.bCanEverTick = true;
	ExplosiveMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Explosive Mesh"));
	SetRootComponent(ExplosiveMesh);
	OverlapSphere = CreateDefaultSubobject<USphereComponent>(TEXT("Overlap Sphere"));
	OverlapSphere->SetupAttachment(GetRootComponent());
}

void AExplosive::BeginPlay()
{
	Super::BeginPlay();
	
}

void AExplosive::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AExplosive::BulletHit_Implementation(FHitResult HitResult, AActor* Shooter, AController* ShooterController)
{
	IBulletHitInterface::BulletHit_Implementation(HitResult, Shooter, ShooterController);
	if(ExplosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, GetActorLocation());
	}
	if(ExplosionParticle)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionParticle, HitResult.Location, FRotator(0.f), true);
	}
	TArray<AActor*> OverlappingActors;
	GetOverlappingActors(OverlappingActors, ACharacter::StaticClass());
	for(auto Actor : OverlappingActors)
	{
		UGameplayStatics::ApplyDamage(Actor, Damage, ShooterController, Shooter, UDamageType::StaticClass());
	}
	Destroy();
}

