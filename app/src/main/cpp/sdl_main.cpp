#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_render.h>

extern "C" bool AndroidClientInit();
extern "C" bool AndroidClientFrame();
extern "C" void AndroidClientShutdown();

static bool InitEngine()
{
    SDL_Log("[FONLINE] InitApp: starting AndroidClientInit");
    const bool ok = AndroidClientInit();
    SDL_Log("[FONLINE] InitApp result: %d", static_cast<int>(ok));
    return ok;
}

int SDL_main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    SDL_SetMainReady();

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER) != 0) {
        SDL_Log("[FONLINE] SDL_Init failed: %s", SDL_GetError());
        return -1;
    }

    SDL_Log("[FONLINE] SDL_Init OK");

    SDL_Window* window = SDL_CreateWindow("FOnline", 0, 0,
        SDL_WINDOW_FULLSCREEN | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window) {
        SDL_Log("[FONLINE] SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        return -2;
    }

    SDL_Log("[FONLINE] SDL window created");

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);
    if (!renderer) {
        SDL_Log("[FONLINE] SDL_CreateRenderer failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -3;
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);

    const bool engineReady = InitEngine();
    bool running = engineReady;
    int frame = 0;

    while (running) {
        SDL_Event evt;
        while (SDL_PollEvent(&evt)) {
            if (evt.type == SDL_EVENT_QUIT || evt.type == SDL_EVENT_TERMINATING || evt.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
                SDL_Log("[FONLINE] Received quit event");
                running = false;
            }
        }

        if (running && engineReady) {
            SDL_Log("[FONLINE] Frame %d: Begin", frame);
            running = AndroidClientFrame();
        }

        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);

        if (engineReady) {
            SDL_Log("[FONLINE] Frame %d: End", frame);
        }

        frame++;
    }

    if (engineReady) {
        SDL_Log("[FONLINE] Shutdown requested");
        AndroidClientShutdown();
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
