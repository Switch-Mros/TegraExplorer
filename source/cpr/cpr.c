// #include <utils/util.h>
// #include "tools.h"
#include <storage/nx_sd.h>
#include "../fs/readers/folderReader.h"
#include "../fs/fstypes.h"
#include "../fs/fscopy.h"
#include <utils/sprintf.h>
#include <stdlib.h>
#include <string.h>
#include "../gfx/gfx.h"
#include "../gfx/gfxutils.h"
#include "../gfx/menu.h"
#include "../hid/hid.h"
// #include "utils.h"
#include "../utils/utils.h"
#include "../fs/fsutils.h"

#include <unistd.h>
#include <sys/types.h>
// #include <dirent.h>
// #include <stdio.h>
#include <string.h>
#include <sys/stat.h>

void _DeleteFileSimple(char *thing)
{
    //char *thing = CombinePaths(path, entry.name);
    int res = f_unlink(thing);
    if (res)
        DrawError(newErrCode(res));
    free(thing);
}
void _RenameFileSimple(char *sourcePath, char *destPath)
{
    int res = f_rename(sourcePath, destPath);
    if (res)
    {
        DrawError(newErrCode(res));
    }
}
ErrCode_t _FolderDelete(const char *path)
{
    int res = 0;
    ErrCode_t ret = newErrCode(0);
    u32 x, y;
    gfx_con_getpos(&x, &y);
    Vector_t fileVec = ReadFolder(path, &res);
    if (res)
    {
        ret = newErrCode(res);
    }
    else
    {
        vecDefArray(FSEntry_t *, fs, fileVec);
        for (int i = 0; i < fileVec.count && !ret.err; i++)
        {
            char *temp = CombinePaths(path, fs[i].name);
            if (fs[i].isDir)
            {
                ret = _FolderDelete(temp);
            }
            else
            {
                res = f_unlink(temp);
                if (res)
                {
                    ret = newErrCode(res);
                }
            }
            free(temp);
        }
    }
    if (!ret.err)
    {
        res = f_unlink(path);
        if (res)
            ret = newErrCode(res);
    }
    clearFileVector(&fileVec);
    return ret;
}
int _StartsWith(const char *a, const char *b)
{
    if (strncmp(a, b, strlen(b)) == 0)
        return 1;
    return 0;
}

int listdir(char *path, u32 hos_folder)
{
    FRESULT res;
    DIR dir;
    u32 dirLength = 0;
    static FILINFO fno;

    u32 x, y;
    gfx_con_getpos(&x, &y);

    // Open directory.
    res = f_opendir(&dir, path);
    if (res != FR_OK)
        return res;

    dirLength = strlen(path);
    for (;;)
    {
        // Clear file or folder path.
        path[dirLength] = 0;
        // Read a directory item.
        res = f_readdir(&dir, &fno);
        // Break on error or end of dir.
        if (res != FR_OK || fno.fname[0] == 0)
            break;
        // Skip official Nintendo dir if started from root.
        if (!hos_folder && !strcmp(fno.fname, "Nintendo"))
            continue;

        // // Set new directory or file.
        memcpy(&path[dirLength], "/", 1);
        memcpy(&path[dirLength + 1], fno.fname, strlen(fno.fname) + 1);
        // gfx_printf("THING: %s\n", fno.fname);
        // gfx_printf("Pfad: %s\n", dir);
        // Is it a directory?
        if (fno.fattrib & AM_DIR)
        {
            if (
                strcmp(fno.fname, ".Trash") == 0 ||
                strcmp(fno.fname, ".Trashes") == 0 ||
                strcmp(fno.fname, ".DS_Store") == 0 ||
                strcmp(fno.fname, ".Spotlight-V100") == 0 ||
                strcmp(fno.fname, ".apDisk") == 0 ||
                strcmp(fno.fname, ".VolumeIcon.icns") == 0 ||
                strcmp(fno.fname, ".fseventsd") == 0 ||
                strcmp(fno.fname, ".TemporaryItems") == 0)
            {
                gfx_puts_limit(path, (YLEFT - x) / 16 - 10);
                BoxRestOfScreen();

                gfx_con_setpos(x, y);
                _FolderDelete(path);
            }

            // Enter the directory.
            listdir(path, 0);
            if (res != FR_OK)
                break;
        }
        else
        {
            if (
                strcmp(fno.fname, ".DS_Store") == 0 ||
                strcmp(fno.fname, ".Spotlight-V100") == 0 ||
                strcmp(fno.fname, ".apDisk") == 0 ||
                strcmp(fno.fname, ".VolumeIcon.icns") == 0 ||
                strcmp(fno.fname, ".fseventsd") == 0 ||
                strcmp(fno.fname, ".TemporaryItems") == 0 ||
                _StartsWith(fno.fname, "._"))
            {
                gfx_puts_limit(path, (YLEFT - x) / 16 - 10);
                BoxRestOfScreen();

                gfx_con_setpos(x, y);
                _DeleteFileSimple(path);
            }
        }
    }
    f_closedir(&dir);
    free(path);
    return res;
}

