#include <stdlib.h>

#include "../pico_fatfs/tf_card.h"
#include "../pico_fatfs/fatfs/ff.h"

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "modules.h"
#include "../drivers/fs.h"

static char* fs_error_strings[20] = {
  "Succeeded",
  "A hard error occurred in the low level disk I/O layer",
  "Assertion failed",
  "The physical drive cannot work",
  "Could not find the file",
  "Could not find the path",
  "The path name format is invalid",
  "Access denied due to prohibited access or directory full",
  "Access denied due to prohibited access",
  "The file/directory object is invalid",
  "The physical drive is write protected",
  "The logical drive number is invalid",
  "The volume has no work area",
  "There is no valid FAT volume",
  "The f_mkfs() aborted due to any problem",
  "Could not get a grant to access the volume within defined period",
  "The operation is rejected according to the file sharing policy",
  "LFN working buffer could not be allocated",
  "Number of open files > FF_FS_LOCK",
  "Given parameter is invalid",
};

//f_open - Open/Create a file
static int l_fs_open(lua_State* L) {
  const TCHAR* path = luaL_checkstring(L, 1);
  BYTE mode = luaL_checkinteger(L, 2);
  FIL* fp = lua_newuserdata(L, sizeof(FIL));
  FRESULT result = f_open(fp, path, mode);
  if (result != FR_OK) return luaL_error(L, fs_error_strings[result]);
  return 1;
}

//f_close - Close an open file
static int l_fs_close(lua_State* L) {
  FIL* fp = (FIL*) lua_touserdata(L, 1);
  FRESULT result = f_close(fp);
  if (result != FR_OK) return luaL_error(L, fs_error_strings[result]);
  return 0;
}

//f_read - Read data from the file
static int l_fs_read(lua_State* L) {
  FIL* fp = (FIL*) lua_touserdata(L, 1);
  UINT to_read = luaL_checkinteger(L, 2); 
  char* buffer = malloc(to_read);
  UINT read;
  
  FRESULT result = f_read (fp, buffer, to_read, &read);
  if (result != FR_OK) return luaL_error(L, fs_error_strings[result]);

  lua_pushlstring(L, buffer, read);
  return 1;
}

//f_write - Write data to the file
static int l_fs_write(lua_State* L) {
  FIL* fp = (FIL*) lua_touserdata(L, 1);
  int to_write;
  const char* buffer = luaL_checklstring(L, 2, &to_write);
  UINT written;
  
  FRESULT result = f_write (fp, buffer, to_write, &written);
  if (result != FR_OK) return luaL_error(L, fs_error_strings[result]);

  lua_pushinteger(L, written);
  return 1;
}

//f_lseek - Move read/write pointer, Expand size
static int l_fs_seek(lua_State* L) {
  FIL* fp = (FIL*) lua_touserdata(L, 1);
  FSIZE_t offset = luaL_checkinteger(L, 2);
  FRESULT result = f_lseek(fp, offset);
  if (result != FR_OK) return luaL_error(L, fs_error_strings[result]);
  return 0;
}

//f_truncate - Truncate file size
//f_sync - Flush cached data
static int l_fs_sync(lua_State* L) {
  FIL* fp = (FIL*) lua_touserdata(L, 1);
  FRESULT result = f_sync(fp);
  if (result != FR_OK) return luaL_error(L, fs_error_strings[result]);
  return 0;
}

//f_forward - Forward data to the stream
//f_expand - Allocate a contiguous block to the file
//f_gets - Read a string
//f_putc - Write a character
//f_puts - Write a string
//f_printf - Write a formatted string
//f_tell - Get current read/write pointer
static int l_fs_tell(lua_State* L) {
  FIL* fp = (FIL*) lua_touserdata(L, 1);
  FSIZE_t offset = f_tell(fp);
  lua_pushinteger(L, offset);
  return 1;
}

//f_eof - Test for end-of-file
static int l_fs_eof(lua_State* L) {
  FIL* fp = (FIL*) lua_touserdata(L, 1);
  int eof = f_eof(fp);
  lua_pushinteger(L, eof);
  return 1;
}

//f_size - Get size
static int l_fs_size(lua_State* L) {
  FIL* fp = (FIL*) lua_touserdata(L, 1);
  FSIZE_t size = f_size(fp);
  lua_pushinteger(L, size);
  return 1;
}

//f_error - Test for an error

//Directory Access
//f_opendir - Open a directory
static int l_fs_opendir(lua_State* L) {
  const char* path = luaL_checkstring(L, 1);
  DIR* dir = lua_newuserdata(L, sizeof(DIR));
  FRESULT result = f_opendir(dir, path);
  if (result != FR_OK) return luaL_error(L, fs_error_strings[result]);
  return 1;
}

//f_closedir - Close an open directory
static int l_fs_closedir(lua_State* L) {
  DIR* dir = (DIR*) lua_touserdata(L, 1);
  FRESULT result = f_closedir(dir);
  if (result != FR_OK) return luaL_error(L, fs_error_strings[result]);
  return 0;
}

