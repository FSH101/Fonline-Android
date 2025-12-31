# Android bring-up inventory

## Client entrypoint
- Desktop client starts in `Source/Applications/ClientApp.cpp` (`main` delegates to `InitApp` and repeatedly calls `MainEntry`).
- `MainEntry` wraps a frame: `App->BeginFrame()`, lazy resource sync + `FOClient` construction, then `FOClient::MainLoop()` and `App->EndFrame()`.
- SDL drives lifecycle differences: iOS/Web use callbacks, Android loops until `App->IsQuitRequested()`, desktop uses `FrameBalancer` to control FPS.

## Platform services needed
- Platform helpers in `Source/Essentials/Platform.cpp` back engine logging (`Platform::InfoLog`), thread naming (`SetThreadName`), executable path discovery (`GetExePath`), process forking (`ForkProcess`), dynamic library loading (`LoadModule`/`GetFuncAddr`/`UnloadModule`), and modifier state checks (`IsShiftDown`).
- Application setup (in `Source/Frontend/Application.cpp`) expects OS window management, timer/sleep from SDL, and file system access for settings/prebake paths.
- Event handling relies on SDL's input queue to translate mouse/keyboard (and eventually touch) into the engine's `InputEvent` list each frame.

## Rendering backend used
- Rendering type is chosen from `RenderType` in `Rendering.h`; defaults come from settings and capability checks in `Application.cpp` (Null, OpenGL, Direct3D, Metal, Vulkan).
- SDL window creation configures the swapchain: OpenGL paths set GL context attributes/ES profile, while Metal/Vulkan toggle corresponding window properties before `SDL_CreateWindowWithProperties`.
- Render loop uses `ActiveRenderer` to clear the backbuffer at `BeginFrame`, process ImGui/input, and present after drawing.

## Audio / video / extras
- Audio is streamed through SDL (`SDL_AudioStream` plus `AppAudio::AudioStreamCallback`) inside `Application.cpp`.
- Video playback is optional; Theora-backed `VideoClip` is guarded behind `FO_HAVE_THEORA` with stubs when disabled.
- Spark particle effects and ACM audio are feature-flagged; Android defaults keep them off until dependencies are available.

Expected outcome: Android bring-up progresses: native build compiles & links; APK launches; logs show engine init loop.
