#pragma once
#include "CoreMinimal.h"

DECLARE_LOG_CATEGORY_EXTERN(LogIslandEscape, Log, All);

#define ECC_SoftCollision ECC_GameTraceChannel1
#define ECC_Tiger ECC_GameTraceChannel2
#define ECC_Enemy ECC_GameTraceChannel3
#define ECC_WorldItem ECC_GameTraceChannel4

namespace IslandEscapeUIZOrder
{
	constexpr int32 GameplayHUD = 7000;
	constexpr int32 PlayerNotice = 8000;
	constexpr int32 GameGuide = 8500;
	constexpr int32 Dialogue = 8600;
	constexpr int32 InteractionPanelBase = 9000;
	constexpr int32 Modal = 9500;
	constexpr int32 PauseMenu = 10000;
	constexpr int32 SettingsModal = 10500;
	constexpr int32 EndScreen = 15000;
	constexpr int32 FadeScreen = 19000;
	constexpr int32 KeyGuide = 20000;
}

