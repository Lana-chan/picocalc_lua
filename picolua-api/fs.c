#include <stdlib.h>
#include <string.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "../pico_fatfs/tf_card.h"
#include "../pico_fatfs/fatfs/ff.h"

#include "../drivers/fs.h"

#define filehandle "FileHandle"

static FIL* checkfile(lua_State *L) {
	void *ud = luaL_checkudata(L, 1, filehandle);
	luaL_argcheck(L, ud != NULL, 1, "'FileHandle' expected");
	return (FIL*)ud;
}

//f_open - Open/Create a file
static int l_fs_open(lua_State* L) {
	const TCHAR* path = luaL_checkstring(L, 1);

	static const BYTE modes[] = {
		FA_READ,
		FA_READ | FA_WRITE,
		FA_CREATE_ALWAYS | FA_WRITE,
		FA_CREATE_ALWAYS | FA_WRITE | FA_READ,
		FA_OPEN_APPEND | FA_WRITE,
		FA_OPEN_APPEND | FA_WRITE | FA_READ,
		FA_CREATE_NEW | FA_WRITE,
		FA_CREATE_NEW | FA_WRITE | FA_READ
	};
	static const char *const modenames[] = {
		"r",
		"r+",
		"w",
		"w+",
		"a",
		"a+",
		"wx",
		"w+x"
	};
	
	BYTE mode = luaL_checkoption(L, 2, "r", modenames);

	FIL* fp = lua_newuserdata(L, sizeof(FIL));
	luaL_getmetatable(L, filehandle);
	lua_setmetatable(L, -2);
	FRESULT result = f_open(fp, path, modes[mode]);
	if (result != FR_OK) return luaL_error(L, fs_error_strings[result]);
	return 1;
}

//f_close - Close an open file
static int l_fs_close(lua_State* L) {
	FIL* fp = checkfile(L);
	if (fp) {
		FRESULT result = f_close(fp);
		if (result != FR_OK) return luaL_error(L, fs_error_strings[result]);
	}
	return 0;
}

//f_read - Read data from the file
static int l_fs_read(lua_State* L) {
	FIL* fp = checkfile(L);
	UINT to_read = luaL_optinteger(L, 2, 1); 
	char* buffer = malloc(to_read);
	UINT read;
	
	FRESULT result = f_read(fp, buffer, to_read, &read);
	if (result != FR_OK) { free(buffer); return luaL_error(L, fs_error_strings[result]); }

	if (read > 0) {
		lua_pushlstring(L, buffer, read);
	} else {
		lua_pushnil(L);
	}

	free(buffer);
	return 1;
}

// read entire file into a string
static int l_fs_readAll(lua_State* L) {
	FIL* fp = checkfile(L);
	UINT to_read = f_size(fp);
	char* buffer = malloc(to_read);
	UINT read;
	
	FRESULT result = f_read(fp, buffer, to_read, &read);
	if (result != FR_OK) { free(buffer); return luaL_error(L, fs_error_strings[result]); }

	if (read > 0) {
		lua_pushlstring(L, buffer, read);
	} else {
		lua_pushnil(L);
	}

	free(buffer);
	return 1;
}

// read until newline
static int l_fs_readLine(lua_State* L) {
	FIL* fp = checkfile(L);

	UINT read;
	char* buffer = NULL;
	FRESULT res = fs_readline(fp, &buffer, &read);
	if (res != FR_OK) { free(buffer); return luaL_error(L, fs_error_strings[res]); }

	if (!buffer) {
		lua_pushnil(L);
		return 1;
	} else if (read > 0) {
		lua_pushlstring(L, buffer, read);
	} else {
		lua_pushlstring(L, "", 0);
	}
	
	free(buffer);
	return 1;
}

//f_write - Write data to the file
static int l_fs_write(lua_State* L) {
	FIL* fp = checkfile(L);
	int to_write;
	const char* buffer = luaL_checklstring(L, 2, &to_write);
	UINT written;
	
	FRESULT result = f_write(fp, buffer, to_write, &written);
	if (result != FR_OK) return luaL_error(L, fs_error_strings[result]);

	lua_pushinteger(L, written);
	return 1;
}

// write data and append newline
static int l_fs_writeLine(lua_State* L) {
	FIL* fp = checkfile(L);
	int to_write;
	const char* buffer = luaL_checklstring(L, 2, &to_write);
	UINT written;

	FRESULT result = fs_writeline(fp, buffer, to_write, &written);
	if (result != FR_OK) return luaL_error(L, fs_error_strings[result]);

	lua_pushinteger(L, written);
	return 1;
}