int _fix_attributes(char *path, u32 *total, u32 hos_folder, u32 check_first_run)
{
    FRESULT res;
    DIR dir;
    u32 dirLength = 0;
    static FILINFO fno;

    u32 x, y;
    gfx_con_getpos(&x, &y);

    if (check_first_run)
    {
        // Read file attributes.
        res = f_stat(path, &fno);
        if (res != FR_OK)
            return res;

        // Check if archive bit is set.
        if (fno.fattrib & AM_ARC)
        {
            *(u32 *)total = *(u32 *)total + 1;
            f_chmod(path, 0, AM_ARC);
        }
    }

    // Open directory.
    res = f_opendir(&dir, path);
    if (res != FR_OK)
        return res;

    dirLength = strlen(path);
    for (;;)
    {
        // Clear file or folder path.
        path[dirLength] = 0;

        // Read a directory item.
        res = f_readdir(&dir, &fno);

        // Break on error or end of dir.
        if (res != FR_OK || fno.fname[0] == 0)
            break;

        // Skip official Nintendo dir if started from root.
        if (!hos_folder && (!strcmp(fno.fname, "Nintendo") || !strcmp(fno.fname, "roms") || !strcmp(fno.fname, "contents") || !strcmp(fno.fname, "erpt_reports") || !strcmp(fno.fname, "fatal_reports") || !strcmp(fno.fname, "fatal_errors") || !strcmp(fno.fname, "crash_reports")))
            continue;

        // Set new directory or file.
        memcpy(&path[dirLength], "/", 1);
        memcpy(&path[dirLength + 1], fno.fname, strlen(fno.fname) + 1);

        // Check if archive bit is set.
        if (fno.fattrib & AM_ARC)
        {
            *total = *total + 1;
            // gfx_printf("%s\n", path);
            gfx_puts_limit(path, (YLEFT - x) / 16 - 10);
            BoxRestOfScreen();

            gfx_con_setpos(x, y);
            // gfx_clearscreen();
            f_chmod(path, 0, AM_ARC);
        }

        // Is it a directory?
        if (fno.fattrib & AM_DIR)
        {
            // Set archive bit to NCA folders.
            if (hos_folder && !strcmp(fno.fname + strlen(fno.fname) - 4, ".nca"))
            {
                *total = *total + 1;
                // gfx_printf("%s\n", path);
                gfx_puts_limit(path, (YLEFT - x) / 16 - 10);
                BoxRestOfScreen();

                gfx_con_setpos(x, y);
                // gfx_clearscreen();
                f_chmod(path, AM_ARC, AM_ARC);
            }

            // Enter the directory.
            res = _fix_attributes(path, total, hos_folder, 0);
            if (res != FR_OK)
                break;
        }
    }

    f_closedir(&dir);

    return res;
}

