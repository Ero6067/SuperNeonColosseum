#include "Hovercraft.h"
#include "SuperNeonColosseumPawn.h"
#include "WheeledVehicleMovementComponent4W.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "FOctoMath.h"
#include "DataRecorder.h"
#include "SuperNeonColosseumGameMode.h"
#include "Components/InputComponent.h"
#include "HealthManager.h"
#include "EnergyManager.h"
#include "UnrealNetwork.h"
#include "WeaponManager.h"
#include "Engine/World.h"
#include "Haptics/HapticFeedbackEffect_Base.h"
#include "Haptics/HapticFeedbackEffect_Buffer.h"
#include "SuperNeonColosseumGameInstance.h"
#include "Components/SkeletalMeshComponent.h"

AHovercraft::AHovercraft() {
	SetReplicates(true);

	HealthManagerComponent	= CreateDefaultSubobject<UHealthManager>(TEXT("HealthManager"));
	WeaponManagerComponent	= CreateDefaultSubobject<UWeaponManager>(TEXT("WeaponManager"));
	EnergyManagerComponent  = CreateDefaultSubobject<UEnergyManager>(TEXT("EnergyManager"));
	HovercraftMesh			= CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HovercraftMesh"));
	TankMuzzle				= CreateDefaultSubobject<USceneComponent>(TEXT("TankMuzzle"));
	RocketMuzzle			= CreateDefaultSubobject<USceneComponent>(TEXT("RocketMuzzle"));

	cameraLineTraceDistance					= 100000.f;
	boostForceToApply						= 100000.f;
	forwardAlignmentThresholdAngle			= 15.f;
	forwardAlignmentSpeed					= 7.f;
	groundedCheckLength						= 95.f;
	
	flyingGravityScale				        = 100.f;
	maxFlyingTime							= 3.5f;
	maxFlyingAngularVelocity				= FVector(22.f, 22.f, 22.f);

	HovercraftMesh->SetupAttachment(RootComponent);
	TankMuzzle->SetupAttachment(HovercraftMesh, NAME_None);
	RocketMuzzle->SetupAttachment(HovercraftMesh, NAME_None);
}

void AHovercraft::BeginPlay() {
	Super::BeginPlay();

	timer = RateOfFire;
	RocketTimer = RocketCD;
	LastShotTime = 0.f;

	//play engine sounds when the hovercraft gets created
	UAkGameplayStatics::PostEvent(nullptr, this, true, "Play_SFX_Merged_Engines");
}

void AHovercraft::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	check(PlayerInputComponent);

	PlayerInputComponent->BindAction("PrimaryFire", IE_Pressed, this, &AHovercraft::StartPrimaryFire);
	PlayerInputComponent->BindAction("PrimaryFire", IE_Released, this, &AHovercraft::StopPrimaryFire);
	PlayerInputComponent->BindAction("SecondaryFire", IE_Pressed, this, &AHovercraft::SecondaryFire);
	//PlayerInputComponent->BindAction("Boost", IE_Pressed, this, &AHovercraft::Booster);
}

void AHovercraft::Tick(float Delta)
{
	Super::Tick(Delta);
	timer = aTF ?  timer + Delta : RateOfFire;
	RocketTimer += Delta;

	if (aTF && timer >= RateOfFire) {
		PrimaryFire();
		timer = 0.0f;
	}

	if (!HealthManagerComponent->IsDead()) {
		DoCameraMovement();
		DoThrusterMovement();

		// Do sound
		float maxValue = 1000.f;

		RTPC_SpeedValue = (FMath::Min(GetVelocity().Size(), maxValue) / maxValue) * 100;

		UAkGameplayStatics::SetRTPCValue("Speed", RTPC_SpeedValue, 0, this);
		//WWise function( RTPC_SpeedValue );

		if (IsGrounded()) {
			flyingTimer = 0.0f;

		} else { 
			PreventFlyingAway(Delta);

		}
	}
}

/// UE4 Defaults ^

