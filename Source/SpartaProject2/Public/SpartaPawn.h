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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCapsuleComponent* CapsuleComp;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* MeshComp;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USpringArmComponent* SpringArmComp;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCameraComponent* CameraComp;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float GroundMaxSpeed;       
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float AirControlRatio;      
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float RotationSpeed;        

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Gravity")
	float CustomGravity;        
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Gravity")
	float TraceDistance;        

	FVector2D MoveInput;
	FVector2D LookInput;
	float UpDownInput;
	float RollInput;

	UPROPERTY(BlueprintReadOnly, Category = "Movement|Internal")
	FVector CurrentVelocity;     

	UPROPERTY(BlueprintReadOnly, Category = "Movement|Internal")
	EPawnMovementState CurrentMovementState;

	UPROPERTY(BlueprintReadOnly, Category = "Movement|Internal")
	bool bIsSprinting;           

	float VerticalVelocity;      

	float LastJumpTime;
	float DoubleTapThreshold;

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