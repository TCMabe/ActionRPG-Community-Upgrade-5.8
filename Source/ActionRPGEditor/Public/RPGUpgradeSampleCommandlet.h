#pragma once

#include "Commandlets/Commandlet.h"
#include "RPGUpgradeSampleCommandlet.generated.h"

/** Applies the narrowly scoped, repeatable asset repairs required by the 5.8 port. */
UCLASS()
class URPGUpgradeSampleCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    URPGUpgradeSampleCommandlet();
    virtual int32 Main(const FString& Params) override;
};
