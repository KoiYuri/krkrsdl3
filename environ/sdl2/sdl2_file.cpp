#include "tjsCommHead.h"

#include "Platform.h"
#include "PlatformFile.h"
#include "Random.h"

#include "TVPStorage.h"
#include "TVPMsg.h"

#include <SDL.h>

#include <string>
#include <vector>
#include <functional>
#include <sstream>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#endif

//---------------------------------------------------------------------------
static ttstr TVPLocalExtractFilePath(const ttstr& name)
{
    // this extracts given name's path under local filename rule
    const tjs_char* p = name.c_str();
    tjs_int i = name.GetLen() - 1;
    for (; i >= 0; i--)
    {
        if (p[i] == TJS_N(':') || p[i] == TJS_N('/') || p[i] == TJS_N('\\'))
            break;
    }
    return ttstr(p, i + 1);
}

static bool TVPWriteDataToFile(const ttstr& filepath, const void* data, unsigned int len)
{
    SDL_RWops* handle = SDL_RWFromFile(filepath.AsStdString().c_str(), "wb");
    if (!handle)
    {
        return false;
    }
    size_t written = SDL_RWwrite(handle, data, 1, len);
    SDL_RWclose(handle);
    return written == len;
}

static bool TVPCopyFolder(const std::string& from, const std::string& to)
{
    if (!TVPCheckExistentLocalFolder(to) && !TVPCreateFolders(to))
    {
        return false;
    }

    bool success = true;
    TVPListDir(from,
               [&](const std::string& _name, int mask)
               {
                   if (_name == "." || _name == "..")
                       return;
                   if (!success)
                       return;
                   if (mask & S_IFREG)
                   {
                       success = TVPCopyFile(from + "/" + _name, to + "/" + _name);
                   }
                   else if (mask & S_IFDIR)
                   {
                       success = TVPCopyFolder(from + "/" + _name, to + "/" + _name);
                   }
               });
    return success;
}

#ifdef _WIN32
static bool TVPRemoveDirectory(const std::string& path)
{
    std::string searchPath = path + "\\*";
    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        return RemoveDirectoryA(path.c_str()) != 0;
    }

    do
    {
        if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0)
            continue;

        std::string fullPath = path + "\\" + fd.cFileName;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if (!TVPRemoveDirectory(fullPath))
            {
                FindClose(hFind);
                return false;
            }
        }
        else
        {
            if (!DeleteFileA(fullPath.c_str()))
            {
                FindClose(hFind);
                return false;
            }
        }
    } while (FindNextFileA(hFind, &fd));

    FindClose(hFind);
    return RemoveDirectoryA(path.c_str()) != 0;
}
#else
static bool TVPRemoveDirectory(const std::string& path)
{
    DIR* dir = opendir(path.c_str());
    if (!dir)
    {
        return rmdir(path.c_str()) == 0;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        std::string fullPath = path + "/" + entry->d_name;
        struct stat st;
        if (stat(fullPath.c_str(), &st) == 0)
        {
            if (S_ISDIR(st.st_mode))
            {
                if (!TVPRemoveDirectory(fullPath))
                {
                    closedir(dir);
                    return false;
                }
            }
            else
            {
                if (unlink(fullPath.c_str()) != 0)
                {
                    closedir(dir);
                    return false;
                }
            }
        }
    }

    closedir(dir);
    return rmdir(path.c_str()) == 0;
}
#endif

//---------------------------------------------------------------------------
// tTVPLocalFileStream — defined internally, header only exposes factory
//---------------------------------------------------------------------------
class tTVPLocalFileStream : public tTJSBinaryStream
{
private:
    SDL_RWops* Handle;
    tTVPMemoryStream* MemBuffer = nullptr;
    ttstr FileName;

public:
    tTVPLocalFileStream(const ttstr& origname, const ttstr& localname, tjs_uint32 flag);
    ~tTVPLocalFileStream();

    tjs_uint64 Seek(tjs_int64 offset, tjs_int whence);
    tjs_uint Read(void* buffer, tjs_uint read_size);
    tjs_uint Write(const void* buffer, tjs_uint write_size);
    bool Flush();
    void SetEndOfStorage();
    tjs_uint64 GetSize();
    const std::string GetFileName() { return FileName.AsStdString(); }
    SDL_RWops* GetHandle() const { return Handle; }
};

tTJSBinaryStream* TVPCreateLocalFileStream(const ttstr& origname,
                                           const ttstr& localname,
                                           tjs_uint32 flag)
{
    return new tTVPLocalFileStream(origname, localname, flag);
}