void AHovercraft::PrimaryFire() {
	if (!HealthManagerComponent->IsDead())
		WeaponManagerComponent->PrimaryFire(TankMuzzle->GetComponentLocation(), GetCameraLinetraceContactPoint(), TankMuzzle);
}

void AHovercraft::StartPrimaryFire() {
	aTF = true;
}

void AHovercraft::StopPrimaryFire() {
	aTF = false;
}

void AHovercraft::SecondaryFire() {
	check(GetWorld());

	if ((LastShotTime > 0 && GetWorld()->GetTimeSeconds() - LastShotTime < RocketCD) || HealthManagerComponent->IsDead())
	{
		UAkGameplayStatics::PostEventAttached(nullptr, this, NAME_None, true, "Play_sfx_secondary_empty_2");
		return;
	}

	/*if (!HealthManagerComponent->IsDead() && RocketTimer >= RocketCD)*/ //Sets the CD in seconds 
	{
		WeaponManagerComponent->SecondaryFire(RocketMuzzle->GetComponentLocation(), GetCameraLinetraceContactPoint(), RocketMuzzle);
		RocketTimer = 0;
		LastShotTime = GetWorld()->GetTimeSeconds();
		
	}
	/*if (RocketTimer < RocketCD) {
		UAkGameplayStatics::PostEventAttached(nullptr, this, NAME_None, true, "Play_UI_EMPTY_BOOST");
	}*/
}

void AHovercraft::DoThrusterMovement()
{
	
	float throttleForce = FMath::Abs(movementVector.X) + FMath::Abs(movementVector.Y);
	float stickAngle = -FMath::Atan2(movementVector.X, movementVector.Y) * 180 / PI;
	float vehicleAngleRelativeToCamera = UFOctoMath::GetDistanceFromAngles(SpringArm->GetComponentRotation().Yaw, this->GetActorRotation().Yaw);
	
	GetVehicleMovementComponent()->SetThrottleInput(throttleForce);

	if (throttleForce > 0.2f) 
		prevValidStickAngle = stickAngle;

	GetVehicleMovementComponent()->SetSteeringInput((UFOctoMath::GetDistanceFromAngles(vehicleAngleRelativeToCamera, prevValidStickAngle) / 180));
}

void AHovercraft::DoCameraMovement() {

	bool cond[3] = {
		SpringArm->GetComponentRotation().Pitch >= -50,	// Condition A / 0
		SpringArm->GetComponentRotation().Pitch <= 0,	// Condition B / 1
		lookVector.Y > 0.0								// Condition C / 2
	};

	if ((cond[1] && cond[2]) || (cond[0] && !cond[2]))
		SpringArm->AddLocalRotation(FRotator(lookVector.Y * aimSensitivity, 0.0, 0.0)); //Pitch


	SpringArm->AddRelativeRotation(FRotator(0.0, lookVector.X * aimSensitivity, 0.0)); //Yaw
	
	ServerRotateSpringarm(SpringArm->GetComponentRotation());
}

#pragma region RotateSpringarm (THE RASMUS EFFECT?!)

void AHovercraft::RotateSpringarm_Implementation(FRotator rotation)
{
	SpringArm->SetRelativeRotation(rotation);
}

void AHovercraft::ServerRotateSpringarm_Implementation(FRotator rotation)
{
	RotateSpringarm(rotation);
}

bool AHovercraft::ServerRotateSpringarm_Validate(FRotator rotation)
{
	return true;
}

#pragma endregion Replicated Function

#pragma region AlignForwardWithTarget

void AHovercraft::AlignForwardWithTarget() {
	float vehicleAngleRelativeToCamera = UFOctoMath::GetDistanceFromAngles(SpringArm->GetComponentRotation().Yaw, GetActorRotation().Yaw);

	if (FMath::Abs(vehicleAngleRelativeToCamera) <= forwardAlignmentThresholdAngle)
		return;

	ServerAlignForwardWithTarget_Implementation(vehicleAngleRelativeToCamera);
}

