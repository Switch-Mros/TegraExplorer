#include "mainmenu.h"
#include "../gfx/gfx.h"
#include "../gfx/gfxutils.h"
#include "../gfx/menu.h"
#include "tools.h"
#include "../hid/hid.h"
#include "../fs/menus/explorer.h"
#include <utils/btn.h>
#include <storage/nx_sd.h>
#include "tconf.h"
#include "../keys/keys.h"
#include "../storage/mountmanager.h"
#include "../storage/gptmenu.h"
#include "../storage/emummc.h"
#include <utils/util.h>
#include "../fs/fsutils.h"
#include <soc/fuse.h>
#include "../utils/utils.h"
#include "../config.h"
#include "../fs/readers/folderReader.h"
#include <string.h>
#include <mem/heap.h>
#include "../fs/menus/filemenu.h"

#define INCLUDE_BUILTIN_SCRIPTS 1
//#define SCRIPT_ONLY 1

#ifdef INCLUDE_BUILTIN_SCRIPTS
#include "../../build/TegraExplorer/script/builtin.h"
#endif

extern hekate_config h_cfg;

enum {
    #ifndef SCRIPT_ONLY
    MainExplore = 0,
    MainBrowseSd,
    MainMountSd,
    MainBrowseEmmc,
    MainBrowseEmummc,
    MainTools,
    MainPartitionSd,
    MainViewKeys,
    MainViewCredits,
    MainExit,
    #else 
    MainExit = 0,
    #endif
    MainPowerOff,
    MainRebootHekate,
    MainReloadTE,
    MainScripts,
};

MenuEntry_t mainMenuEntries[] = {
    #ifndef SCRIPT_ONLY
    [MainExplore] = {.optionUnion = COLORTORGB(COLOR_WHITE) | SKIPBIT, .name = "-- Explorer --"},
    [MainBrowseSd] = {.optionUnion = COLORTORGB(COLOR_GREEN), .name = "SD durchsuchen"},
    [MainMountSd] = {.optionUnion = COLORTORGB(COLOR_YELLOW)}, // To mount/unmount the SD
    [MainBrowseEmmc] = {.optionUnion = COLORTORGB(COLOR_BLUE), .name = "EMMC durchsuchen"},
    [MainBrowseEmummc] = {.optionUnion = COLORTORGB(COLOR_BLUE), .name = "EMUMMC durchsuchen"},
    [MainTools] = {.optionUnion = COLORTORGB(COLOR_WHITE) | SKIPBIT, .name = "\n-- Tools --"},
    [MainPartitionSd] = {.optionUnion = COLORTORGB(COLOR_ORANGE), .name = "SD-Karte partitionieren"},
    [MainViewKeys] = {.optionUnion = COLORTORGB(COLOR_YELLOW), .name = "Ausgelesene Keys anschauen"},
    [MainViewCredits] = {.optionUnion = COLORTORGB(COLOR_YELLOW), .name = "Ueber"},
    [MainExit] = {.optionUnion = COLORTORGB(COLOR_WHITE) | SKIPBIT, .name = "\n-- Beenden --"},
    #else 
    [MainExit] = {.optionUnion = COLORTORGB(COLOR_WHITE), .name = "\n-- Beenden --"},
    #endif
    [MainPowerOff] = {.optionUnion = COLORTORGB(COLOR_VIOLET), .name = "Ausschalten"},
    // [MainRebootAMS] = {.optionUnion = COLORTORGB(COLOR_VIOLET), .name = "Reboot to hekate"},
    // [MainRebootRCM] = {.optionUnion = COLORTORGB(COLOR_VIOLET), .name = "Reboot to RCM"},
    // [MainRebootNormal] = {.optionUnion = COLORTORGB(COLOR_VIOLET), .name = "Reboot normally"},
    [MainRebootHekate] = {.optionUnion = COLORTORGB(COLOR_VIOLET), .name = "Neustart in hekate"},
    [MainReloadTE] = {.optionUnion = COLORTORGB(COLOR_VIOLET), .name = "TegraExplorer neu laden"},
    [MainScripts] = {.optionUnion = COLORTORGB(COLOR_WHITE) | SKIPBIT, .name = "\n-- Scripte --"}
};