//---------------------------------------------------------------------------
// tTVPLocalFileStream implementation
//---------------------------------------------------------------------------
tTVPLocalFileStream::tTVPLocalFileStream(const ttstr& origname,
                                         const ttstr& localname,
                                         tjs_uint32 flag)
  : FileName(localname),
    Handle(nullptr)
{
    tjs_uint32 access = flag & TJS_BS_ACCESS_MASK;
    if (access == TJS_BS_WRITE)
    {
        if (!TVPCheckExistentLocalFile(localname))
        {
            ttstr dirpath = TVPLocalExtractFilePath(localname);
            const tjs_char* p = dirpath.c_str();
            tjs_int i = dirpath.GetLen();
            if (p[i - 1] == TJS_N('/') || p[i - 1] == TJS_N('\\'))
                i--;
            dirpath = dirpath.SubString(0, i);
            if (!TVPCheckExistentLocalFolder(dirpath) && !TVPCreateFolders(dirpath))
            {
                TVPThrowExceptionMessage(TVPCannotOpenStorage, origname);
            }
        }
    }

    const char* mode = nullptr;
    switch (access)
    {
        case TJS_BS_READ:
            mode = "rb";
            break;
        case TJS_BS_WRITE:
            mode = "wb+";
            break;
        case TJS_BS_APPEND:
            mode = "ab+";
            break;
        case TJS_BS_UPDATE:
            mode = "rb+";
            break;
    }

    Handle = SDL_RWFromFile(localname.AsStdString().c_str(), mode);

    if (!Handle)
    {
        if (access == TJS_BS_APPEND || access == TJS_BS_UPDATE)
        {
            Handle = SDL_RWFromFile(localname.AsStdString().c_str(), "rb");
            if (Handle)
            {
                Sint64 size = SDL_RWsize(Handle);
                if (size > 0 && size < 4 * 1024 * 1024)
                {
                    MemBuffer = new tTVPMemoryStream();
                    MemBuffer->SetSize(static_cast<tjs_uint64>(size));
                    SDL_RWread(Handle, MemBuffer->GetInternalBuffer(), 1, size);
                }
                SDL_RWclose(Handle);
                Handle = nullptr;
            }
            if (!MemBuffer)
                TVPThrowExceptionMessage(TVPCannotOpenStorage, origname);
        }
    }

    // push current tick as an environment noise
    uint64_t tick = TVPGetRoughTickCount();
    TVPPushEnvironNoise(&tick, sizeof(tick));
}
//---------------------------------------------------------------------------
tTVPLocalFileStream::~tTVPLocalFileStream()
{
    if (MemBuffer)
    {
        if (!TVPWriteDataToFile(FileName, MemBuffer->GetInternalBuffer(), MemBuffer->GetSize()))
        {
            delete MemBuffer;
            ttstr filename(FileName);
            FileName.~tTJSString();
            free(this);
            TVPThrowExceptionMessage(TJS_N("File Writing Error: %1"), filename);
        }
        delete MemBuffer;
    }
    if (Handle)
    {
        SDL_RWclose(Handle);
    }

    // push current tick as an environment noise
    // (timing information from file accesses may be good noises)
    uint64_t tick = TVPGetRoughTickCount();
    TVPPushEnvironNoise(&tick, sizeof(tick));
}
//---------------------------------------------------------------------------
tjs_uint64 tTVPLocalFileStream::Seek(tjs_int64 offset, tjs_int whence)
{
    if (MemBuffer)
    {
        return MemBuffer->Seek(offset, whence);
    }
    return static_cast<tjs_uint64>(SDL_RWseek(Handle, offset, whence));
}
//---------------------------------------------------------------------------
tjs_uint tTVPLocalFileStream::Read(void* buffer, tjs_uint read_size)
{
    if (MemBuffer)
    {
        return MemBuffer->Read(buffer, read_size);
    }
    return static_cast<tjs_uint>(SDL_RWread(Handle, buffer, 1, read_size));
}
//---------------------------------------------------------------------------
tjs_uint tTVPLocalFileStream::Write(const void* buffer, tjs_uint write_size)
{
    if (MemBuffer)
    {
        return MemBuffer->Write(buffer, write_size);
    }
    return static_cast<tjs_uint>(SDL_RWwrite(Handle, buffer, 1, write_size));
}
//---------------------------------------------------------------------------
bool tTVPLocalFileStream::Flush()
{
    if (MemBuffer)
    {
        return MemBuffer->Flush();
    }
    // SDL2 RWops does not have a flush operation
    return true;
}
//---------------------------------------------------------------------------
void tTVPLocalFileStream::SetEndOfStorage()
{
    if (MemBuffer)
    {
        return MemBuffer->SetEndOfStorage();
    }

    SDL_RWseek(Handle, 0, SEEK_END);
}
//---------------------------------------------------------------------------
tjs_uint64 tTVPLocalFileStream::GetSize()
{
    if (MemBuffer)
    {
        return MemBuffer->GetSize();
    }
    return static_cast<tjs_uint64>(SDL_RWsize(Handle));
}
//---------------------------------------------------------------------------

