#include "ncbind.hpp"
#include <set>

//---------------------------------------------------------------------------
// static変数の実体

// auto register 先頭ポインタ
ncbAutoRegister::ThisClassT const* ncbAutoRegister::_top[ncbAutoRegister::LINE_COUNT] =
    NCB_INNER_AUTOREGISTER_LINES_INSTANCE;

std::set<ttstr> TVPRegisteredPlugins;

std::map<ttstr, ncbAutoRegister::INTERNAL_PLUGIN_LISTS> ncbAutoRegister::_internal_plugins;

bool ncbAutoRegister::LoadModule(const ttstr& _name)
{
    if (_internal_plugins.size() == 0)
    {
        for (int line = 0; line < LINE_COUNT; line++)
        {
            for (ThisClassT const* p = _top[line]; p; p = p->_next)
            {
                ttstr name = p->modulename;
                name.ToLowerCase();
                _internal_plugins[name].lists[line].push_back(p);
            }
        }        
    }

    ttstr name = _name.AsLowerCase();
    if (TVPRegisteredPlugins.find(name) != TVPRegisteredPlugins.end())
        return false;
    auto it = _internal_plugins.find(name);
    if (it != _internal_plugins.end())
    {
        for (int line = 0; line < ncbAutoRegister::LINE_COUNT; ++line)
        {
            const std::list<ncbAutoRegister const*>& plugin_list = it->second.lists[line];
            for (auto i = plugin_list.begin(); i != plugin_list.end(); ++i)
            {
                (*i)->Regist();
            }
        }
        TVPRegisteredPlugins.insert(name);
        return true;
    }
    return false;
}

bool ncbAutoRegister::UnloadModule(const ttstr& _name)
{
    ttstr name = _name.AsLowerCase();
    auto it = TVPRegisteredPlugins.find(name);
    if (it == TVPRegisteredPlugins.end())
        return false;

    auto it2 = _internal_plugins.find(name);
    if (it2 != _internal_plugins.end())
    {
        for (int line = ncbAutoRegister::LINE_COUNT - 1; line >= 0; --line)
        {
            const std::list<ncbAutoRegister const*>& plugin_list = it2->second.lists[line];
            for (auto i = plugin_list.rbegin(); i != plugin_list.rend(); ++i)
            {
                (*i)->Unregist();
            }
        }
    }

    TVPRegisteredPlugins.erase(it);
    return true;
}

void ncbAutoRegister::ClearRegisteredModule()
{
    std::vector<ttstr> loadedPlugins(TVPRegisteredPlugins.begin(), TVPRegisteredPlugins.end());
    for (auto it = loadedPlugins.rbegin(); it != loadedPlugins.rend(); ++it)
    {
        ncbAutoRegister::UnloadModule(*it);
    }
    _internal_plugins.clear();
}