void AHovercraft::ServerAlignForwardWithTarget_Implementation(float angle) {
	if (!(GetMesh()->IsSimulatingPhysics()))
		return;

	GetMesh()->AddTorque(GetActorQuat().RotateVector(FVector(0.0f, 0.0f, forwardAlignmentSpeed * (angle >= 0 ? 1 : -1))), NAME_None, true);
}

bool AHovercraft::ServerAlignForwardWithTarget_Validate(float angle) {
	return true;
}

#pragma endregion Replicated Method

#pragma region PreventFlyingAway

void AHovercraft::PreventFlyingAway(float delta) {

	// If it is not simulating physics, don't treat it as flying.
	if (!(GetMesh()->IsSimulatingPhysics()))
		return;

	// Keep flying time.
	flyingTimer += delta;

	if (flyingTimer > maxFlyingTime) {
		// If it flies more than max flying time, try to get it controllable.
		ServerPreventFlyingAway();

		flyingTimer = 0.0f;
	}
}

void AHovercraft::ServerPreventFlyingAway_Implementation() {
	
	// Add additional force to land faster.
	GetMesh()->AddForce(FVector(0.0f, 0.0f, GetWorld()->GetGravityZ() * GetMesh()->GetMass() * flyingGravityScale));

	// Clamp angular velocity to prevent spinning too much.
	const FVector currentAngularVelocity = GetMesh()->GetPhysicsAngularVelocity();
	
	GetMesh()->SetPhysicsAngularVelocity(FVector(
		FMath::Clamp(currentAngularVelocity.X, 0.0f, maxFlyingAngularVelocity.X),
		FMath::Clamp(currentAngularVelocity.Y, 0.0f, maxFlyingAngularVelocity.Y),
		FMath::Clamp(currentAngularVelocity.Z, 0.0f, maxFlyingAngularVelocity.Z)
	));
	
}

bool AHovercraft::ServerPreventFlyingAway_Validate() {
	return true;
}

#pragma endregion Replicated Method

#pragma region Booster

void AHovercraft::Booster() {
	APlayerController* player = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	ServerBooster(player);
}

void AHovercraft::ServerBooster_Implementation(APlayerController* player) {
	if (IsGrounded() && EnergyManagerComponent->currentEnergy >= BoostEnergyCost)
	{
		EnergyManagerComponent->UseEnergy(BoostEnergyCost);
		//GetMesh()->AddForce(FVector(movementVector.X, movementVector.Y, 0) * boostForceToApply, NAME_None, true);
		GetMesh()->AddForce(GetMesh()->GetForwardVector() * boostForceToApply, NAME_None, true);
		PlayBoosterSound(true);
		PlayParticleForBoost();

		//Record data for analyzation
		ASuperNeonColosseumGameMode* sncGameMode = (ASuperNeonColosseumGameMode*)GetWorld()->GetAuthGameMode();
		UDataRecorder* dataRecorder = sncGameMode->DataRecorderComponent;
		dataRecorder->RecordBoost(player, GetActorLocation());
	}
}

bool AHovercraft::ServerBooster_Validate(APlayerController* player) {
	return true;
}

#pragma endregion Replicated Method

#pragma region PlayBoosterSound

void AHovercraft::PlayBoosterSound(bool activated) {
	ServerPlayBoosterSound(activated);
}

void AHovercraft::ServerPlayBoosterSound_Implementation(bool activated) {

		if (activated)
			UAkGameplayStatics::PostEventAttached(nullptr, this, NAME_None, true, "Play_SFX_new_boost");
}

bool AHovercraft::ServerPlayBoosterSound_Validate(bool activated) {
	return true;
}
#pragma endregion

