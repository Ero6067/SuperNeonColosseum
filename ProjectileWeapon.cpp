#include "ProjectileWeapon.h"
#include "Weapons/Projectiles/Projectile.h"
#include "Engine/World.h"
#include "SuperNeonColosseumGameInstance.h"
#include "Classes/Kismet/KismetMathLibrary.h"

AProjectileWeapon::AProjectileWeapon() : AWeapon() {

}

void AProjectileWeapon::BeginPlay() {

}

#pragma region Fire
void AProjectileWeapon::Fire(FVector startPoint, FVector destination, USceneComponent* tankMuzzleComponent) {
	if (ProjectileType) {
		if(Instigator)
			((AHovercraft*)Instigator)->HovercraftFired();
		ServerFire(startPoint, destination, tankMuzzleComponent);
	}
}

void AProjectileWeapon::ServerFire_Implementation(FVector startPoint, FVector destination, USceneComponent* tankMuzzleComponent) {
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnInfo.Owner = this;
	SpawnInfo.Instigator = Instigator;

	FRotator shootDirection = UKismetMathLibrary::FindLookAtRotation(startPoint, destination);

	//SpawnActorDeferred requires FinishSpawningActor
	AProjectile* Projectile = GetWorld()->SpawnActorDeferred<AProjectile>(ProjectileType, FTransform(shootDirection, startPoint), Instigator, Instigator, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	
	dynamic_cast<USuperNeonColosseumGameInstance*>(GetGameInstance())->SoundManager->MulticastPlaySoundAtLocation(startPoint, GetActorRotation(), ShootingSoundEventName);

	//UAkGameplayStatics::PostEventAtLocation(nullptr, startPoint, GetActorRotation(), ShootingSoundEventName, GetWorld());
	
	if (Projectile)
	{
		UPrimitiveComponent* PrimitiveRoot = Cast<UPrimitiveComponent>(Projectile->GetRootComponent());
		
		if (PrimitiveRoot) {
			PrimitiveRoot->IgnoreActorWhenMoving(Instigator, true);
		}
		
		if (ShootCameraShake)
			UGameplayStatics::PlayWorldCameraShake(GetWorld(), ShootCameraShake, GetActorLocation(), 0.0f, ShootCameraShakeOuterRadius, 1.0, false);

		UGameplayStatics::FinishSpawningActor(Projectile, FTransform(shootDirection, startPoint));
		SpawnMuzzleFlash(startPoint, shootDirection, tankMuzzleComponent);
	}
}

//void AProjectileWeapon::PlayFireSound_Implementation(FVector startPoint) {
//	UAkGameplayStatics::PostEventAtLocation(nullptr, startPoint, GetActorRotation(), ShootingSoundEventName, GetWorld());
//}

bool AProjectileWeapon::ServerFire_Validate(FVector startPoint, FVector destination, USceneComponent* tankMuzzleComponent) {
	return true;
}

#pragma endregion Replicated Method

#pragma region MuzzleFlash
void AProjectileWeapon::SpawnMuzzleFlash(FVector spawnPoint, FRotator rotation, USceneComponent* tankMuzzleComponent) {
	ServerSpawnMuzzleFlash(spawnPoint, rotation, tankMuzzleComponent);
}

void AProjectileWeapon::ServerSpawnMuzzleFlash_Implementation(FVector spawnPoint, FRotator rotation, USceneComponent* tankMuzzleComponent) {
	UGameplayStatics::SpawnEmitterAttached(MuzzleFlashParticleSystem, tankMuzzleComponent, NAME_None, spawnPoint, rotation, EAttachLocation::KeepWorldPosition);
}

bool AProjectileWeapon::ServerSpawnMuzzleFlash_Validate(FVector spawnPoint, FRotator rotation, USceneComponent* tankMuzzleComponent) {
	return true;
}
#pragma endregion ReplicatedMethod
