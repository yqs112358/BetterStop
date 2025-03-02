#include <llapi/mc/Player.hpp>
#include <llapi/mc/Level.hpp>
#include <llapi/EventAPI.h>
#include <string>
#include <vector>
#include <Windows.h>
#include <stdlib.h>
#include "SimpleIni.h"
#include "version.h"
using namespace std;

#define _CONF_PATH "plugins/BetterStop/config.ini"

CSimpleIniA ini;
bool isStopping = false;
bool isServerStarted = false;

extern Logger logger;

//Utils
vector<Player*> Raw_GetOnlinePlayers()
{
    vector<Player*> pls;
    if (isServerStarted)
        pls = Level::getAllPlayers();

    return pls;
}

// Main
void SafeStop()
{
    if (isStopping)
        return;
    isStopping = true;

    logger.info("Safe Stop processing...");
    auto players = Raw_GetOnlinePlayers();
    for (auto& pl : players)
    {
        pl->kick(string(ini.GetValue("Main", "StopMsg", "Server stopping...")));
    }
    Level::runcmd("stop");
}

// ===== onConsoleCmd =====
THook(bool, "??$inner_enqueue@$0A@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@?$SPSCQueue@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@$0CAA@@@AEAA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z",
    void* _this, std::string &cmd)
{
    if (cmd.front() == '/')
        cmd = cmd.substr(1);
    if (!cmd.empty() && cmd == "stop" && !isStopping && isServerStarted)
    {
        SafeStop();
        return false;
    }
    return original(_this, cmd);
}

//Message
BOOL CtrlHandler(DWORD fdwCtrlType)
{
    switch (fdwCtrlType)
    {
    case CTRL_C_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        logger.info("Stop detected.");
        SafeStop();
        return TRUE;

    case CTRL_BREAK_EVENT:
    case CTRL_LOGOFF_EVENT:
        return FALSE;

    default:
        return FALSE;
    }
}

//Main
void PluginInit()
{
    if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE))
        logger.info("Fail to enable Console Close Protection!");

    HWND hwnd = GetConsoleWindow();
    HMENU hmenu = GetSystemMenu(hwnd, false);
    RemoveMenu(hmenu, SC_CLOSE, MF_BYCOMMAND);
    DestroyMenu(hmenu);
    ReleaseDC(hwnd, NULL);

    Event::ServerStartedEvent::subscribe([](Event::ServerStartedEvent ev)
    {
        isServerStarted = true;
        return true;
    });

    ini.SetUnicode(true);
    auto res = ini.LoadFile(_CONF_PATH);
    logger.info("BetterStop 关服保护插件-已装载  当前版本：{}.{}.{}",
        PLUGIN_VERSION_MAJOR, PLUGIN_VERSION_MINOR, PLUGIN_VERSION_REVISION);
}