#include "Projectile.h"
#include "Kismet/GameplayStatics.h"
#include "HealthManager.h"
#include "Hovercraft.h"
#include "BasePlayerController.h"
#include "SuperNeonColosseumGameInstance.h"
#include "GameFramework/DamageType.h"
#include "Runtime/Engine/Public/TimerManager.h"

// Sets default values
AProjectile::AProjectile()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	ProjectileMesh				= CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	ProjectileTrailParticle		= CreateDefaultSubobject<UParticleSystemComponent>(TEXT("ProjectileTrailSource"));
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ExplosionComponent			= CreateDefaultSubobject<URadialForceComponent>(TEXT("ExplosionForce"));

	SetRootComponent(ProjectileMesh);
	ProjectileTrailParticle->SetupAttachment(RootComponent);
	ExplosionComponent->SetupAttachment(RootComponent);

	ProjectileMovementComponent->InitialSpeed = 2000.f;
	ProjectileMovementComponent->MaxSpeed = 3000.f;
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->bIsHomingProjectile = false;
	ProjectileMovementComponent->ProjectileGravityScale = 0;

	ExplosionComponent->Radius = 100;

	bExplosionForceEnabled = true;
	projectileDamage = 10;
	projectileSpeed = 1000;
	lifeSpan = 3;


	
	ProjectileMesh->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);
}

// Called when the game starts or when spawned
void AProjectile::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AProjectile::Explode() {
	if(EndParticleSystem)
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), EndParticleSystem, GetActorLocation());
	//dynamic_cast<USuperNeonColosseumGameInstance*>(GetGameInstance())->SoundManager->MulticastPlaySoundAtLocation(GetActorLocation(), GetActorRotation(), "Play_Secondary_Impact");
	UAkGameplayStatics::PostEventAtLocation(ExplosionSoundEvent, GetActorLocation(), GetActorRotation(), "", GetWorld());
	Destroy();
}

void AProjectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit)
{
	if (AHovercraft* hitActor = dynamic_cast<AHovercraft*>(OtherActor)) {
		if (hitActor != GetOwner()->GetOwner()) {

			hitActor->HealthManagerComponent->TakeDamage(projectileDamage, (AHovercraft*)(GetOwner()->GetOwner()));

		}
		else {
			Destroy();
			return;
		}

	}
	
	Explode();

}
