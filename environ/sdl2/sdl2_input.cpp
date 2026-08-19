#include "tjsCommHead.h"

#include <SDL_ttf.h>
#include <SDL.h>

bool TVPInputQuery(const std::string& title, const std::string& prompt, std::string& value)
{
    const int WIDTH = 400;
    const int HEIGHT = 200;

    static bool ttf_inited = false;
    if (!ttf_inited)
    {
        if (TTF_Init() < 0)
        {
            SDL_Log("fontInit error: %s", SDL_GetError());
            return false;
        }
        ttf_inited = true;
    }

    TTF_Font* font = TTF_OpenFont("simhei.ttf", 16);
    if (!font)
    {
#ifdef _WIN32
        font = TTF_OpenFont("C:/Windows/Fonts/simhei.ttf", 16);
#elif __APPLE__
        font = TTF_OpenFont("/System/Library/Fonts/Helvetica.ttc", 16);
#else
        font = TTF_OpenFont("/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf", 16);
#endif

        if (!font)
        {
            return false;
        }
    }

    SDL_Window* window = SDL_CreateWindow(title.c_str(),
                                          SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED,
                                          WIDTH, HEIGHT, 0);
    if (!window)
    {
        TTF_CloseFont(font);
        return false;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer)
    {
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
        if (!renderer)
        {
            TTF_CloseFont(font);
            SDL_DestroyWindow(window);
            return false;
        }
    }

    std::string inputText = value;
    bool running = true;
    bool result = false;
    size_t cursorPos = inputText.length();
    bool showCursor = true;
    Uint64 cursorBlinkTime = SDL_GetTicks();
    const Uint64 cursorBlinkInterval = 500;

    SDL_Rect okRect = {WIDTH / 2 - 100, HEIGHT - 60, 90, 40};
    SDL_Rect cancelRect = {WIDTH / 2 + 10, HEIGHT - 60, 90, 40};

    SDL_Rect inputRect = {50, 100, WIDTH - 100, 40};

    SDL_StartTextInput();

    while (running)
    {
        Uint64 currentTime = SDL_GetTicks();
        if (currentTime - cursorBlinkTime > cursorBlinkInterval)
        {
            showCursor = !showCursor;
            cursorBlinkTime = currentTime;
        }

        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
            {
                running = false;
            }
            else if (e.type == SDL_KEYDOWN)
            {
                switch (e.key.keysym.sym)
                {
                    case SDLK_RETURN:
                    case SDLK_KP_ENTER:
                        result = true;
                        running = false;
                        break;

                    case SDLK_ESCAPE:
                        result = false;
                        running = false;
                        break;

                    case SDLK_BACKSPACE:
                        if (cursorPos > 0 && !inputText.empty())
                        {
                            size_t pos = cursorPos - 1;
                            while (pos > 0 && (inputText[pos] & 0xC0) == 0x80)
                            {
                                pos--;
                            }

                            unsigned char c = inputText[pos];
                            int charLen = 1;
                            if ((c & 0xF8) == 0xF0)
                                charLen = 4;
                            else if ((c & 0xF0) == 0xE0)
                                charLen = 3;
                            else if ((c & 0xE0) == 0xC0)
                                charLen = 2;

                            if (pos + charLen <= inputText.length())
                            {
                                inputText.erase(pos, charLen);
                                cursorPos = pos;
                                showCursor = true;
                                cursorBlinkTime = currentTime;
                            }
                        }
                        break;

                    case SDLK_DELETE:
                        if (cursorPos < inputText.length())
                        {
                            size_t pos = cursorPos - 1;
                            while (pos > 0 && (inputText[pos] & 0xC0) == 0x80)
                            {
                                pos--;
                            }

                            unsigned char c = inputText[cursorPos];
                            int charLen = 1;
                            if ((c & 0xF8) == 0xF0)
                                charLen = 4;
                            else if ((c & 0xF0) == 0xE0)
                                charLen = 3;
                            else if ((c & 0xE0) == 0xC0)
                                charLen = 2;

                            if (cursorPos + charLen <= inputText.length())
                            {
                                inputText.erase(cursorPos, charLen);
                                showCursor = true;
                                cursorBlinkTime = currentTime;
                            }
                        }
                        break;

                    case SDLK_LEFT:
                        if (cursorPos > 0)
                        {
                            cursorPos--;
                            while (cursorPos > 0 && (inputText[cursorPos] & 0xC0) == 0x80)
                            {
                                cursorPos--;
                            }
                            showCursor = true;
                            cursorBlinkTime = currentTime;
                        }
                        break;

                    case SDLK_RIGHT:
                        if (cursorPos < inputText.length())
                        {
                            cursorPos++;
                            while (cursorPos < inputText.length() && (inputText[cursorPos] & 0xC0) == 0x80)
                            {
                                cursorPos++;
                            }
                            showCursor = true;
                            cursorBlinkTime = currentTime;
                        }
                        break;

                    case SDLK_HOME:
                        cursorPos = 0;
                        showCursor = true;
                        cursorBlinkTime = currentTime;
                        break;

                    case SDLK_END:
                        cursorPos = inputText.length();
                        showCursor = true;
                        cursorBlinkTime = currentTime;
                        break;

                    case SDLK_TAB:
                        break;
                }
            }
            else if (e.type == SDL_TEXTINPUT)
            {
                inputText.insert(cursorPos, e.text.text);
                cursorPos += strlen(e.text.text);
                showCursor = true;
                cursorBlinkTime = currentTime;
            }
            else if (e.type == SDL_MOUSEBUTTONDOWN)
            {
                int x = e.button.x;
                int y = e.button.y;

                if (x >= okRect.x && x < okRect.x + okRect.w && y >= okRect.y &&
                    y < okRect.y + okRect.h)
                {
                    result = true;
                    running = false;
                }
                else if (x >= cancelRect.x && x < cancelRect.x + cancelRect.w &&
                         y >= cancelRect.y && y < cancelRect.y + cancelRect.h)
                {
                    result = false;
                    running = false;
                }

                else if (x >= inputRect.x && x < inputRect.x + inputRect.w && y >= inputRect.y &&
                          y < inputRect.y + inputRect.h)
                {
                    if (font)
                    {
                        int clickX = x - inputRect.x - 5;
                        std::string textBeforeCursor;
                        int textWidth = 0;

                        for (size_t i = 0; i <= inputText.length(); i++)
                        {
                            textBeforeCursor = inputText.substr(0, i);
                            SDL_Surface* surface =
                                TTF_RenderText_Blended(font, textBeforeCursor.c_str(),
                                                       {0, 0, 0, 255});
                            if (surface)
                            {
                                int w = surface->w;
                                SDL_FreeSurface(surface);

                                if (clickX <= w)
                                {
                                    cursorPos = i;
                                    break;
                                }
                            }
                        }
                    }
                    showCursor = true;
                    cursorBlinkTime = currentTime;
                }
            }
        }

        // Render
        SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255);
        SDL_RenderClear(renderer);

        SDL_Color black = {0, 0, 0, 255};
        SDL_Surface* promptSurface =
            TTF_RenderText_Blended(font, prompt.c_str(), black);
        if (promptSurface)
        {
            SDL_Texture* promptTexture = SDL_CreateTextureFromSurface(renderer, promptSurface);
            if (promptTexture)
            {
                SDL_Rect promptRect = {20, 20, promptSurface->w, promptSurface->h};
                SDL_RenderCopy(renderer, promptTexture, NULL, &promptRect);
                SDL_DestroyTexture(promptTexture);
            }
            SDL_FreeSurface(promptSurface);
        }

        SDL_Rect inputRectDraw = {20, 60, WIDTH - 40, 40};
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderFillRect(renderer, &inputRectDraw);
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        SDL_RenderDrawRect(renderer, &inputRectDraw);

        if (!inputText.empty())
        {
            SDL_Surface* textSurface =
                TTF_RenderText_Blended(font, inputText.c_str(), black);
            if (textSurface)
            {
                SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
                if (textTexture)
                {
                    SDL_Rect textRect = {inputRectDraw.x + 5, inputRectDraw.y + 10,
                                         SDL_min(textSurface->w, inputRectDraw.w - 10),
                                         textSurface->h};
                    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);

                    if (cursorPos <= inputText.length())
                    {
                        std::string beforeCursor = inputText.substr(0, cursorPos);
                        SDL_Surface* cursorSurface = TTF_RenderText_Blended(
                            font, beforeCursor.c_str(), black);
                        if (cursorSurface && showCursor)
                        {
                            int cursorX = inputRectDraw.x + 5 + cursorSurface->w;
                            SDL_Rect cursorRect = {cursorX, inputRectDraw.y + 5, 2,
                                                   inputRectDraw.h - 10};
                            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                            SDL_RenderFillRect(renderer, &cursorRect);
                            SDL_FreeSurface(cursorSurface);
                        }
                        else if (cursorSurface)
                        {
                            SDL_FreeSurface(cursorSurface);
                        }
                    }

                    SDL_DestroyTexture(textTexture);
                }
                SDL_FreeSurface(textSurface);
            }
        }
        else
        {
            if (showCursor)
            {
                SDL_Rect cursorRect = {inputRectDraw.x + 5, inputRectDraw.y + 5, 2, inputRectDraw.h - 10};
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                SDL_RenderFillRect(renderer, &cursorRect);
            }
        }

        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);

        bool okHover = (mouseX >= okRect.x && mouseX < okRect.x + okRect.w && mouseY >= okRect.y &&
                        mouseY < okRect.y + okRect.h);
        bool cancelHover = (mouseX >= cancelRect.x && mouseX < cancelRect.x + cancelRect.w &&
                            mouseY >= cancelRect.y && mouseY < cancelRect.y + cancelRect.h);

        SDL_SetRenderDrawColor(renderer, okHover ? 120 : 100, okHover ? 220 : 200,
                               okHover ? 120 : 100, 255);
        SDL_RenderFillRect(renderer, &okRect);

        SDL_SetRenderDrawColor(renderer, cancelHover ? 220 : 200, cancelHover ? 120 : 100,
                               cancelHover ? 120 : 100, 255);
        SDL_RenderFillRect(renderer, &cancelRect);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderDrawRect(renderer, &okRect);
        SDL_RenderDrawRect(renderer, &cancelRect);

        SDL_Surface* okSurface = TTF_RenderText_Blended(font, "OK", black);
        if (okSurface)
        {
            SDL_Texture* okTexture = SDL_CreateTextureFromSurface(renderer, okSurface);
            if (okTexture)
            {
                SDL_Rect okTextRect = {okRect.x + okRect.w / 2 - okSurface->w / 2,
                                        okRect.y + okRect.h / 2 - okSurface->h / 2,
                                        okSurface->w, okSurface->h};
                SDL_RenderCopy(renderer, okTexture, NULL, &okTextRect);
                SDL_DestroyTexture(okTexture);
            }
            SDL_FreeSurface(okSurface);
        }

        SDL_Surface* cancelSurface = TTF_RenderText_Blended(font, "Cancel", black);
        if (cancelSurface)
        {
            SDL_Texture* cancelTexture = SDL_CreateTextureFromSurface(renderer, cancelSurface);
            if (cancelTexture)
            {
                SDL_Rect cancelTextRect = {cancelRect.x + cancelRect.w / 2 - cancelSurface->w / 2,
                                            cancelRect.y + cancelRect.h / 2 - cancelSurface->h / 2,
                                            cancelSurface->w, cancelSurface->h};
                SDL_RenderCopy(renderer, cancelTexture, NULL, &cancelTextRect);
                SDL_DestroyTexture(cancelTexture);
            }
            SDL_FreeSurface(cancelSurface);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    SDL_StopTextInput();

    if (result)
    {
        value = inputText;
    }

    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    return result;
}

int TVPShowSimpleInputBox(ttstr& text,
                          const ttstr& caption,
                          const ttstr& prompt,
                          const std::vector<ttstr>& btns)
{
    std::string inputStr = text.AsStdString();
    bool res = TVPInputQuery(caption.AsStdString(), prompt.AsStdString(), inputStr);
    text = inputStr;
    if (res)
        return 0;
    else
        return 1;
}