void m_entry_fixArchiveBit(u32 type)
{

    char path[256];
    char label[16];

    u32 total = 0;
    if (sd_mount())
    {
        // strcpy(path, "/");
        // strcpy(label, "SD-Karte");
        // gfx_printf("Durchlaufe \"%s\"..\nDas kann einige Zeit dauern...\n\n", label);
        // _fix_attributes(path, &total, type, type);

        strcpy(path, "atmosphere");
        strcpy(label, "atmosphere");
        gfx_printf("Attribute reparieren. Das kann einige Zeit dauern...", label);
        gfx_printf("\nDurchlaufe \"%k%s%k\"..\n\n", 0xFF96FF00, label, 0xFFCCCCCC);
        _fix_attributes(path, &total, type, type);
        gfx_clearscreen();
        total = total + total;

        strcpy(path, "bootloader");
        strcpy(label, "bootloader");
        gfx_printf("\nDurchlaufe \"%k%s%k\"..\n\n", 0xFF96FF00, label, 0xFFCCCCCC);
        _fix_attributes(path, &total, type, type);
        gfx_clearscreen();
        total = total + total;

        strcpy(path, "switch");
        strcpy(label, "switch");
        gfx_printf("\nDurchlaufe \"%k%s%k\"..\n\n", 0xFF96FF00, label, 0xFFCCCCCC);
        _fix_attributes(path, &total, type, type);
        gfx_clearscreen();
        total = total + total;

        strcpy(path, "config");
        strcpy(label, "config");
        gfx_printf("\nDurchlaufe \"%k%s%k\"..\n\n", 0xFF96FF00, label, 0xFFCCCCCC);
        _fix_attributes(path, &total, type, type);
        gfx_clearscreen();
        total = total + total;

        strcpy(path, "firmware");
        strcpy(label, "firmware");
        gfx_printf("\nDurchlaufe \"%k%s%k\"..\n\n", 0xFF96FF00, label, 0xFFCCCCCC);
        _fix_attributes(path, &total, type, type);
        gfx_clearscreen();
        total = total + total;

        strcpy(path, "emummc");
        strcpy(label, "emummc");
        gfx_printf("\nDurchlaufe \"%k%s%k\"..\n\n", 0xFF96FF00, label, 0xFFCCCCCC);
        _fix_attributes(path, &total, type, type);
        gfx_clearscreen();
        total = total + total;
    }

    gfx_clearscreen();
    gfx_printf("%kAnzahl geloeschter Archiv Bits: %d!%k", 0xFF96FF00, total, 0xFFCCCCCC);
}

void m_entry_fixAIOUpdate()
{
    gfx_clearscreen();
    gfx_printf("\n\n-- Behebe fehlerhaftes AIO-switch-updater Update.\n\n");

    char *aio_fs_path = CpyStr("sd:/atmosphere/fusee-secondary.bin.aio");
    char *aio_p_path = CpyStr("sd:/sept/payload.bin.aio");
    char *aio_strt_path = CpyStr("sd:/atmosphere/stratosphere.romfs.aio");

    char *o_fs_path = CpyStr("sd:/atmosphere/fusee-secondary.bin");
    char *o_p_path = CpyStr("sd:/sept/payload.bin");
    char *o_strt_path = CpyStr("sd:/atmosphere/stratosphere.romfs");

    if (FileExists(aio_fs_path))
    {
        gfx_printf("Von AIO aktualisierte fusee-secondary erkannt -> ersetze Original\n\n");
        if (FileExists(o_fs_path))
        {
            _DeleteFileSimple(o_fs_path);
        }
        _RenameFileSimple(aio_fs_path, o_fs_path);
    }
    free(aio_fs_path);
    free(o_fs_path);

    if (FileExists(aio_p_path))
    {
        gfx_printf("Von AIO aktualisierte payload erkannt -> ersetze Original\n\n");
        if (FileExists(o_p_path))
        {
            _DeleteFileSimple(o_p_path);
        }
        _RenameFileSimple(aio_p_path, o_p_path);
    }
    free(aio_p_path);
    free(o_p_path);

    if (FileExists(aio_strt_path))
    {
        gfx_printf("Von AIO aktualisierte stratosphere erkannt -> ersetze Original\n\n");
        if (FileExists(o_strt_path))
        {
            _DeleteFileSimple(o_strt_path);
        }
        _RenameFileSimple(aio_strt_path, o_strt_path);
    }
    free(aio_strt_path);
    free(o_strt_path);
}

void m_entry_fixClingWrap()
{
    gfx_clearscreen();
    gfx_printf("\n\n-- Repariere SigPatches.\n\n");
    char *bpath = CpyStr("sd:/_b0otloader");
    char *bopath = CpyStr("sd:/bootloader");
    char *kpath = CpyStr("sd:/atmosphere/_k1ps");
    char *kopath = CpyStr("sd:/atmosphere/kips");

    char *ppath = CpyStr("sd:/bootloader/_patchesCW.ini");
    char *popath = CpyStr("sd:/atmosphere/patches.ini");

    if (FileExists(bpath))
    {
        if (FileExists(bopath))
        {
            FolderDelete(bopath);
        }
        int res = f_rename(bpath, bopath);
        if (res)
        {
            DrawError(newErrCode(res));
        }
        gfx_printf("-- Bootloader repariert!\n");
    }

    if (FileExists(kpath))
    {
        if (FileExists(kopath))
        {
            FolderDelete(kopath);
        }
        int res = f_rename(kpath, kopath);
        if (res)
        {
            DrawError(newErrCode(res));
        }
        gfx_printf("-- kips repariert!\n");
    }

    if (FileExists(ppath))
    {
        if (FileExists(popath))
        {
            _DeleteFileSimple(popath);
        }
        _RenameFileSimple(ppath, popath);
        gfx_printf("-- patches.ini repariert!\n");
    }

    free(bpath);
    free(bopath);
    free(kpath);
    free(kopath);
    free(ppath);
    free(popath);
}

