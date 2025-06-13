#ifndef SRC_TEXTUREMANAGER_H
#define SRC_TEXTUREMANAGER_H

#include "SDL3/SDL_surface.h"
#include "nlohmann/json_fwd.hpp"
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
#include <SDL3_image/SDL_image.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>

struct TextureInfo
{
    SDL_FRect rect;
    int offsetX;
    int offsetY;
    int untrimmedWidth;
    int untrimmedHeight;

    TextureInfo()
        : rect({0, 0, 0, 0}), offsetX(0), offsetY(0), untrimmedWidth(0), untrimmedHeight(0)
    {
    }

    TextureInfo(SDL_FRect rect, int offsetX, int offsetY, int untrimmedWidth, int untrimmedHeight)
        : rect(rect), offsetX(offsetX), offsetY(offsetY), untrimmedWidth(untrimmedWidth),
          untrimmedHeight(untrimmedHeight)
    {
    }
};

class TextureAtlas
{
private:
    SDL_Texture *m_atlas = nullptr;
    std::unordered_map<std::string, TextureInfo> m_textures;

public:
    TextureAtlas() : m_atlas(nullptr) {}

    ~TextureAtlas()
    {
        if (m_atlas)
        {
            SDL_DestroyTexture(m_atlas);
        }
        m_textures.clear();
    }

    bool loadAtlas(SDL_Renderer *renderer, const char *atlasPath, const char *jsonPath)
    {
        m_atlas = IMG_LoadTexture(renderer, atlasPath);
        if (m_atlas == nullptr)
        {
            SDL_LogError(
                SDL_LOG_CATEGORY_APPLICATION,
                "Failed to load atlas \"%s\": %s",
                atlasPath,
                SDL_GetError()
            );
            return false;
        }
        SDL_SetTextureScaleMode(m_atlas, SDL_SCALEMODE_PIXELART);
        return parseData(jsonPath);
    }

    std::string removeExtension(const std::string &filename)
    {
        size_t lastDot = filename.find_last_of('.');
        if (lastDot == std::string::npos) return filename; // Tidak ada ekstensi
        return filename.substr(0, lastDot);
    }

    bool parseData(const char *jsonPath)
    {
        std::ifstream jsonFile(jsonPath);
        if (!jsonFile.is_open())
        {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to open JSON file: %s", jsonPath);
            return false;
        }

        nlohmann::json jsonData;
        try
        {
            jsonData = nlohmann::json::parse(jsonFile);
        }
        catch (const nlohmann::json::exception &e)
        {
            SDL_LogError(
                SDL_LOG_CATEGORY_APPLICATION,
                "Failed to parse json \"%s\": %s",
                jsonPath,
                e.what()
            );
            return false;
        }

        if (jsonData.contains("Images"))
        {
            try
            {
                for (auto jsonInfo : jsonData["Images"])
                {
                    TextureInfo textureinfo;
                    textureinfo.rect.x = jsonInfo["X"];
                    textureinfo.rect.y = jsonInfo["Y"];
                    textureinfo.rect.w = jsonInfo["W"];
                    textureinfo.rect.h = jsonInfo["H"];
                    textureinfo.offsetX = jsonInfo["TrimOffsetX"];
                    textureinfo.offsetY = jsonInfo["TrimOffsetY"];
                    textureinfo.untrimmedWidth = jsonInfo["UntrimmedWidth"];
                    textureinfo.untrimmedHeight = jsonInfo["UntrimmedHeight"];

                    m_textures[removeExtension(jsonInfo["Name"])] = textureinfo;
                }
            }
            catch (const nlohmann::json::exception &e)
            {
                SDL_LogError(
                    SDL_LOG_CATEGORY_APPLICATION,
                    "Failed to get textures information: %s",
                    e.what()
                );
                return false;
            }
        }
        return true;
    }

    TextureInfo getTextureInfo(const std::string &textureName) const
    {
        auto it = m_textures.find(textureName);
        if (it != m_textures.end())
        {
            return it->second;
        }
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Texture \"%s\" not found", textureName.c_str());
        return TextureInfo();
    }

    SDL_Texture *getAtlas() const
    {
        return m_atlas;
    }
};

class TextureManager
{
private:
    std::unordered_map<std::string, TextureAtlas *> m_atlases;

public:
    static TextureManager *instance()
    {
        static TextureManager instance;
        return &instance;
    }

    TextureAtlas *getAtlas(const std::string &atlasName)
    {
        auto it = m_atlases.find(atlasName);
        if (it != m_atlases.end())
        {

            return it->second;
        }
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Atlas \"%s\" not found", atlasName.c_str());
        return nullptr;
    }

    void addAtlas(const std::string &atlasName, TextureAtlas *atlas)
    {
        m_atlases[atlasName] = atlas;
    }

    void addAtlas(
        const std::string &atlasName,
        SDL_Renderer *renderer,
        const char *atlasPath,
        const char *jsonPath
    )
    {
        TextureAtlas *atlas = new TextureAtlas();
        if (atlas->loadAtlas(renderer, atlasPath, jsonPath))
        {
            m_atlases[atlasName] = atlas;
        }
    }

    void clear()
    {
        for (auto atlas : m_atlases)
        {
            delete atlas.second;
        }
        m_atlases.clear();
    }
};

#endif // SRC_TEXTUREMANAGER_H