void HandleSD(){
    gfx_clearscreen();
    TConf.curExplorerLoc = LOC_SD;
    if (!sd_mount() || sd_get_card_removed()){
        gfx_printf("SD-Karte ist nicht gemounted!");
        hidWait();
    }
    else
        FileExplorer("sd:/");
}

void HandleEMMC(){
   GptMenu(MMC_CONN_EMMC);
}

void HandleEMUMMC(){
    GptMenu(MMC_CONN_EMUMMC);
}

void ViewKeys(){
    gfx_clearscreen();
    for (int i = 0; i < 3; i++){
        gfx_printf("\nBis key 0%d:   ", i);
        PrintKey(dumpedKeys.bis_key[i], AES_128_KEY_SIZE * 2);
    }
    
    gfx_printf("\nMaster key 0: ");
    PrintKey(dumpedKeys.master_key, AES_128_KEY_SIZE);
    gfx_printf("\nHeader key:   ");
    PrintKey(dumpedKeys.header_key, AES_128_KEY_SIZE * 2);
    gfx_printf("\nSichere mac key: ");
    PrintKey(dumpedKeys.save_mac_key, AES_128_KEY_SIZE);

    u8 fuseCount = 0;
    for (u32 i = 0; i < 32; i++){
        if ((fuse_read_odm(7) >> i) & 1)
            fuseCount++;
    }

    gfx_printf("\n\nPkg1 ID: '%s' (kb %d)\nFuse Zaehler: %d", TConf.pkg1ID, TConf.pkg1ver, fuseCount);

    hidWait();
}

void ViewCredits(){
    gfx_clearscreen();
    gfx_printf("\nTegraexplorer v%d.%d.%d.%d\nVon SuchMemeManySkill (Uebersetzt von Switch Bros.)\n\nBasierend auf Lockpick_RCM & Hekate, von shchmue & CTCaer\n\n\n", LP_VER_MJ, LP_VER_MN, LP_VER_BF, LP_VER_KEF);

    if (hidRead()->r)
        gfx_printf("%k\"Ich bin mir nicht mal sicher ob es funktioniert\" - meme", COLOR_ORANGE);

    hidWait();
}

extern bool sd_mounted;
extern bool is_sd_inited;
extern int launch_payload(char *path);

void RebootToAMS(){
    launch_payload("sd:/atmosphere/reboot_payload.bin");
}

void ReloadTE(){
    launch_payload("sd:/bootloader/payloads/TegraExplorer.bin");
}

void RebootToHekate(){
    launch_payload("sd:/bootloader/update.bin");
}

void MountOrUnmountSD(){
    gfx_clearscreen();
    if (sd_mounted)
        sd_unmount();
    else if (!sd_mount())
        hidWait();
}

menuPaths mainMenuPaths[] = {
    #ifndef SCRIPT_ONLY
    [MainBrowseSd] = HandleSD,
    [MainMountSd] = MountOrUnmountSD,
    [MainBrowseEmmc] = HandleEMMC,
    [MainBrowseEmummc] = HandleEMUMMC,
    [MainPartitionSd] = FormatSD,
    [MainViewKeys] = ViewKeys,
    [MainViewCredits] = ViewCredits,
    #endif
    [MainPowerOff] = power_off,
    [MainRebootHekate] = RebootToHekate,
    // [MainRebootAMS] = RebootToAMS,
    // [MainRebootRCM] = reboot_rcm,
    // [MainRebootNormal] = reboot_normal,
    [MainReloadTE] = ReloadTE
};

