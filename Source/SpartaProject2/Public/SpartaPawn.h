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

UCLASS()
class SPARTAPROJECT2_API ASpartaPawn : public APawn
{
	GENERATED_BODY()

public:
	ASpartaPawn();

protected:
	virtual void BeginPlay() override;

	// ─── [컴포넌트 리스트] ───
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCapsuleComponent* CapsuleComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* MeshComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USpringArmComponent* SpringArmComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCameraComponent* CameraComp;

	// ─── [Enhanced Input 에셋 매핑 변수] ───
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* InputMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* UpDownAction; // ⭐ 도전: 상하 고도 조절 및 지상 점프/스프린트 통합 액션

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* RollAction;   // ⭐ 도전: 6자유도 Roll 회전 액션

	// ─── [이동 및 회전 제어 변수] ───
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float GroundMaxSpeed;       // 지상 기본 최고 속도

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float AirControlRatio;      // ⭐ 도전: 공중 상태 이동 속도 제한 비율 (0.3 ~ 0.5)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float RotationSpeed;        // 마우스 및 키보드 회전 속도

	// ─── [중력 및 지면 감지 변수] ───
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Gravity")
	float CustomGravity;        // ⭐ 도전: 인공 중력 가속도 (-980.0f)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Gravity")
	float TraceDistance;        // ⭐ 도전: LineTrace 지면 충돌 감지 레이저 거리

	// ─── [내부 상태 연산용 변수 (매크로 추가 완료)] ───
	UPROPERTY(BlueprintReadOnly, Category = "Movement|Internal")
	FVector2D MoveInput;

	UPROPERTY(BlueprintReadOnly, Category = "Movement|Internal")
	FVector2D LookInput;

	UPROPERTY(BlueprintReadOnly, Category = "Movement|Internal")
	float UpDownInput;

	UPROPERTY(BlueprintReadOnly, Category = "Movement|Internal")
	float RollInput;

	UPROPERTY(BlueprintReadOnly, Category = "Movement|Internal")
	FVector CurrentVelocity;     // ⭐ ABP가 실시간으로 읽어갈 실제 물리 속도 벡터

	UPROPERTY(BlueprintReadOnly, Category = "Movement|Internal")
	bool bIsSprinting;           // 지상 달리기 작동 여부 스위치

	UPROPERTY(BlueprintReadOnly, Category = "Movement|Internal")
	bool bIsPawnFalling;         // ⭐ 지상/공중 로직 분기를 위한 핵심 상태 스위치

	UPROPERTY(BlueprintReadOnly, Category = "Movement|Internal")
	float VerticalVelocity;      // Z축 전용 실시간 하방 가속도 데이터

	// ─── [입력 바인딩 처리 함수] ───
	void OnMove(const FInputActionValue& Value);
	void OnLook(const FInputActionValue& Value);
	void OnUpDown(const FInputActionValue& Value); // ⭐ 추가
	void OnRoll(const FInputActionValue& Value);   // ⭐ 추가

public:	
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// ⭐ 애니메이션 블루프린트(ABP)가 GetVelocity 노드를 쓸 때 프레임 속도를 반환하는 오버라이드 함수
	virtual FVector GetVelocity() const override;
};