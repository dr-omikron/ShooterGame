

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "BulletHitInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UBulletHitInterface : public UInterface
{
	GENERATED_BODY()
};
class SHOOTER_API IBulletHitInterface
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void BulletHit(FHitResult HitResult);
};