void AHovercraft::WallRide() {
	FCollisionQueryParams RV_TraceParams = FCollisionQueryParams(FName(TEXT("RV_Trace")), true, this);
	RV_TraceParams.bTraceComplex = true;
	RV_TraceParams.bReturnPhysicalMaterial = false;

	FHitResult RV_Hit(ForceInit);

	// //Linetrace. If hitting something, apply gravity in that direction.
	if (GetWorld()->LineTraceSingleByChannel(RV_Hit, GravityTraceOrigin->GetComponentLocation(), (GetMesh()->GetUpVector() * -50) + GravityTraceOrigin->GetComponentLocation(), ECC_Visibility, RV_TraceParams)) {
		GetMesh()->AddForce((GetWorld()->GetGravityZ() * GetMesh()->GetMass()) * RV_Hit.ImpactNormal, NAME_None, true);
	} else {
		GetMesh()->AddForce(FVector(0, 0, (GetWorld()->GetGravityZ() * GetMesh()->GetMass())), NAME_None, true);
	}
}

bool AHovercraft::IsGrounded() {
	FCollisionQueryParams RV_TraceParams = FCollisionQueryParams(FName(TEXT("RV_Trace")), true, this);
	RV_TraceParams.bTraceComplex = true;
	RV_TraceParams.bReturnPhysicalMaterial = false;

	FHitResult RV_Hit(ForceInit);

	return GetWorld()->LineTraceSingleByChannel(RV_Hit, GravityTraceOrigin->GetComponentLocation(), (GravityTraceOrigin->GetUpVector() * -groundedCheckLength) + GravityTraceOrigin->GetComponentLocation(), ECC_Visibility, RV_TraceParams);
}

FVector AHovercraft::GetCameraLinetraceContactPoint() {
	FCollisionQueryParams RV_TraceParams = FCollisionQueryParams(FName(TEXT("RV_Trace")), true, this);
	RV_TraceParams.bTraceComplex = true;
	RV_TraceParams.bReturnPhysicalMaterial = false;

	FHitResult RV_Hit(ForceInit);

	if (!GetWorld()->LineTraceSingleByChannel(RV_Hit, Camera->GetComponentLocation(), (Camera->GetForwardVector() * cameraLineTraceDistance) + Camera->GetComponentLocation(), ECC_Visibility, RV_TraceParams))
		return Camera->GetForwardVector() * cameraLineTraceDistance;

	return RV_Hit.ImpactPoint;
}

void AHovercraft::DestroyHovercraft_Implementation()
{

	// TODO:
	// Shake camera
	if (DeathCameraShake)
		UGameplayStatics::PlayWorldCameraShake(GetWorld(), DeathCameraShake, GetActorLocation(), 0.0f, DeathCameraShakeOuterRadius, 1.0, false);
	
	// Create particles...
	DeathExplosion();

	// Disable control... 

	// Make Pawn invisible.. Or spawn a "Destroyed" mesh..
	// Display message on screen...
	// Disable UI?

	// Tell server that this hovercraft has died
	BroadcastDeath();
}

void AHovercraft::FellOutOfWorld(const class UDamageType& DmgType) {
	//Super::FellOutOfWorld(DmgType);
	
	if (!HealthManagerComponent->IsDead())
		HealthManagerComponent->TakeDamage(100000);
}

#pragma region DeathExplosion
void AHovercraft::DeathExplosion() {
	if (DeathExplosionParticle)
		ServerDeathExplosion();
}

void AHovercraft::ServerDeathExplosion_Implementation() {
	MulticastDeathExplosion();
	dynamic_cast<USuperNeonColosseumGameInstance*>(GetGameInstance())->SoundManager->MulticastPlaySoundAtLocation(GetActorLocation(), GetActorRotation(), "Play_SFX_DestroyHovercraft");
}

bool AHovercraft::ServerDeathExplosion_Validate() {
	return true;
}

void AHovercraft::MulticastDeathExplosion_Implementation() {
	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), DeathExplosionParticle, GetActorLocation());
}

bool AHovercraft::MulticastDeathExplosion_Validate() {
	return true;
}

#pragma endregion Replicated Method

#pragma region Audio

FString AHovercraft::GenerateAudioEventName(const FString prefix, const FString suffix) {
	return prefix + VehicleId + suffix;
}

#pragma endregion
