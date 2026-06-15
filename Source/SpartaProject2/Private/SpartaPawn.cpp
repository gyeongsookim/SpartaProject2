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

	// 초기 설정치
	GroundMaxSpeed = 500.0f;
	AirControlRatio = 0.4f;
	RotationSpeed = 50.0f;
	CustomGravity = -1500.0f;     // 일반 점프 낙하의 무게감을 위해 -1500cm/s² 권장
	TraceDistance = 99.0f;	

	// 상태 초기화
	CurrentMovementState = EPawnMovementState::GroundWalking;
	bIsSprinting = false;
	VerticalVelocity = 0.0f;
	UpDownInput = 0.0f;
	RollInput = 0.0f;
	CurrentVelocity = FVector::ZeroVector;

	// 더블 탭 타이밍 설정
	LastJumpTime = -1.0f;
	DoubleTapThreshold = 0.25f; // 0.25초 이내에 연타해야 비행 발동

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

		// Space / Shift 입력 처리
		// ─── 기존 OnUpDown 바인딩 3줄을 아래와 같이 교체하세요 ───
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

// ⭐ 상태 머신 기반으로 분리된 지상 점프/스프린트 및 더블 탭 비행 전환 함수
void ASpartaPawn::OnJumpStarted(const FInputActionValue& Value)
{
	float InputVal = Value.Get<float>();

	// 위쪽 방향(스페이스바) 입력일 때만 작동
	if (InputVal > 0.0f)
	{
		float CurrentTime = GetWorld()->GetTimeSeconds();

		if (CurrentMovementState == EPawnMovementState::GroundWalking)
		{
			// [지상일 때] -> 정석 일반 점프 실행
			CurrentMovementState = EPawnMovementState::NormalJumping;
			VerticalVelocity = 550.0f; 
			LastJumpTime = CurrentTime; // 첫 누른 시간 기록
		}
		else if (CurrentMovementState == EPawnMovementState::NormalJumping)
		{
			// [일반 점프/추락 중일 때 연타] -> 비행 모드(Flying) 전환!
			if (CurrentTime - LastJumpTime <= DoubleTapThreshold)
			{
				CurrentMovementState = EPawnMovementState::Flying;
				VerticalVelocity = 0.0f; // 비행 진입 시 낙하 속도 리셋
			}
			LastJumpTime = CurrentTime; // 시간 갱신
		}
		else if (CurrentMovementState == EPawnMovementState::Flying)
		{
			// ⭐ [비행 중일 때 연타] -> 비행 모드 해제하고 일반 추락 상태로 전환!
			if (CurrentTime - LastJumpTime <= DoubleTapThreshold)
			{
				CurrentMovementState = EPawnMovementState::NormalJumping;
				VerticalVelocity = 0.0f; // 부드러운 추락 시작을 위해 수직 속도 리셋
			}
			LastJumpTime = CurrentTime; // 비행 중에도 누른 시간을 계속 기록해야 더블 탭을 감지할 수 있습니다.
		}
	}
}

// 2️⃣ 쉬프트(Sprint) 및 지속 입력 제어 (Triggered, Completed 전용)
void ASpartaPawn::OnUpDownMovement(const FInputActionValue& Value)
{
	float InputVal = Value.Get<float>();

	// 키를 떼면 Completed 상태가 되어 InputVal이 0에 가깝게 들어옵니다.
	if (FMath::IsNearlyZero(InputVal))
	{
		bIsSprinting = false;
		UpDownInput = 0.0f;
		return;
	}

	// 지속적으로 키를 누르고 있는 상태 (Triggered)
	if (CurrentMovementState == EPawnMovementState::GroundWalking && InputVal < 0.0f)
	{
		bIsSprinting = true; // 지상에서 쉬프트는 대시
	}
	else if (CurrentMovementState == EPawnMovementState::Flying)
	{
		UpDownInput = InputVal; // 비행 중에는 상하 고도 조절치로 작동
		bIsSprinting = false;
	}
}

void ASpartaPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// ─── [1단계: 지면 충돌 감지 (LineTrace)] ───
	FHitResult HitResult;
	FVector StartLocation = GetActorLocation();
	FVector EndLocation = StartLocation + (FVector::DownVector * TraceDistance);
	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(this);

	bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_Visibility, TraceParams);
	
	// 땅에 닿았을 때 상태 강제 리셋 (비행 중이라도 땅에 착륙하면 지상 모드로 복귀)
	if (bHit)
	{
		// 떨어지는 중이거나 비행 중에 지면에 닿았다면 착지 처리
		if (VerticalVelocity <= 0.0f && CurrentMovementState != EPawnMovementState::GroundWalking)
		{
			CurrentMovementState = EPawnMovementState::GroundWalking;
			VerticalVelocity = 0.0f;
			UpDownInput = 0.0f;
		
			// ⭐ [해결 2] 착지하는 순간 이전 속도 잔상을 칼같이 제로로 클리어! (즉시 Idle 전환)
			CurrentVelocity = FVector::ZeroVector; 
		
			// ⭐ [해결 1] 발이 파고들지 않도록 오프셋을 +88.0f에서 +98.0f로 상향 보정!
			FVector NewLoc = GetActorLocation();
			NewLoc.Z = HitResult.ImpactPoint.Z + 98.0f; 
			SetActorLocation(NewLoc);
		}
	}
	else if (!bHit && CurrentMovementState == EPawnMovementState::GroundWalking)
	{
		// 걷다가 절벽에서 떨어지면 일반 추락 상태로 전환
		CurrentMovementState = EPawnMovementState::NormalJumping;
	}

	// ─── [2단계: 상태별 이동 속도 공식 제어] ───
	float CurrentSpeed = GroundMaxSpeed;
	
	if (CurrentMovementState == EPawnMovementState::NormalJumping)
	{
		CurrentSpeed = GroundMaxSpeed * AirControlRatio; // 일반 점프 중에는 공중 에어컨트롤 속도 제한 (40%)
	}
	else if (bIsSprinting && CurrentMovementState == EPawnMovementState::GroundWalking)
	{
		CurrentSpeed = GroundMaxSpeed * 1.5f; // 지상 스프린트 대시 시 속도 우대
	}

	FVector TargetVelocity = FVector::ZeroVector;
	FVector ForwardDirection = GetActorForwardVector();
	FVector RightDirection = GetActorRightVector();
	FVector UpDirection = GetActorUpVector();

	// 수평 이동(WASD) 벡터 연산
	if (!MoveInput.IsNearlyZero())
	{
		FVector Direction = (ForwardDirection * MoveInput.X) + (RightDirection * MoveInput.Y);
		Direction.Normalize();
		TargetVelocity += Direction * CurrentSpeed;
	}

	// ⭐ 오직 비행 모드(Flying)일 때만 상하 6자유도 고도 조절 허용
	if (CurrentMovementState == EPawnMovementState::Flying && !FMath::IsNearlyZero(UpDownInput))
	{
		TargetVelocity += UpDirection * UpDownInput * CurrentSpeed;
	}

	// 속도 인터폴레이션 보간(가감속 슬라이딩 효과)
	CurrentVelocity = FMath::VInterpTo(CurrentVelocity, TargetVelocity, DeltaTime, 8.0f);
	FVector TotalOffset = CurrentVelocity * DeltaTime;

	// ─── [3단계: 인공 중력 분기 처리] ───
	// ⭐ 오직 지상이 아니거나 비행 모드가 아닐 때(즉, 순수 점프/추락 중일 때만) 중력 적용!
	if (CurrentMovementState == EPawnMovementState::NormalJumping)
	{
		VerticalVelocity += CustomGravity * DeltaTime;
		TotalOffset += FVector::UpVector * VerticalVelocity * DeltaTime;
		CurrentVelocity.Z = VerticalVelocity; 
	}
	else if (CurrentMovementState == EPawnMovementState::Flying)
	{
		// 비행 중에는 중력이 잡아당기지 않고 수직 축 속도를 그대로 애니메이션에 넘김
		CurrentVelocity.Z = TargetVelocity.Z;
	}

	// 최종 트랜스폼 이동 적용
	AddActorWorldOffset(TotalOffset, true);
	
	// ─── [4단계: 6자유도 및 시점 회전 연산] ───
	FRotator ActorRotation = FRotator::ZeroRotator;
	ActorRotation.Yaw = LookInput.X * RotationSpeed * DeltaTime;
	
	// ⭐ 오직 비행 모드일 때만 공중 Roll(몸통 기울임) 회전 허용
	if (CurrentMovementState == EPawnMovementState::Flying)
	{
		ActorRotation.Roll = RollInput * RotationSpeed * DeltaTime;
	}
	AddActorLocalRotation(ActorRotation);

	// 마우스 Y(Pitch) 3D 멀미 방지용 스프링암 클램프 회전
	if (!FMath::IsNearlyZero(LookInput.Y))
	{
		float PitchRotation = -LookInput.Y * RotationSpeed * DeltaTime;
		FRotator CurrentRelativeRot = SpringArmComp->GetRelativeRotation();
		float ClampedPitch = FMath::Clamp(CurrentRelativeRot.Pitch + PitchRotation, -60.0f, 60.0f);
		SpringArmComp->SetRelativeRotation(FRotator(ClampedPitch, 0.0f, 0.0f));
	}

	// 버퍼 청소
	LookInput = FVector2D::ZeroVector;
	MoveInput = FVector2D::ZeroVector; 
}

FVector ASpartaPawn::GetVelocity() const
{
	return CurrentVelocity;
}