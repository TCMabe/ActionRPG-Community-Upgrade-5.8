#pragma once

#include "Commandlets/Commandlet.h"
#include "RPGInspectContentCommandlet.generated.h"

/** Read-only export of sample authoring data and Blueprint graph connections. */
UCLASS()
class URPGInspectContentCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    URPGInspectContentCommandlet();
    virtual int32 Main(const FString& Params) override;
};
