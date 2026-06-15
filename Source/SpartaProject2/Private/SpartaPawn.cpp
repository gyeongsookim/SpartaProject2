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

	GroundMaxSpeed = 500.0f;
	AirControlRatio = 0.4f;
	RotationSpeed = 50.0f;
	CustomGravity = -1500.0f;
	TraceDistance = 99.0f;	

	CurrentMovementState = EPawnMovementState::GroundWalking;
	bIsSprinting = false;
	VerticalVelocity = 0.0f;
	UpDownInput = 0.0f;
	RollInput = 0.0f;
	CurrentVelocity = FVector::ZeroVector;

	LastJumpTime = -1.0f;
	DoubleTapThreshold = 0.25f;

	SpringArmComp->bEnableCameraRotationLag = true;
	SpringArmComp->CameraRotationLagSpeed = 10.0f;
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

		EnhancedInputComponent->BindAction(UpDownAction, ETriggerEvent::Started, this, &ASpartaPawn::OnJumpStarted);
		EnhancedInputComponent->BindAction(UpDownAction, ETriggerEvent::Triggered, this, &ASpartaPawn::OnUpDownMovement);
		EnhancedInputComponent->BindAction(UpDownAction, ETriggerEvent::Completed, this, &ASpartaPawn::OnUpDownMovement);
		
		EnhancedInputComponent->BindAction(RollAction, ETriggerEvent::Triggered, this, &ASpartaPawn::OnRoll);
		EnhancedInputComponent->BindAction(RollAction, ETriggerEvent::Completed, this, &ASpartaPawn::OnRoll);
	}
}

void ASpartaPawn::OnMove(const FInputActionValue& Value) { MoveInput = Value.Get<FVector2D>(); }
void ASpartaPawn::OnLook(const FInputActionValue& Value) { LookInput = Value.Get<FVector2D>(); }
void ASpartaPawn::OnRoll(const FInputActionValue& Value) { RollInput = Value.Get<float>(); }

void ASpartaPawn::OnJumpStarted(const FInputActionValue& Value)
{
	float InputVal = Value.Get<float>();

	if (InputVal > 0.0f)
	{
		float CurrentTime = GetWorld()->GetTimeSeconds();

		if (CurrentMovementState == EPawnMovementState::GroundWalking)
		{
			CurrentMovementState = EPawnMovementState::NormalJumping;
			VerticalVelocity = 550.0f; 
			LastJumpTime = CurrentTime;
		}
		else if (CurrentMovementState == EPawnMovementState::NormalJumping)
		{
			if (CurrentTime - LastJumpTime <= DoubleTapThreshold)
			{
				CurrentMovementState = EPawnMovementState::Flying;
				VerticalVelocity = 0.0f;
			}
			LastJumpTime = CurrentTime;
		}
		else if (CurrentMovementState == EPawnMovementState::Flying)
		{
			if (CurrentTime - LastJumpTime <= DoubleTapThreshold)
			{
				CurrentMovementState = EPawnMovementState::NormalJumping;
				VerticalVelocity = 0.0f;
			}
			LastJumpTime = CurrentTime;
		}
	}
}

void ASpartaPawn::OnUpDownMovement(const FInputActionValue& Value)
{
	float InputVal = Value.Get<float>();

	if (FMath::IsNearlyZero(InputVal))
	{
		bIsSprinting = false;
		UpDownInput = 0.0f;
		return;
	}

	if (CurrentMovementState == EPawnMovementState::GroundWalking && InputVal < 0.0f)
	{
		bIsSprinting = true;
	}
	else if (CurrentMovementState == EPawnMovementState::Flying)
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
		if (VerticalVelocity <= 0.0f && CurrentMovementState != EPawnMovementState::GroundWalking)
		{
			CurrentMovementState = EPawnMovementState::GroundWalking;
			VerticalVelocity = 0.0f;
			UpDownInput = 0.0f;
		
			CurrentVelocity = FVector::ZeroVector; 
		
			FVector NewLoc = GetActorLocation();
			NewLoc.Z = HitResult.ImpactPoint.Z + 98.0f; 
			SetActorLocation(NewLoc);
		}
	}
	else if (!bHit && CurrentMovementState == EPawnMovementState::GroundWalking)
	{
		CurrentMovementState = EPawnMovementState::NormalJumping;
	}

	float CurrentSpeed = GroundMaxSpeed;
	
	if (CurrentMovementState == EPawnMovementState::NormalJumping)
	{
		CurrentSpeed = GroundMaxSpeed * AirControlRatio;
	}
	else if (bIsSprinting && CurrentMovementState == EPawnMovementState::GroundWalking)
	{
		CurrentSpeed = GroundMaxSpeed * 1.5f;
	}

	FVector TargetVelocity = FVector::ZeroVector;
	FVector ForwardDirection = GetActorForwardVector();
	FVector RightDirection = GetActorRightVector();
	FVector UpDirection = GetActorUpVector();

	if (!MoveInput.IsNearlyZero())
	{
		FVector Direction = (ForwardDirection * MoveInput.X) + (RightDirection * MoveInput.Y);
		Direction.Normalize();
		TargetVelocity += Direction * CurrentSpeed;
	}

	if (CurrentMovementState == EPawnMovementState::Flying && !FMath::IsNearlyZero(UpDownInput))
	{
		TargetVelocity += UpDirection * UpDownInput * CurrentSpeed;
	}

	CurrentVelocity = FMath::VInterpTo(CurrentVelocity, TargetVelocity, DeltaTime, 8.0f);
	FVector TotalOffset = CurrentVelocity * DeltaTime;

	if (CurrentMovementState == EPawnMovementState::NormalJumping)
	{
		VerticalVelocity += CustomGravity * DeltaTime;
		TotalOffset += FVector::UpVector * VerticalVelocity * DeltaTime;
		CurrentVelocity.Z = VerticalVelocity; 
	}
	else if (CurrentMovementState == EPawnMovementState::Flying)
	{
		CurrentVelocity.Z = TargetVelocity.Z;
	}

	AddActorWorldOffset(TotalOffset, true);
	
	FRotator ActorRotation = FRotator::ZeroRotator;
	ActorRotation.Yaw = LookInput.X * RotationSpeed * DeltaTime;
	
	if (CurrentMovementState == EPawnMovementState::Flying)
	{
		ActorRotation.Roll = RollInput * RotationSpeed * DeltaTime;
	}
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