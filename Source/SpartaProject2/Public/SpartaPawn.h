#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "SpartaPawn.generated.h"

class UCapsuleComponent;
class USkeletalMeshComponent;
class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;

// ⭐ 캐릭터의 상태를 정의하는 열거형 추가
UENUM(BlueprintType)
enum class EPawnMovementState : uint8
{
	GroundWalking   UMETA(DisplayName = "Ground Walking"),
	NormalJumping   UMETA(DisplayName = "Normal Jumping"),
	Flying          UMETA(DisplayName = "Flying")
};

UCLASS()
class SPARTAPROJECT2_API ASpartaPawn : public APawn
{
	GENERATED_BODY()

public:
	ASpartaPawn();

protected:
	virtual void BeginPlay() override;

	// 컴포넌트 리스트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCapsuleComponent* CapsuleComp;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* MeshComp;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USpringArmComponent* SpringArmComp;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCameraComponent* CameraComp;

	// Enhanced Input
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* InputMappingContext;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* MoveAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* LookAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* UpDownAction; 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* RollAction;   

	// 이동 제어 세팅 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float GroundMaxSpeed;       
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float AirControlRatio;      
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float RotationSpeed;        

	// 중력 및 지면 감지 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Gravity")
	float CustomGravity;        
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Gravity")
	float TraceDistance;        

	// ─── [내부 상태 연산용 변수] ───
	FVector2D MoveInput;
	FVector2D LookInput;
	float UpDownInput;
	float RollInput;

	UPROPERTY(BlueprintReadOnly, Category = "Movement|Internal")
	FVector CurrentVelocity;     

	// ⭐ 기존 bool 스위치 대신 열거형 상태 변수로 대체관리
	UPROPERTY(BlueprintReadOnly, Category = "Movement|Internal")
	EPawnMovementState CurrentMovementState;

	UPROPERTY(BlueprintReadOnly, Category = "Movement|Internal")
	bool bIsSprinting;           

	float VerticalVelocity;      

	// ⭐ 더블 점프(비행 전환) 감지용 타이밍 변수들
	float LastJumpTime;          // 마지막으로 스페이스바를 누른 절대 시간
	float DoubleTapThreshold;    // 더블 탭 인정 시간 범위 (예: 0.25초)

	// 입력 처리 함수
	void OnMove(const FInputActionValue& Value);
	void OnLook(const FInputActionValue& Value);
	void OnJumpStarted(const FInputActionValue& Value);
	void OnUpDownMovement(const FInputActionValue& Value);
	void OnRoll(const FInputActionValue& Value);   

public:	
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual FVector GetVelocity() const override;
};