#include "tjsCommHead.h"

#include "Platform.h"

#include <SDL.h>

int TVPShowSimpleMessageBox(const ttstr& text, const ttstr& caption)
{
    std::vector<ttstr> normal;
    normal.emplace_back("OK");
    return TVPShowSimpleMessageBox(text, caption, normal);
}

int TVPShowSimpleMessageBox(const char* pszText,
                            const char* pszTitle,
                            unsigned int nButton,
                            const char** btnText)
{
    std::vector<ttstr> vecButtons;
    for (unsigned int i = 0; i < nButton; ++i)
    {
        vecButtons.emplace_back(btnText[i]);
    }
    return TVPShowSimpleMessageBox(pszText, pszTitle, vecButtons);
}

int TVPShowSimpleMessageBox(const ttstr& text,
                            const ttstr& caption,
                            const std::vector<ttstr>& vecButtons)
{
    std::vector<SDL_MessageBoxButtonData> sdlButtons;
    std::vector<std::string> btnTexts;
    sdlButtons.resize(vecButtons.size());
    btnTexts.resize(vecButtons.size());

    for (size_t i = 0; i < vecButtons.size(); ++i)
    {
        btnTexts[i] = vecButtons[i].AsStdString();
        SDL_MessageBoxButtonData btn;
        SDL_zero(btn);
        btn.buttonid = static_cast<int>(i);

        if (i == 0)
        {
            btn.flags |= SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT;
        }
        if (i == vecButtons.size() - 1)
        {
            btn.flags |= SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT;
        }

        btn.text = btnTexts[i].c_str();
        sdlButtons[i] = btn;
    }

    std::string titleStr = caption.AsStdString();
    std::string textStr = text.AsStdString();

    SDL_MessageBoxData msgboxData;
    SDL_zero(msgboxData);
    msgboxData.flags = SDL_MESSAGEBOX_INFORMATION;
    msgboxData.window = NULL;
    msgboxData.title = titleStr.c_str();
    msgboxData.message = textStr.c_str();
    msgboxData.numbuttons = static_cast<int>(vecButtons.size());
    msgboxData.buttons = vecButtons.empty() ? nullptr : sdlButtons.data();
    msgboxData.colorScheme = NULL;

    if (vecButtons.empty())
    {
        SDL_MessageBoxButtonData defaultButton;
        SDL_zero(defaultButton);
        defaultButton.flags =
            SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT | SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT;
        defaultButton.buttonid = 0;
        defaultButton.text = "\xe7\xa1\xae\xe5\xae\x9a"; // utf-8 "确定"
        msgboxData.buttons = &defaultButton;
        msgboxData.numbuttons = 1;
    }

    int buttonid = -1;
    if (SDL_ShowMessageBox(&msgboxData, &buttonid) < 0)
    {
        SDL_Log("SDL_ShowMessageBox failed: %s", SDL_GetError());
        return -1;
    }
    return buttonid;
}

std::string TVPShowFileSelector(const std::string& title,
                                const std::string& filename,
                                std::string initdir,
                                bool issave)
{
    SDL_Log("TVPShowFileSelector: not implemented in SDL2, using message box fallback");
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION,
                             title.empty() ? (issave ? "Save File" : "Select File") : title.c_str(),
                             "File dialog not available in SDL2 build.\nPlease use SDL3 for file dialog support.",
                             NULL);
    return "";
}

std::string TVPShowDirectorySelector(const std::string& title,
                                     std::string initdir,
                                     std::string rootdir)
{
    SDL_Log("TVPShowDirectorySelector: not implemented in SDL2, using message box fallback");
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION,
                             title.empty() ? "Select Folder" : title.c_str(),
                             "Folder dialog not available in SDL2 build.\nPlease use SDL3 for folder dialog support.",
                             NULL);
    return "";
}