void EnterMainMenu(){
    int res = 0;
    while (1){
        if (sd_get_card_removed())
            sd_unmount();

        #ifndef SCRIPT_ONLY
        // -- Explore --
        mainMenuEntries[MainBrowseSd].hide = !sd_mounted;
        mainMenuEntries[MainMountSd].name = (sd_mounted) ? "Unmount SD" : "Mount SD";
        mainMenuEntries[MainBrowseEmummc].hide = (!emu_cfg.enabled || !sd_mounted);

        // -- Tools --
        mainMenuEntries[MainPartitionSd].hide = (!is_sd_inited || sd_get_card_removed());
        mainMenuEntries[MainViewKeys].hide = !TConf.keysDumped;

        // -- Exit --
        // mainMenuEntries[MainRebootAMS].hide = (!sd_mounted || !FileExists("sd:/atmosphere/reboot_payload.bin"));
        mainMenuEntries[MainRebootHekate].hide = (!sd_mounted || !FileExists("sd:/bootloader/update.bin"));
        // mainMenuEntries[MainRebootRCM].hide = h_cfg.t210b01;
        #endif
        // -- Scripts --
        #ifndef INCLUDE_BUILTIN_SCRIPTS
        mainMenuEntries[MainScripts].hide = (!sd_mounted || !FileExists("sd:/tegraexplorer/scripts"));
        #else
        mainMenuEntries[MainScripts].hide = ((!sd_mounted || !FileExists("sd:/tegraexplorer/scripts")) && !EMBEDDED_SCRIPTS_LEN);
        #endif

        Vector_t ent = newVec(sizeof(MenuEntry_t), ARRAY_SIZE(mainMenuEntries));
        ent.count = ARRAY_SIZE(mainMenuEntries);
        memcpy(ent.data, mainMenuEntries, sizeof(MenuEntry_t) * ARRAY_SIZE(mainMenuEntries));
        Vector_t scriptFiles = {0};
        u8 hasScripts = 0;

        #ifdef INCLUDE_BUILTIN_SCRIPTS
        for (int i = 0; i < EMBEDDED_SCRIPTS_LEN; i++){
            MenuEntry_t m = {.name = embedded_scripts_g[i].name, .optionUnion = COLORTORGB(COLOR_BLUE), .icon = 128};
            vecAdd(&ent, m);
        }
        #endif

        if (sd_mounted && FileExists("sd:/tegraexplorer/scripts")){
            scriptFiles = ReadFolder("sd:/tegraexplorer/scripts", &res);
            if (!res){
                if (!scriptFiles.count){
                    FREE(scriptFiles.data);
                    mainMenuEntries[MainScripts].hide = 1;
                }
                else {
                    hasScripts = 1;
                    vecForEach(FSEntry_t*, scriptFile, (&scriptFiles)){
                        if (!scriptFile->isDir && StrEndsWith(scriptFile->name, ".te")){
                            MenuEntry_t a = MakeMenuOutFSEntry(*scriptFile);
                            vecAdd(&ent, a);
                        }
                    }

                    if (ent.count == ARRAY_SIZE(mainMenuEntries)){
                        clearFileVector(&scriptFiles);
                        hasScripts = 0;
                        mainMenuEntries[MainScripts].hide = 1;
                    }
                }
            }
        }
        

        gfx_clearscreen();
        gfx_putc('\n');
        
        res = newMenu(&ent, res, 79, 30, (ent.count == ARRAY_SIZE(mainMenuEntries)) ? ALWAYSREDRAW : ALWAYSREDRAW | ENABLEPAGECOUNT, ent.count - ARRAY_SIZE(mainMenuEntries));
        if (res < MainScripts && mainMenuPaths[res] != NULL)
            mainMenuPaths[res]();
        #ifndef INCLUDE_BUILTIN_SCRIPTS
        else if (hasScripts){
        #else
        else {
            if (res - ARRAY_SIZE(mainMenuEntries) < EMBEDDED_SCRIPTS_LEN){
                char *script = embedded_scripts_g[res - ARRAY_SIZE(mainMenuEntries)].script;
                RunScriptString(script, strlen(script));
            }
            else {
            #endif
                vecDefArray(MenuEntry_t*, entArray, ent);
                MenuEntry_t entry = entArray[res];
                FSEntry_t fsEntry = {.name = entry.name, .sizeUnion = entry.sizeUnion};
                RunScript("sd:/tegraexplorer/scripts", fsEntry);
            #ifdef INCLUDE_BUILTIN_SCRIPTS
            }
            #endif
        }

        if (hasScripts){
            clearFileVector(&scriptFiles);
        }

        free(ent.data);
    }
}

