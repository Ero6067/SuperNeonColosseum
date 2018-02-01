#include "WeaponManager.h"
#include "Weapons/Weapon.h"
#include "Engine/World.h"

UWeaponManager::UWeaponManager() {
}

void UWeaponManager::BeginPlay() {
	Super::BeginPlay();

	World = GetOwner()->GetWorld();

	if (World->GetAuthGameMode())
	{
		FActorSpawnParameters SpawnInfo;
		SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnInfo.Owner = (AHovercraft*)GetOwner();
		SpawnInfo.Instigator = (AHovercraft*)GetOwner();

		if(PrimaryWeapon)
			PrimaryWeaponInstance = World->SpawnActor<AWeapon>(*PrimaryWeapon, FTransform(), SpawnInfo);
		if(SecondaryWeapon)
			SecondaryWeaponInstance = World->SpawnActor<AWeapon>(*SecondaryWeapon, FTransform(), SpawnInfo);
	}
}

#pragma region PrimaryFire

void UWeaponManager::PrimaryFire(FVector startPoint, FVector destination, USceneComponent* tankMuzzleComponent) {
	APlayerController* player = UGameplayStatics::GetPlayerController(World, 0);
	ServerPrimaryFire(player, startPoint, destination, tankMuzzleComponent);
}

void UWeaponManager::ServerPrimaryFire_Implementation(APlayerController* player, FVector startPoint, FVector destination, USceneComponent* tankMuzzleComponent)
{
	if (PrimaryWeaponInstance)
		PrimaryWeaponInstance->Fire(startPoint, destination, tankMuzzleComponent);

	// Record data for analysing
	ASuperNeonColosseumGameMode* sncGameMode = (ASuperNeonColosseumGameMode*)World->GetAuthGameMode();
	if (sncGameMode) {
		UDataRecorder* dataRecorder = sncGameMode->DataRecorderComponent;
		dataRecorder->RecordPrimary(player, startPoint);
	}
}

bool UWeaponManager::ServerPrimaryFire_Validate(APlayerController* player, FVector startPoint, FVector destination, USceneComponent* tankMuzzleComponent)
{
	return true;
}

#pragma endregion Replicated Method


#pragma region SecondaryFire

void UWeaponManager::SecondaryFire(FVector startPoint, FVector destination, USceneComponent* tankMuzzleComponent) {
	APlayerController* player = UGameplayStatics::GetPlayerController(World, 0);
	ServerSecondaryFire(player, startPoint, destination, tankMuzzleComponent);
}

void UWeaponManager::ServerSecondaryFire_Implementation(APlayerController* player, FVector startPoint, FVector destination, USceneComponent* tankMuzzleComponent)
{
	if(SecondaryWeaponInstance)
		SecondaryWeaponInstance->Fire(startPoint, destination, tankMuzzleComponent);

	ASuperNeonColosseumGameMode* sncGameMode = (ASuperNeonColosseumGameMode*)World->GetAuthGameMode();
	if (sncGameMode) {
		UDataRecorder* dataRecorder = sncGameMode->DataRecorderComponent;
		dataRecorder->RecordSecondary(player, startPoint);
	}
}

bool UWeaponManager::ServerSecondaryFire_Validate(APlayerController* player, FVector startPoint, FVector destination, USceneComponent* tankMuzzleComponent)
{
	return true;
}

#pragma endregion Replicated Method