//f_lseek - Move read/write pointer, Expand size
static int l_fs_seek(lua_State* L) {
	static const int mode[] = {SEEK_SET, SEEK_CUR, SEEK_END};
	static const char *const modenames[] = {"set", "cur", "end", NULL};
	FIL* fp = checkfile(L);
	int op = 1;
	FSIZE_t offset = 0;
	if (lua_type(L, 2) == LUA_TNUMBER) {
		offset = luaL_optinteger(L, 2, 0);
	} else {
		op = luaL_checkoption(L, 2, "cur", modenames);
		offset = luaL_optinteger(L, 3, 0);
	}
	FRESULT result;

	if (offset != 0) {
		switch (mode[op]) {
			case SEEK_SET:
				result = f_lseek(fp, offset);
				break;
			case SEEK_CUR:
				result = f_lseek(fp, fp->fptr + offset);
				break;
			case SEEK_END:
				result = f_lseek(fp, f_size(fp) + offset);
				break;
			default:
				return luaL_error(L, "bad argument #1 to 'seek'");
		}
	
		if (result != FR_OK) return luaL_error(L, fs_error_strings[result]);
	}

	lua_pushinteger(L, fp->fptr);
	return 1;
}

//f_sync - Flush cached data
static int l_fs_flush(lua_State* L) {
	FIL* fp = checkfile(L);
	FRESULT result = f_sync(fp);
	if (result != FR_OK) return luaL_error(L, fs_error_strings[result]);
	return 0;
}

//Directory Access
static int l_fs_list(lua_State* L) {
	const char* path = luaL_checkstring(L, 1);

	DIR dp;
	FRESULT result = f_opendir(&dp, path);
	if (result != FR_OK) return luaL_error(L, fs_error_strings[result]);

	FILINFO fp;
	result = f_readdir(&dp, &fp);
	if (result != FR_OK) {
		f_closedir(&dp);
		return luaL_error(L, fs_error_strings[result]);
	}

	lua_newtable(L);
	int i = 0;
	
	while (result == FR_OK && fp.fname[0] != 0) {
		lua_newtable(L);
		lua_pushstring(L, fp.fname);
		lua_setfield(L, -2, "name");
		lua_pushinteger(L, fp.fsize);
		lua_setfield(L, -2, "size");
		lua_pushboolean(L, fp.fattrib & AM_DIR);
		lua_setfield(L, -2, "isDir");
		lua_rawseti(L, -2, ++i);
		result = f_readdir(&dp, &fp);
	}
	
	f_closedir(&dp);
	if (result != FR_OK) return luaL_error(L, fs_error_strings[result]);

	return 1;
}

//File and Directory Management
//f_stat - Check existance of a file or sub-directory
static int l_fs_getSize(lua_State* L) {
	const char* path = luaL_checkstring(L, 1);
	FILINFO info;
	FRESULT result = f_stat(path, &info);
	if (result != FR_OK || info.fname[0] == 0) return luaL_error(L, fs_error_strings[result]);
	lua_pushinteger(L, info.fsize);
	return 1;
}

// is path a directory?
static int l_fs_isDir(lua_State* L) {
	const char* path = luaL_checkstring(L, 1);
	FILINFO info;
	FRESULT result = f_stat(path, &info);
	if (result != FR_OK || info.fname[0] == 0) return luaL_error(L, fs_error_strings[result]);
	lua_pushboolean(L, info.fattrib & AM_DIR);
	return 1;
}

// is path read only?
static int l_fs_isReadOnly(lua_State* L) {
	const char* path = luaL_checkstring(L, 1);
	FILINFO info;
	FRESULT result = f_stat(path, &info);
	if (result != FR_OK || info.fname[0] == 0) return luaL_error(L, fs_error_strings[result]);
	lua_pushboolean(L, info.fattrib & AM_RDO);
	return 1;
}

// does path exist?
static int l_fs_exists(lua_State* L) {
	const char* path = luaL_checkstring(L, 1);
	lua_pushboolean(L, fs_exists(path));
	return 1;
}

// return attributes table like cc
static int l_fs_attribs(lua_State* L) {
	const char* path = luaL_checkstring(L, 1);
	FILINFO info;
	FRESULT result = f_stat(path, &info);
	if (result != FR_OK || info.fname[0] == 0) return luaL_error(L, fs_error_strings[result]);
	lua_newtable(L);
	lua_pushinteger(L, info.fsize);
	lua_setfield(L, -2, "size");
	lua_pushboolean(L, info.fattrib & AM_DIR);
	lua_setfield(L, -2, "isDir");
	lua_pushboolean(L, info.fattrib & AM_RDO);
	lua_setfield(L, -2, "isReadOnly");
	lua_pushinteger(L, info.fdate);
	lua_setfield(L, -2, "date");
	lua_pushinteger(L, info.ftime);
	lua_setfield(L, -2, "time");
	return 1;
}

