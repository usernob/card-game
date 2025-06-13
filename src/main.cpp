#include "game.h"
#include "textrenderer.h"
#include "texturemanager.h"
#include "typedef.h"
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
#include <string>

struct AppContext
{
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    Game *game = nullptr;
    unsigned int last_tick = 0;
};

SDL_AppResult SDL_AppInit(void **appcontext, int argc, char *argv[])
{
    AppContext *const context = new AppContext();

    *appcontext = context;
    SDL_SetAppMetadata("Deck Builder Game", "1.0", "com.usernob.builder-deck");


    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer(
            "sdl-deck-builder",
            WINDOW_WIDTH,
            WINDOW_HEIGHT,
            0,
            &context->window,
            &context->renderer
        ))
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetWindowResizable(context->window, false);

    SDL_SetRenderLogicalPresentation(
        context->renderer,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_LOGICAL_PRESENTATION_INTEGER_SCALE
    );

    FontsManager::addFont(context->renderer, "font1-w", ASSETS_PATH "/fonts/ThaleahFat.fnt");
    FontsManager::addFont(context->renderer, "font2-w", ASSETS_PATH "/fonts/ThaleahFat2.fnt");
    FontsManager::addFont(context->renderer, "font3-w", ASSETS_PATH "/fonts/monogram.fnt");

    TextureManager::instance()->addAtlas(
        "base-card-atlas",
        context->renderer,
        ASSETS_PATH "/textures/atlas/base-card-atlas.png",
        ASSETS_PATH "/textures/atlas/base-card-atlas.json"
    );

    TextureManager::instance()->addAtlas(
        "tarot-card-atlas",
        context->renderer,
        ASSETS_PATH "/textures/atlas/tarot-card-atlas.png",
        ASSETS_PATH "/textures/atlas/tarot-card-atlas.json"
    );

    TextureManager::instance()->addAtlas(
        "ui-atlas",
        context->renderer,
        ASSETS_PATH "/textures/atlas/ui-atlas.png",
        ASSETS_PATH "/textures/atlas/ui-atlas.json"
    );

    // make sure the game is initialized after the window is created and the fonts are loaded
    context->game = new Game();
    context->last_tick = SDL_GetTicks();

    SDL_SetRenderVSync(context->renderer, 1);

    return SDL_APP_CONTINUE; /* carry on with the program! */
}

SDL_AppResult SDL_AppEvent(void *appcontext, SDL_Event *event)
{
    AppContext *const context = (AppContext *)appcontext;
    if (event->type == SDL_EVENT_QUIT)
    {
        return SDL_APP_SUCCESS;
    }

    context->game->registerMouseEvents(event);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appcontext)
{
    AppContext *const context = (AppContext *)appcontext;

    float delta = (SDL_GetTicks() - context->last_tick) / 1000.0f;
    context->last_tick = SDL_GetTicks();

    SDL_SetRenderDrawColor(context->renderer, 150, 134, 129, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(context->renderer);

    context->game->update(delta);
    context->game->render(context->renderer);

    if (context->game->exit())
    {
        return SDL_APP_SUCCESS;
    }

    SDL_RenderPresent(context->renderer); /* put it all on the screen! */

    return SDL_APP_CONTINUE; /* carry on with the program! */
}

void SDL_AppQuit(void *appcontext, SDL_AppResult result)
{
    AppContext *const context = (AppContext *)appcontext;
    SDL_DestroyRenderer(context->renderer);
    SDL_DestroyWindow(context->window);
    delete context->game;
    delete context;
    FontsManager::clear();
    TextureManager::instance()->clear();
    SDL_Quit();
}
