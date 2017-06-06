#include "protocol_ex.h"
#include "uuid_manager.h"

#include <engine/uuid.h>

static CUuidManager CreateGlobalUuidManager()
{
	CUuidManager Manager;
	RegisterUuids(&Manager);
	RegisterGameUuids(&Manager);
	return Manager;
}

CUuidManager g_UuidManager = CreateGlobalUuidManager();
