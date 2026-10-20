# SA Events Reference (GTASA only)

All in `plugin::Events`. Source: `shared/Events.h` lines 347–421.

## Usage
```cpp
Events::gameProcessEvent += [] { /* ... */ };
Events::pedCtorEvent += [](CPed* p) { /* ... */ };
Events::drawingEvent.before += [] { /* before */ };
Events::drawingEvent.after += [] { /* after */ };
// Remove:
Events::gameProcessEvent -= myHandler;
// Alternative syntax (for non-lambda):
Events::initRwEvent.Add(MyFunc);
```

## Hook addresses (SA 1.0 US Compact)

| Event | Hook addr | Hook type | Priority | Signature |
|---|---|---|---|---|
| `restartGameEvent` | `0x53C6DB` | H_CALL | AFTER | `void()` |
| `renderMirrorEvent` | `0x53EABA` | H_CALL | AFTER | `void()` |
| `renderSceneEvent` | `0x53EABF` | H_CALL | AFTER | `void()` |
| `renderEffectsEvent` | `0x53E222` | H_CALL | AFTER | `void()` |
| `drawingEvent` | `0x53E293` | H_CALL | AFTER | `void()` |
| `drawHudEvent` | `0x53E4FF` | H_CALL | AFTER | `void()` |
| `drawAfterFadeEvent` | `0x53E550`, `0x53E83C`, `0x53EBA2` | H_CALL | AFTER | `void()` |
| `drawFontsEvent` | `0x53EBB1` | H_CALL | AFTER | `void()` |
| `drawRadarEvent` | `0x58FC53` | H_CALL | AFTER | `void()` |
| `drawBlipsEvent` | `0x58AA2D` H_JUMP, `0x575B44` H_CALL | AFTER | `void()` |
| `drawRadarOverlayEvent` | `0x5759E4`, `0x5869BF` | H_CALL | AFTER | `void(bool)` |
| `drawMenuBackgroundEvent` | `0x57C324` | H_JUMP | AFTER | `void(void*)` |
| `initRwEvent` | `0x5BF3A1` | H_CALL | AFTER | `void()` |
| `shutdownRwEvent` | `0x53D910` | H_CALL | BEFORE | `void()` |
| `vehicleCtorEvent` | `0x6D5F2F` | H_CALL | AFTER | `void(CVehicle*)` |
| `vehicleDtorEvent` | `0x6E2D35` | H_CALL | BEFORE | `void(CVehicle*)` |
| `pedCtorEvent` | `0x5E8052` | H_CALL | AFTER | `void(CPed*)` |
| `pedDtorEvent` | `0x5E880C` | H_CALL | BEFORE | `void(CPed*)` |
| `objectCtorEvent` | `0x5A1D2D`, `0x5A1D8D`, `0x5A1E0E`, `0x5A1FA3`, `0x5A2018` | H_CALL | AFTER | `void(CObject*)` |
| `objectDtorEvent` | `0x59F82A` | H_CALL | BEFORE | `void(CObject*)` |
| `initPoolsEvent` | `0x5BF85B` | H_CALL | AFTER | `void()` |
| `shutdownPoolsEvent` | `0x53CA5C` | H_CALL | AFTER | `void()` |
| `vehicleRenderEvent` | `0x6C4523` H_CALL, `0x6D0E89` H_JUMP | BEFORE | `void(CVehicle*)` |
| `pedRenderEvent` | `0x5E77FC`, `0x5E780A` | H_CALL | BEFORE | `void(CPed*)` |
| `objectRenderEvent` | `0x59F1ED` | H_JUMP | BEFORE | `void(CObject*)` |
| `vehicleSetModelEvent` | `0x6D6A49` | H_CALL | AFTER | `void(CVehicle*, int)` |
| `pedSetModelEvent` | `0x5E4890` | H_CALL | AFTER | `void(CPed*, int)` |
| `d3dResetEvent` | 13 addrs (0x7F79E1 etc.) | H_CALL | AFTER | `void()` |
| `d3dLostEvent` | 13 addrs (0x7F795B etc.) | H_CALL | AFTER | `void()` |
| `gameProcessEvent` | `0x53E981` | H_CALL | AFTER | `void()` |
| `initGameEvent` | `0x748CFB` | H_CALL | AFTER | `void()` |
| `reInitGameEvent` | `0x748E09`, `0x748E48` | H_CALL | AFTER | `void()` |
| `onPauseAllSounds` | `0x747FA5` | H_CALL | BEFORE | `void(void*)` |
| `onResumeAllSounds` | `0x747FE4` | H_CALL | BEFORE | `void(void*)` |
| `initScriptsEvent` | `0x53BDD7`, `0x5BA340`, `0x5D4FD7` | H_CALL | AFTER | `void()` |
| `processScriptsEvent` | `0x53C077` | H_CALL | AFTER | `void()` |
| `menuDrawingEvent` | `0x53E82D`, `0x53EB8C` | H_CALL | AFTER | `void(CMenuManager*)` |
| `attachRwPluginsEvent` | `0x53ECA1` | H_CALL | AFTER | `bool()` |