void _deleteTheme(char *basePath, char *folderId)
{
    char *path = CombinePaths(basePath, folderId);
    if (FileExists(path))
    {
        gfx_printf("-- Gefundene Themes: %s\n", path);
        FolderDelete(path);
    }
    free(path);
}

void m_entry_deleteInstalledThemes()
{
    gfx_clearscreen();
    gfx_printf("\n\n-- Loesche installierte Themes.\n\n");
    _deleteTheme("sd:/atmosphere/contents", "0100000000001000");
    _deleteTheme("sd:/atmosphere/contents", "0100000000001007");
    _deleteTheme("sd:/atmosphere/contents", "0100000000001013");
}

void m_entry_deleteBootFlags()
{
    gfx_clearscreen();
    gfx_printf("\n\n-- Automatisches starten der sysmodule deaktivieren.\n\n");
    char *storedPath = CpyStr("sd:/atmosphere/contents");
    int readRes = 0;
    Vector_t fileVec = ReadFolder(storedPath, &readRes);
    if (readRes)
    {
        clearFileVector(&fileVec);
        DrawError(newErrCode(readRes));
    }
    else
    {
        vecDefArray(FSEntry_t *, fsEntries, fileVec);
        for (int i = 0; i < fileVec.count; i++)
        {

            char *suf = "/flags/boot2.flag";
            char *flagPath = CombinePaths(storedPath, fsEntries[i].name);
            flagPath = CombinePaths(flagPath, suf);

            if (FileExists(flagPath))
            {
                gfx_printf("Loesche: %s\n", flagPath);
                _DeleteFileSimple(flagPath);
            }
            free(flagPath);
        }
    }
}

void m_entry_fixMacSpecialFolders()
{
    listdir("/", 0);

    // browse path
    // list files & folders
    // if file -> delete
    // if folder !== nintendo
    //      if folder m_entry_fixMacSpecialFolders with new path
}

void m_entry_stillNoBootInfo()
{
    gfx_clearscreen();
    gfx_printf("\n\n-- Meine Switch startet immer noch nicht.\n\n");

    gfx_printf("%kSteckt eine Spiel-Cardrige im Slot?\n", COLOR_WHITE);
    gfx_printf("Entferne sie und starte neu.\n\n");

    gfx_printf("%kHast du vor kurzem Atmosphere/SwitchBros. aktualisiert?\n", COLOR_WHITE);
    gfx_printf("Stecke die SD-Karte in deinen PC, loesche 'atmosphere', 'bootloader' & 'sept' Ordner, lade dein bevorzugtes CFW runter und pack die Dateien auf die SD-Karte.\nEinfacher ist es mit unserem SwitchBros. Paket!\nWir bieten dir gerne Hilfe auf unserem SwitchBros. Discord Server an. https://discord.gg/switchbros\n\n");

    gfx_printf("%kHast du eine neue SD-Karte gekauft?\n", COLOR_WHITE);
    gfx_printf("Vergewissere dich das es keine 'fake' Karte ist.\n\n");
}

void m_entry_ViewCredits()
{
    gfx_clearscreen();
    gfx_printf("\nAllgemeinerProblemLoeser v%d.%d.%d.%d\nVon Team Neptune - (Uebersetzt von Switch Bros. und OLED Support hinzugefügt)\n\nBasierend auf TegraExplorer von SuchMemeManySkill,\nLockpick_RCM & Hekate, von shchmue & CTCaer\n\n\n", LP_VER_MJ, LP_VER_MN, LP_VER_BF, LP_VER_SB);
}

void m_entry_fixAll()
{
    gfx_clearscreen();
    m_entry_deleteBootFlags();
    m_entry_deleteInstalledThemes();
    m_entry_fixClingWrap();
    m_entry_fixAIOUpdate();

    m_entry_stillNoBootInfo();
}
