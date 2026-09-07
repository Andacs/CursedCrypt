#include "CCPlayerCharacter.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/LocalPlayer.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/SpringArmComponent.h"
#include "AttributeComponent.h"
#include "TimerManager.h"

ACCPlayerCharacter::ACCPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 350.f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	Attributes = CreateDefaultSubobject<UAttributeComponent>(TEXT("Attributes"));
}

void ACCPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (ULocalPlayer* LP = PC->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				if (DefaultMappingContext)
				{
					Subsystem->AddMappingContext(DefaultMappingContext, 0);
				}
			}
		}
	}
}

void ACCPlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Stamina regen is server-authoritative; result is replicated to clients.
	if (HasAuthority() && Attributes)
	{
		Attributes->RestoreStamina(this, StaminaRegenRate * DeltaTime);
	}
}

void ACCPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EIC->BindAction(IA_Move, ETriggerEvent::Triggered, this, &ACCPlayerCharacter::Move);
		EIC->BindAction(IA_Look, ETriggerEvent::Triggered, this, &ACCPlayerCharacter::Look);
		EIC->BindAction(IA_Jump, ETriggerEvent::Started, this, &ACharacter::Jump);
		EIC->BindAction(IA_Jump, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		EIC->BindAction(IA_Attack, ETriggerEvent::Started, this, &ACCPlayerCharacter::Attack);
	}
}

void ACCPlayerCharacter::Move(const FInputActionValue& Value)
{
	FVector2D AxisValue = Value.Get<FVector2D>();
	if (Controller)
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, AxisValue.X);
		AddMovementInput(RightDirection, AxisValue.Y);
	}
}

void ACCPlayerCharacter::Look(const FInputActionValue& Value)
{
	FVector2D AxisValue = Value.Get<FVector2D>();
	AddControllerYawInput(AxisValue.X);
	AddControllerPitchInput(AxisValue.Y);
}

void ACCPlayerCharacter::Attack()
{
	Server_Attack();
}

void ACCPlayerCharacter::Server_Attack_Implementation()
{
	// Attack lock guard.
	if (bIsAttacking) return;
	if (!Attributes || !Attributes->IsAlive()) return;

	if (Attributes->ConsumeStamina(this, AttackStaminaCost))
	{
		bIsAttacking = true;

		// Reset the melee hit list once per attack (player path).
		// Server_Attack is the single entry point so this avoids the blend-bug
		// seen when resetting from inside the animation notify.
		Attributes->ResetMeleeHitList();

		Multicast_PlayAttackAnim();

		const float AnimDuration = AttackMontage ? AttackMontage->GetPlayLength() : 1.0f;
		GetWorldTimerManager().SetTimer(TimerHandle_AttackLock, this, &ACCPlayerCharacter::ResetAttackLock, AnimDuration, false);
	}
}

void ACCPlayerCharacter::Multicast_PlayAttackAnim_Implementation()
{
	if (AttackMontage && GetMesh())
	{
		if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
		{
			AnimInstance->Montage_Play(AttackMontage);
		}
	}
}

float ACCPlayerCharacter::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (HasAuthority() && Attributes && ActualDamage > 0.0f)
	{
		Attributes->ApplyDamage(DamageCauser, ActualDamage);
	}
	return ActualDamage;
}