//f_unlink - Remove a file or sub-directory
static int l_fs_delete(lua_State* L) {
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

static int l_fs_copy(lua_State* L) {
	const char* src_path = luaL_checkstring(L, 1);
	const char* tgt_path = luaL_checkstring(L, 2);
	FIL src, tgt;
	FRESULT result;
	char buffer[8192];
	UINT read;

	result = f_open(&src, src_path, FA_READ);
	if (result != FR_OK) return luaL_error(L, fs_error_strings[result]);
	result = f_open(&tgt, tgt_path, FA_CREATE_NEW | FA_WRITE);
	if (result != FR_OK) return luaL_error(L, fs_error_strings[result]);

	while(!f_eof(&src)) {
		result = f_read(&src, buffer, sizeof(buffer), &read);
		if (result != FR_OK) return luaL_error(L, fs_error_strings[result]);
		result = f_write(&tgt, buffer, read, NULL);
		if (result != FR_OK) return luaL_error(L, fs_error_strings[result]);
	}

	f_close(&src);
	f_close(&tgt);

	return 0;
}

//f_mkdir - Create a sub-directory
static int l_fs_mkdir(lua_State* L) {
	const char* path = luaL_checkstring(L, 1);
	FRESULT result = f_mkdir(path);
	if (result != FR_OK) return luaL_error(L, fs_error_strings[result]);
	return 0;
}


//Volume Management and System Configuration
//f_getfree - Get free space on the volume
static int l_fs_getfree(lua_State* L) {
	const char* path = luaL_optstring(L, 1, "");

	FATFS *fs;
	DWORD fre_clust, fre_sect, tot_sect;

	FRESULT result = f_getfree(path, &fre_clust, &fs);
	if (result != FR_OK) return luaL_error(L, fs_error_strings[result]);

	DWORD total_sectors = (fs->n_fatent - 2) * fs->csize;
	DWORD free_sectors = fre_clust * fs->csize;

	lua_pushinteger(L, free_sectors / 2);
	lua_pushinteger(L, total_sectors / 2);
	return 2;
}

/*
complete(...)	Provides completion for a file or directory name, suitable for use with _G.read.
find(path)	Searches for files matching a string with wildcards.
isDriveRoot(path)	Returns true if a path is mounted to the parent filesystem.
-list(path)	Returns a list of files in a directory.
combine(path, ...)	Combines several parts of a path into one full path, adding separators as needed.
getName(path)	Returns the file name portion of a path.
getDir(path)	Returns the parent directory portion of a path.
-getSize(path)	Returns the size of the specified file.
-exists(path)	Returns whether the specified path exists.
-isDir(path)	Returns whether the specified path is a directory.
-isReadOnly(path)	Returns whether a path is read-only.
-makeDir(path)	Creates a directory, and any missing parents, at the specified path.
-move(path, dest)	Moves a file or directory from one path to another.
-copy(path, dest)	Copies a file or directory to a new path.
-delete(path)	Deletes a file or directory.
-open(path, mode)	Opens a file for reading or writing at a path.
getDrive(path)	Returns the name of the mount that the specified path is located on.
-getFreeSpace(path)	Returns the amount of free space available on the drive the path is located on.
getCapacity(path)	Returns the capacity of the drive the path is located on.
-attributes(path)	Get attributes about a specific file or folder.
*/

int luaopen_fs(lua_State *L) {
	static const luaL_Reg fslib_m[] = {
		{"read", l_fs_read},
		{"readAll", l_fs_readAll},
		{"readLine", l_fs_readLine},
		{"seek", l_fs_seek},
		{"write", l_fs_write},
		{"writeLine", l_fs_writeLine},
		{"flush", l_fs_flush},
		{"close", l_fs_close},
		{"__gc", l_fs_close}, // i think this is correct? i can't find much on __gc
		{NULL, NULL}
	};
	
	static const luaL_Reg fslib_f [] = {
		{"open", l_fs_open},
		{"list", l_fs_list},
		{"makeDir", l_fs_mkdir},
		{"getSize", l_fs_getSize},
		{"isDir", l_fs_isDir},
		{"isReadOnly", l_fs_isReadOnly},
		{"attributes", l_fs_attribs},
		{"exists", l_fs_exists},
		{"delete", l_fs_delete},
		{"move", l_fs_rename},
		{"copy", l_fs_copy},
		{"getFreeSpace", l_fs_getfree},
		{NULL, NULL}
	};
	
	luaL_newlib(L, fslib_f);

	luaL_newmetatable(L, filehandle);
	luaL_setfuncs(L, fslib_m, 0);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");

	lua_setfield(L, -2, filehandle);
	
	return 1;
}