std::string TVPGetDefaultFileDir()
{
    char* path = SDL_GetBasePath();
    if (!path)
    {
        return std::string();
    }
    std::string result(path);
    SDL_free(path);
    return result;
}

std::vector<std::string> TVPGetAppStoragePath()
{
    std::vector<std::string> ret;
    ret.emplace_back(TVPGetDefaultFileDir());
    return ret;
}

#ifdef _WIN32
bool TVPCheckExistentLocalFile(const ttstr& name)
{
    if (name.IsEmpty()) return false;
    DWORD attr = GetFileAttributesA(name.AsStdString().c_str());
    return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
}

bool TVPCheckExistentLocalFolder(const ttstr& name)
{
    if (name.IsEmpty()) return false;
    DWORD attr = GetFileAttributesA(name.AsStdString().c_str());
    return (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY));
}
#else
bool TVPCheckExistentLocalFile(const ttstr& name)
{
    if (name.IsEmpty()) return false;
    struct stat st;
    if (stat(name.AsStdString().c_str(), &st) != 0)
        return false;
    return S_ISREG(st.st_mode);
}

bool TVPCheckExistentLocalFolder(const ttstr& name)
{
    if (name.IsEmpty()) return false;
    struct stat st;
    if (stat(name.AsStdString().c_str(), &st) != 0)
        return false;
    return S_ISDIR(st.st_mode);
}
#endif

std::string TVPSearchPath(const std::string& filename, const std::string& searchpath)
{
    std::vector<std::string> paths;
    std::string pathStr =
        searchpath.empty() ? (SDL_getenv("PATH") ? SDL_getenv("PATH") : "") : searchpath;
    if (pathStr.empty())
        return "";

    std::stringstream ss(pathStr);
    std::string p;
    while (std::getline(ss, p, ';'))
    {
        if (!p.empty())
            paths.push_back(p);
    }

    for (const auto& p : paths)
    {
        std::string fullpath = p;
        if (!fullpath.empty() && fullpath.back() != '/' && fullpath.back() != '\\')
        {
#ifdef _KRKRSDL3_WINDOWS
            fullpath += '\\';
#else
            fullpath += '/';
#endif
        }
        fullpath += filename;

        if (TVPCheckExistentLocalFile(fullpath))
        {
            return fullpath;
        }
    }
    return "";
}

#ifdef _WIN32
bool TVPDeleteFile(const std::string& filename)
{
    return DeleteFileA(filename.c_str()) != 0;
}

bool TVPDeleteFolder(const std::string& foldername)
{
    return TVPRemoveDirectory(foldername);
}

bool TVPRenameFile(const std::string& from, const std::string& to)
{
    return MoveFileA(from.c_str(), to.c_str()) != 0;
}
#else
bool TVPDeleteFile(const std::string& filename)
{
    return unlink(filename.c_str()) == 0;
}

bool TVPDeleteFolder(const std::string& foldername)
{
    return TVPRemoveDirectory(foldername);
}

bool TVPRenameFile(const std::string& from, const std::string& to)
{
    return rename(from.c_str(), to.c_str()) == 0;
}
#endif

bool TVPCopyFile(const std::string& from, const std::string& to)
{
    SDL_RWops* fFrom = SDL_RWFromFile(from.c_str(), "rb");
    if (!fFrom)
    {
        return TVPCopyFolder(from, to);
    }
    SDL_RWops* fTo = SDL_RWFromFile(to.c_str(), "wb");
    if (!fTo)
    {
        SDL_RWclose(fFrom);
        return false;
    }
    const int bufSize = 1 * 1024 * 1024;
    std::vector<char> buffer(bufSize);
    size_t bytesRead;
    while ((bytesRead = SDL_RWread(fFrom, buffer.data(), 1, bufSize)) > 0)
    {
        if (SDL_RWwrite(fTo, buffer.data(), 1, bytesRead) != bytesRead)
        {
            SDL_RWclose(fFrom);
            SDL_RWclose(fTo);
            return false;
        }
    }

    SDL_RWclose(fFrom);
    SDL_RWclose(fTo);
    return true;
}

