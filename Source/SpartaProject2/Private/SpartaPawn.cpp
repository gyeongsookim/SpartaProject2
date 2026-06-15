#include "SpartaPawn.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/World.h"

ASpartaPawn::ASpartaPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	CapsuleComp = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComp"));
	SetRootComponent(CapsuleComp);
	CapsuleComp->SetCapsuleHalfHeight(88.0f);
	CapsuleComp->SetCapsuleRadius(34.0f);
	CapsuleComp->SetSimulatePhysics(false);

	MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetupAttachment(CapsuleComp);
	MeshComp->SetSimulatePhysics(false);

	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComp"));
	SpringArmComp->SetupAttachment(CapsuleComp);
	SpringArmComp->TargetArmLength = 300.0f;
	SpringArmComp->bUsePawnControlRotation = false;

	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	CameraComp->SetupAttachment(SpringArmComp, USpringArmComponent::SocketName);

	// 초기 물리 세팅 값
	GroundMaxSpeed = 500.0f;
	AirControlRatio = 0.4f;
	RotationSpeed = 50.0f;
	CustomGravity = -980.0f;
	TraceDistance = 89.0f;

	bIsSprinting = false;
	bIsPawnFalling = true;
	VerticalVelocity = 0.0f;
	UpDownInput = 0.0f;
	RollInput = 0.0f;
	CurrentVelocity = FVector::ZeroVector;
}

void ASpartaPawn::BeginPlay()
{
	Super::BeginPlay();
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			if (InputMappingContext) Subsystem->AddMappingContext(InputMappingContext, 0);
		}
	}
}

void ASpartaPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASpartaPawn::OnMove);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &ASpartaPawn::OnMove);

		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ASpartaPawn::OnLook);

		EnhancedInputComponent->BindAction(UpDownAction, ETriggerEvent::Triggered, this, &ASpartaPawn::OnUpDown);
		EnhancedInputComponent->BindAction(UpDownAction, ETriggerEvent::Completed, this, &ASpartaPawn::OnUpDown);
		
		EnhancedInputComponent->BindAction(RollAction, ETriggerEvent::Triggered, this, &ASpartaPawn::OnRoll);
		EnhancedInputComponent->BindAction(RollAction, ETriggerEvent::Completed, this, &ASpartaPawn::OnRoll);
	}
}

void ASpartaPawn::OnMove(const FInputActionValue& Value) { MoveInput = Value.Get<FVector2D>(); }
void ASpartaPawn::OnLook(const FInputActionValue& Value) { LookInput = Value.Get<FVector2D>(); }
void ASpartaPawn::OnRoll(const FInputActionValue& Value) { RollInput = Value.Get<float>(); }

void ASpartaPawn::OnUpDown(const FInputActionValue& Value)
{
	float InputVal = Value.Get<float>();

	if (!bIsPawnFalling)
	{
		if (InputVal > 0.0f)
		{
			bIsPawnFalling = true;
			VerticalVelocity = 500.0f;
		}
		else if (InputVal < 0.0f)
		{
			bIsSprinting = true;
		}
		else
		{
			// 키를 떼면 달리기 해제
			bIsSprinting = false;
		}
	}
	else
	{
		UpDownInput = InputVal;
		bIsSprinting = false; 
	}
}

void ASpartaPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FHitResult HitResult;
	FVector StartLocation = GetActorLocation();
	FVector EndLocation = StartLocation + (FVector::DownVector * TraceDistance);
	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(this);

	bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_Visibility, TraceParams);
	
	if (bHit)
	{
		if (bIsPawnFalling && VerticalVelocity <= 0.0f)
		{
			bIsPawnFalling = false;
			VerticalVelocity = 0.0f;
			UpDownInput = 0.0f;
			
			FVector NewLoc = GetActorLocation();
			NewLoc.Z = HitResult.ImpactPoint.Z + 88.0f;
			SetActorLocation(NewLoc);
		}
	}
	else
	{
		bIsPawnFalling = true;
	}

	float CurrentSpeed = GroundMaxSpeed;
	if (bIsPawnFalling)
	{
		CurrentSpeed = GroundMaxSpeed * AirControlRatio;
	}
	else if (bIsSprinting)
	{
		CurrentSpeed = GroundMaxSpeed * 1.5f;
	}

	FVector TotalOffset = FVector::ZeroVector;
	FVector ForwardDirection = GetActorForwardVector();
	FVector RightDirection = GetActorRightVector();
	FVector UpDirection = GetActorUpVector();

	if (!MoveInput.IsNearlyZero())
	{
		TotalOffset += ForwardDirection * MoveInput.X * CurrentSpeed * DeltaTime;
		TotalOffset += RightDirection * MoveInput.Y * CurrentSpeed * DeltaTime;
	}

	if (bIsPawnFalling && !FMath::IsNearlyZero(UpDownInput))
	{
		TotalOffset += UpDirection * UpDownInput * CurrentSpeed * DeltaTime;
	}
	
	if (bIsPawnFalling)
	{
		VerticalVelocity += CustomGravity * DeltaTime;
		TotalOffset += FVector::UpVector * VerticalVelocity * DeltaTime;
	}

	AddActorWorldOffset(TotalOffset, true);
	CurrentVelocity = TotalOffset / DeltaTime;
	
	FRotator ActorRotation = FRotator::ZeroRotator;
	ActorRotation.Yaw = LookInput.X * RotationSpeed * DeltaTime;
	ActorRotation.Roll = RollInput * RotationSpeed * DeltaTime;
	AddActorLocalRotation(ActorRotation);

	if (!FMath::IsNearlyZero(LookInput.Y))
	{
		float PitchRotation = -LookInput.Y * RotationSpeed * DeltaTime;
		FRotator CurrentRelativeRot = SpringArmComp->GetRelativeRotation();
		float ClampedPitch = FMath::Clamp(CurrentRelativeRot.Pitch + PitchRotation, -60.0f, 60.0f);
		SpringArmComp->SetRelativeRotation(FRotator(ClampedPitch, 0.0f, 0.0f));
	}

	LookInput = FVector2D::ZeroVector;
	MoveInput = FVector2D::ZeroVector; 
}

FVector ASpartaPawn::GetVelocity() const
{
	return CurrentVelocity;
}