#include <filesystem>
#include <iostream>
#include <ranges>
#include <span>

extern "C"
{
#include <lauxlib.h>
}

#ifndef __cpp_lib_format
#define FMT_HEADER_ONLY
#include <fmt/format.h>
#endif

using namespace std;

bool check_ast_file(const char *pth)
{
    cout << format("checking file {} ...\t", pth);
    auto state = luaL_newstate();
    bool flag = true;
    do
    {
        if (luaL_dofile(state, pth))
        {
            cout << "ast file corrupt" << endl;
            flag = false;
            break;
        }
        lua_getglobal(state, "astver");
        auto x = lua_tonumber(state, -1);
        cout << format("astfile version={}", x) << endl;

    } while (false);
    lua_close(state);
    return flag;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        cerr << format("usage: {} [...args]",
                       filesystem::path(argv[0]).filename().string())
             << endl;
        return 1;
    }
    bool flag = true;
    for (auto arg : span(argv, argc) | views::drop(1))
    {
        if (filesystem::is_directory(arg))
        {
            cout << format("traversing directory {}", arg) << endl;
            for (auto &subfile :
                 filesystem::recursive_directory_iterator(arg) |
                     views::filter([](auto &n)
                                   { return n.path().extension() == ".ast"; }))
            {
                flag &= check_ast_file(subfile.path().string().data());
            }
        }
        else if (filesystem::is_regular_file(arg))
        {
            if (filesystem::path(arg).extension() == ".ast")
                flag &= check_ast_file(arg);
        }
        else
        {
            cerr << format("invalid path {}", arg) << endl;
        }
    }
    return !flag;
}