#ifdef _WIN32
void TVPListDir(const std::string& folder, std::function<void(const std::string&, int)> cb)
{
    std::string searchPath = folder;
    if (!searchPath.empty() && searchPath.back() != '\\')
        searchPath += '\\';
    searchPath += "*";

    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE)
        return;

    do
    {
        int mode = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? S_IFDIR : S_IFREG;
        cb(fd.cFileName, mode);
    } while (FindNextFileA(hFind, &fd));

    FindClose(hFind);
}

void TVPGetLocalFileListAt(const ttstr& name,
                           const std::function<void(const ttstr&, tTVPLocalFileInfo*)>& cb)
{
    std::string searchPath = name.AsStdString();
    if (!searchPath.empty() && searchPath.back() != '\\')
        searchPath += '\\';
    searchPath += "*";

    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE)
        return;

    do
    {
        tTVPLocalFileInfo info;
        info.NativeName = fd.cFileName;
        info.Mode = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? S_IFDIR : S_IFREG;
        info.Size = ((tjs_uint64)fd.nFileSizeHigh << 32) | fd.nFileSizeLow;

        auto fileTimeToTimeT = [](const FILETIME& ft) -> time_t
        {
            ULARGE_INTEGER ull;
            ull.LowPart = ft.dwLowDateTime;
            ull.HighPart = ft.dwHighDateTime;
            return (time_t)((ull.QuadPart / 10000000ULL) - 11644473600ULL);
        };

        info.AccessTime = fileTimeToTimeT(fd.ftLastAccessTime);
        info.ModifyTime = fileTimeToTimeT(fd.ftLastWriteTime);
        info.CreationTime = fileTimeToTimeT(fd.ftCreationTime);

        cb(fd.cFileName, &info);
    } while (FindNextFileA(hFind, &fd));

    FindClose(hFind);
}

bool TVPCreateFolders(const ttstr& folderttstr)
{
    std::string folder = folderttstr.AsStdString();
    if (folder.empty())
        return true;

    DWORD attr = GetFileAttributesA(folder.c_str());
    if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY))
        return true;

    size_t pos = folder.find_last_of("/\\");
    if (pos != std::string::npos && pos > 0)
    {
        std::string parent = folder.substr(0, pos);
        if (!(parent.size() == 2 && parent[1] == ':'))
        {
            if (!TVPCreateFolders(parent))
                return false;
        }
    }

    return CreateDirectoryA(folder.c_str(), NULL) != 0;
}
#else
void TVPListDir(const std::string& folder, std::function<void(const std::string&, int)> cb)
{
    DIR* dir = opendir(folder.c_str());
    if (!dir)
        return;

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL)
    {
        std::string fullPath = folder + "/" + entry->d_name;
        struct stat st;
        int mode = 0;
        if (stat(fullPath.c_str(), &st) == 0)
        {
            mode = S_ISDIR(st.st_mode) ? S_IFDIR : S_IFREG;
        }
        cb(entry->d_name, mode);
    }

    closedir(dir);
}

void TVPGetLocalFileListAt(const ttstr& name,
                           const std::function<void(const ttstr&, tTVPLocalFileInfo*)>& cb)
{
    std::string folder = name.AsStdString();
    DIR* dir = opendir(folder.c_str());
    if (!dir)
        return;

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL)
    {
        std::string fullPath = folder + "/" + entry->d_name;
        struct stat st;
        if (stat(fullPath.c_str(), &st) == 0)
        {
            tTVPLocalFileInfo info;
            info.NativeName = entry->d_name;
            info.Mode = S_ISDIR(st.st_mode) ? S_IFDIR : S_IFREG;
            info.Size = st.st_size;
            info.AccessTime = st.st_atime;
            info.ModifyTime = st.st_mtime;
            info.CreationTime = st.st_ctime;
            cb(entry->d_name, &info);
        }
    }

    closedir(dir);
}

bool TVPCreateFolders(const ttstr& folderttstr)
{
    std::string folder = folderttstr.AsStdString();
    if (folder.empty())
        return true;

    struct stat st;
    if (stat(folder.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
        return true;

    size_t pos = folder.find_last_of("/\\");
    if (pos != std::string::npos && pos > 0)
    {
        std::string parent = folder.substr(0, pos);
        if (!TVPCreateFolders(parent))
            return false;
    }

#ifdef _WIN32
    return mkdir(folder.c_str()) == 0;
#else
    return mkdir(folder.c_str(), 0755) == 0;
#endif
}
#endif