//f_readdir - Read a directory item
static int l_fs_readdir(lua_State* L) {
  DIR* dir = (DIR*) lua_touserdata(L, 1);
  FILINFO info;
  FRESULT result = f_readdir(dir, &info);
  if (result != FR_OK || info.fname[0] == 0) return 0;
  lua_pushinteger(L, info.fsize);
  lua_pushboolean(L, info.fattrib & AM_DIR);
  lua_pushstring(L, info.fname);
  return 3;
}
//f_findfirst - Open a directory and read the first item matched
//f_findnext - Read a next item matched

//File and Directory Management
//f_stat - Check existance of a file or sub-directory
static int l_fs_stat(lua_State* L) {
  const char* path = luaL_checkstring(L, 1);
  FILINFO info;
  FRESULT result = f_stat(path, &info);
  if (result != FR_OK || info.fname[0] == 0) return 0;
  lua_pushinteger(L, info.fsize);
  lua_pushboolean(L, info.fattrib & AM_DIR);
  lua_pushstring(L, info.fname);
  return 3;
}

//f_unlink - Remove a file or sub-directory
static int l_fs_unlink(lua_State* L) {
  const char* path = luaL_checkstring(L, 1);
  FRESULT result = f_unlink(path);
  if (result != FR_OK) return luaL_error(L, fs_error_strings[result]);
  return 0;
}

//f_rename - Rename/Move a file or sub-directory
static int l_fs_rename(lua_State* L) {
  const char* old_path = luaL_checkstring(L, 1);
  const char* new_path = luaL_checkstring(L, 2);
  FRESULT result = f_rename(old_path, new_path);
  if (result != FR_OK) return luaL_error(L, fs_error_strings[result]);
  return 0;
}

//f_chmod - Change attribute of a file or sub-directory
//f_utime - Change timestamp of a file or sub-directory
//f_mkdir - Create a sub-directory
static int l_fs_mkdir(lua_State* L) {
  const char* path = luaL_checkstring(L, 1);
  FRESULT result = f_mkdir(path);
  if (result != FR_OK) return luaL_error(L, fs_error_strings[result]);
  return 0;
}

//f_chdir - Change current directory
static int l_fs_chdir(lua_State* L) {
  const char* path = luaL_checkstring(L, 1);
  FRESULT result = f_chdir(path);
  if (result != FR_OK) return luaL_error(L, fs_error_strings[result]);
  return 0;
}

//f_chdrive - Change current drive
//f_getcwd - Retrieve the current directory and drive
/*int l_fs_getcwd(lua_State* L) {
  char path[256];
  FRESULT result = f_getcwd(path, 256);
  if (result != FR_OK) return luaL_error(L, fs_error_strings[result]);
  lua_pushstring(L, path);
  return 1;
}*/


//Volume Management and System Configuration
//f_mount - Register/Unregister the work area of the volume
static int l_fs_mount(lua_State* L) {
  const TCHAR* path = luaL_checkstring(L, 1);
  BYTE opt = luaL_checkinteger(L, 2);
  FATFS* fs = lua_newuserdata(L, sizeof(FATFS));
  FRESULT result = f_mount(fs, path, opt);
  if (result != FR_OK) return luaL_error(L, fs_error_strings[result]);
  return 1;
}

//f_mkfs - Create an FAT volume on the logical drive
//f_fdisk - Create partitions on the physical drive
//f_getfree - Get free space on the volume
static int l_fs_getfree(lua_State* L) {
  const char* path = luaL_checkstring(L, 1);

  FATFS *fs;
  DWORD fre_clust, fre_sect, tot_sect;

  FRESULT result = f_getfree(path, &fre_clust, &fs);
  if (result != FR_OK) return luaL_error(L, fs_error_strings[result]);

  DWORD total_sectors = (fs->n_fatent - 2) * fs->csize;
  DWORD free_sectors = fre_clust * fs->csize;

  lua_pushinteger(L, total_sectors / 2);
  lua_pushinteger(L, free_sectors / 2);
  return 2;
}

//f_getlabel - Get volume label
//f_setlabel - Set volume label
//f_setcp - Set active code page

void fs_register_wrapper(lua_State* L) {
  lua_newtable(L);
  lua_table_register(L, "open", l_fs_open);
  lua_table_register(L, "close", l_fs_close);
  lua_table_register(L, "read", l_fs_read);
  lua_table_register(L, "write", l_fs_write);
  lua_table_register(L, "seek", l_fs_seek);
  lua_table_register(L, "tell", l_fs_tell);
  lua_table_register(L, "eof", l_fs_eof);
  lua_table_register(L, "size", l_fs_size);
  lua_table_register(L, "sync", l_fs_sync);

  lua_table_register(L, "opendir", l_fs_opendir);
  lua_table_register(L, "closedir", l_fs_closedir);
  lua_table_register(L, "readdir", l_fs_readdir);

  lua_table_register(L, "stat", l_fs_stat);
  lua_table_register(L, "unlink", l_fs_unlink);
  lua_table_register(L, "rename", l_fs_rename);
  lua_table_register(L, "mkdir", l_fs_mkdir);
  lua_table_register(L, "chdir", l_fs_chdir);
  //lua_register(L, "fs_getcwd", l_fs_getcwd);

  lua_table_register(L, "mount", l_fs_mount);
  lua_table_register(L, "getfree", l_fs_getfree);
  lua_setglobal(L, "fs");
}
