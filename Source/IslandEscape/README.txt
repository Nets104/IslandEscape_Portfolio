Day4 Rain Guide modified files

Files:
- Public/GuideData.h
- Public/IslandEscapePlayerController.h
- Private/IslandEscapePlayerController.cpp
- Public/DayNightCycle.h
- Private/DayNightCycle.cpp

Editor setup:
1. In your PlayerController Blueprint, set UI|GameGuide > GameGuideWidgetClass to WBP_GameGuide.
2. In WBP_GameGuide, set GuideDataTable to DT_GuideData.
3. Add a new row to DT_GuideData:
   - RowName: Day4EveningRain
   - TriggerType: 4일차 저녁 강한 비
   - GuideText: 오늘 저녁부터 강한 비가 내립니다. 시야가 나빠질 수 있으니 이동과 전투에 주의하세요.
   - DisplayDuration: 4.0
   - bShowOnlyOnce: true
