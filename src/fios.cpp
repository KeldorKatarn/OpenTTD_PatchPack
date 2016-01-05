/* $Id: fios.cpp 26423 2014-03-23 14:55:32Z planetmaker $ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file fios.cpp
 * This file contains functions for building file lists for the save/load dialogs.
 */

#include "stdafx.h"
#include "fios.h"
#include "fileio_func.h"
#include "tar_type.h"
#include "screenshot.h"
#include "string_func.h"
#include <sys/stat.h>

#ifndef WIN32
# include <unistd.h>
#endif /* WIN32 */

#include "table/strings.h"

#include "safeguards.h"

/* Variables to display file lists */
SmallVector<FiosItem, 32> _fios_items;
static char *_fios_path;
static const char *_fios_path_last;
SmallFiosItem _file_to_saveload;
SortingBits _savegame_sort_order = SORT_BY_DATE | SORT_DESCENDING;

/* OS-specific functions are taken from their respective files (win32/unix/os2 .c) */
extern bool FiosIsRoot(const char *path);
extern bool FiosIsValidFile(const char *path, const struct dirent *ent, struct stat *sb);
extern bool FiosIsHiddenFile(const struct dirent *ent);
extern void FiosGetDrives();
extern bool FiosGetDiskFreeSpace(const char *path, uint64 *tot);

/* get the name of an oldstyle savegame */
extern void GetOldSaveGameName(const char *file, char *title, const char *last);

/**
 * Compare two FiosItem's. Used with sort when sorting the file list.
 * @param da A pointer to the first FiosItem to compare.
 * @param db A pointer to the second FiosItem to compare.
 * @return -1, 0 or 1, depending on how the two items should be sorted.
 */
int CDECL CompareFiosItems(const FiosItem *da, const FiosItem *db)
{
	int r = 0;

	if ((_savegame_sort_order & SORT_BY_NAME) == 0 && da->mtime != db->mtime) {
		r = da->mtime < db->mtime ? -1 : 1;
	} else {
		r = strcasecmp(da->title, db->title);
	}

	if (_savegame_sort_order & SORT_DESCENDING) r = -r;
	return r;
}

/** Free the list of savegames. */
void FiosFreeSavegameList()
{
	_fios_items.Clear();
	_fios_items.Compact();
}

/**
 * Get descriptive texts. Returns the path and free space
 * left on the device
 * @param path string describing the path
 * @param total_free total free space in megabytes, optional (can be NULL)
 * @return StringID describing the path (free space or failure)
 */
StringID FiosGetDescText(const char **path, uint64 *total_free)
{
	*path = _fios_path;
	return FiosGetDiskFreeSpace(*path, total_free) ? STR_SAVELOAD_BYTES_FREE : STR_ERROR_UNABLE_TO_READ_DRIVE;
}

/**
 * Browse to a new path based on the passed \a item, starting at #_fios_path.
 * @param *item Item telling us what to do.
 * @return A filename w/path if we reached a file, otherwise \c NULL.
 */
const char *FiosBrowseTo(const FiosItem *item)
{
	switch (item->type) {
		case FIOS_TYPE_DRIVE:
#if defined(WINCE)
			seprintf(_fios_path, _fios_path_last, PATHSEP "");
#elif defined(WIN32) || defined(__OS2__)
			seprintf(_fios_path, _fios_path_last, "%c:" PATHSEP, item->title[0]);
#endif
			/* FALL THROUGH */
		case FIOS_TYPE_INVALID:
			break;

		case FIOS_TYPE_PARENT: {
			/* Check for possible NULL ptr (not required for UNIXes, but AmigaOS-alikes) */
			char *s = strrchr(_fios_path, PATHSEPCHAR);
			if (s != NULL && s != _fios_path) {
				s[0] = '\0'; // Remove last path separator character, so we can go up one level.
			}
			s = strrchr(_fios_path, PATHSEPCHAR);
			if (s != NULL) {
				s[1] = '\0'; // go up a directory
#if defined(__MORPHOS__) || defined(__AMIGAOS__)
			/* On MorphOS or AmigaOS paths look like: "Volume:directory/subdirectory" */
			} else if ((s = strrchr(_fios_path, ':')) != NULL) {
				s[1] = '\0';
#endif
			}
			break;
		}

		case FIOS_TYPE_DIR:
			strecat(_fios_path, item->name, _fios_path_last);
			strecat(_fios_path, PATHSEP, _fios_path_last);
			break;

		case FIOS_TYPE_DIRECT:
			seprintf(_fios_path, _fios_path_last, "%s", item->name);
			break;

		case FIOS_TYPE_FILE:
		case FIOS_TYPE_OLDFILE:
		case FIOS_TYPE_SCENARIO:
		case FIOS_TYPE_OLD_SCENARIO:
		case FIOS_TYPE_PNG:
		case FIOS_TYPE_BMP:
			return item->name;
	}

	return NULL;
}

/**
 * Construct a filename from its components in destination buffer \a buf.
 * @param buf Destination buffer.
 * @param path Directory path, may be \c NULL.
 * @param name Filename.
 * @param ext Filename extension (use \c "" for no extension).
 * @param last Last element of buffer \a buf.
 */
static void FiosMakeFilename(char *buf, const char *path, const char *name, const char *ext, const char *last)
{
	const char *period;

	/* Don't append the extension if it is already there */
	period = strrchr(name, '.');
	if (period != NULL && strcasecmp(period, ext) == 0) ext = "";
#if  defined(__MORPHOS__) || defined(__AMIGAOS__)
	if (path != NULL) {
		unsigned char sepchar = path[(strlen(path) - 1)];

		if (sepchar != ':' && sepchar != '/') {
			seprintf(buf, last, "%s" PATHSEP "%s%s", path, name, ext);
		} else {
			seprintf(buf, last, "%s%s%s", path, name, ext);
		}
	} else {
		seprintf(buf, last, "%s%s", name, ext);
	}
#else
	seprintf(buf, last, "%s" PATHSEP "%s%s", path, name, ext);
#endif
}

/**
 * Make a save game or scenario filename from a name.
 * @param buf Destination buffer for saving the filename.
 * @param name Name of the file.
 * @param last Last element of buffer \a buf.
 */
void FiosMakeSavegameName(char *buf, const char *name, const char *last)
{
	const char *extension = (_game_mode == GM_EDITOR) ? ".scn" : ".sav";

	FiosMakeFilename(buf, _fios_path, name, extension, last);
}

/**
 * Construct a filename for a height map.
 * @param buf Destination buffer.
 * @param name Filename.
 * @param last Last element of buffer \a buf.
 */
void FiosMakeHeightmapName(char *buf, const char *name, const char *last)
{
	char ext[5];
	ext[0] = '.';
	strecpy(ext + 1, GetCurrentScreenshotExtension(), lastof(ext));

	FiosMakeFilename(buf, _fios_path, name, ext, last);
}

/**
 * Delete a file.
 * @param name Filename to delete.
 * @return Whether the file deletion was successful.
 */
bool FiosDelete(const char *name)
{
	char filename[512];

	FiosMakeSavegameName(filename, name, lastof(filename));
	return unlink(filename) == 0;
}

typedef FiosType fios_getlist_callback_proc(SaveLoadDialogMode mode, const char *filename, const char *ext, char *title, const char *last);

/**
 * Scanner to scan for a particular type of FIOS file.
 */
class FiosFileScanner : public FileScanner {
	SaveLoadDialogMode mode; ///< The mode we want to search for
	fios_getlist_callback_proc *callback_proc; ///< Callback to check whether the file may be added
public:
	/**
	 * Create the scanner
	 *  @param mode The mode we are in. Some modes don't allow 'parent'.
	 *  @param callback_proc The function that is called where you need to do the filtering.
	 */
	FiosFileScanner(SaveLoadDialogMode mode, fios_getlist_callback_proc *callback_proc) :
		mode(mode),
		callback_proc(callback_proc)
	{}

	/* virtual */ bool AddFile(const char *filename, size_t basepath_length, const char *tar_filename);
};

/**
 * Try to add a fios item set with the given filename.
 * @param filename        the full path to the file to read
 * @param basepath_length amount of characters to chop of before to get a relative filename
 * @return true if the file is added.
 */
bool FiosFileScanner::AddFile(const char *filename, size_t basepath_length, const char *tar_filename)
{
	const char *ext = strrchr(filename, '.');
	if (ext == NULL) return false;

	char fios_title[64];
	fios_title[0] = '\0'; // reset the title;

	FiosType type = this->callback_proc(this->mode, filename, ext, fios_title, lastof(fios_title));
	if (type == FIOS_TYPE_INVALID) return false;

	for (const FiosItem *fios = _fios_items.Begin(); fios != _fios_items.End(); fios++) {
		if (strcmp(fios->name, filename) == 0) return false;
	}

	FiosItem *fios = _fios_items.Append();
#ifdef WIN32
	struct _stat sb;
	if (_tstat(OTTD2FS(filename), &sb) == 0) {
#else
	struct stat sb;
	if (stat(filename, &sb) == 0) {
#endif
		fios->mtime = sb.st_mtime;
	} else {
		fios->mtime = 0;
	}

	fios->type = type;
	strecpy(fios->name, filename, lastof(fios->name));

	/* If the file doesn't have a title, use its filename */
	const char *t = fios_title;
	if (StrEmpty(fios_title)) {
		t = strrchr(filename, PATHSEPCHAR);
		t = (t == NULL) ? filename : (t + 1);
	}
	strecpy(fios->title, t, lastof(fios->title));
	str_validate(fios->title, lastof(fios->title));

	return true;
}


/**
 * Fill the list of the files in a directory, according to some arbitrary rule.
 *  @param mode The mode we are in. Some modes don't allow 'parent'.
 *  @param callback_proc The function that is called where you need to do the filtering.
 *  @param subdir The directory from where to start (global) searching.
 */
static void FiosGetFileList(SaveLoadDialogMode mode, fios_getlist_callback_proc *callback_proc, Subdirectory subdir)
{
	struct stat sb;
	struct dirent *dirent;
	DIR *dir;
	FiosItem *fios;
	int sort_start;
	char d_name[sizeof(fios->name)];

	_fios_items.Clear();

	/* A parent directory link exists if we are not in the root directory */
	if (!FiosIsRoot(_fios_path)) {
		fios = _fios_items.Append();
		fios->type = FIOS_TYPE_PARENT;
		fios->mtime = 0;
		strecpy(fios->name, "..", lastof(fios->name));
		strecpy(fios->title, ".. (Parent directory)", lastof(fios->title));
	}

	/* Show subdirectories */
	if ((dir = ttd_opendir(_fios_path)) != NULL) {
		while ((dirent = readdir(dir)) != NULL) {
			strecpy(d_name, FS2OTTD(dirent->d_name), lastof(d_name));

			/* found file must be directory, but not '.' or '..' */
			if (FiosIsValidFile(_fios_path, dirent, &sb) && S_ISDIR(sb.st_mode) &&
					(!FiosIsHiddenFile(dirent) || strncasecmp(d_name, PERSONAL_DIR, strlen(d_name)) == 0) &&
					strcmp(d_name, ".") != 0 && strcmp(d_name, "..") != 0) {
				fios = _fios_items.Append();
				fios->type = FIOS_TYPE_DIR;
				fios->mtime = 0;
				strecpy(fios->name, d_name, lastof(fios->name));
				seprintf(fios->title, lastof(fios->title), "%s" PATHSEP " (Directory)", d_name);
				str_validate(fios->title, lastof(fios->title));
			}
		}
		closedir(dir);
	}

	/* Sort the subdirs always by name, ascending, remember user-sorting order */
	{
		SortingBits order = _savegame_sort_order;
		_savegame_sort_order = SORT_BY_NAME | SORT_ASCENDING;
		QSortT(_fios_items.Begin(), _fios_items.Length(), CompareFiosItems);
		_savegame_sort_order = order;
	}

	/* This is where to start sorting for the filenames */
	sort_start = _fios_items.Length();

	/* Show files */
	FiosFileScanner scanner(mode, callback_proc);
	if (subdir == NO_DIRECTORY) {
		scanner.Scan(NULL, _fios_path, false);
	} else {
		scanner.Scan(NULL, subdir, true, true);
	}

	QSortT(_fios_items.Get(sort_start), _fios_items.Length() - sort_start, CompareFiosItems);

	/* Show drives */
	FiosGetDrives();

	_fios_items.Compact();
}

/**
 * Get the title of a file, which (if exists) is stored in a file named
 * the same as the data file but with '.title' added to it.
 * @param file filename to get the title for
 * @param title the title buffer to fill
 * @param last the last element in the title buffer
 * @param subdir the sub directory to search in
 */
static void GetFileTitle(const char *file, char *title, const char *last, Subdirectory subdir)
{
	char buf[MAX_PATH];
	strecpy(buf, file, lastof(buf));
	strecat(buf, ".title", lastof(buf));

	FILE *f = FioFOpenFile(buf, "r", subdir);
	if (f == NULL) return;

	size_t read = fread(title, 1, last - title, f);
	assert(title + read <= last);
	title[read] = '\0';
	str_validate(title, last);
	FioFCloseFile(f);
}

/**
 * Callback for FiosGetFileList. It tells if a file is a savegame or not.
 * @param mode Save/load mode.
 * @param file Name of the file to check.
 * @param ext A pointer to the extension identifier inside file
 * @param title Buffer if a callback wants to lookup the title of the file; NULL to skip the lookup
 * @param last Last available byte in buffer (to prevent buffer overflows); not used when title == NULL
 * @return a FIOS_TYPE_* type of the found file, FIOS_TYPE_INVALID if not a savegame
 * @see FiosGetFileList
 * @see FiosGetSavegameList
 */
FiosType FiosGetSavegameListCallback(SaveLoadDialogMode mode, const char *file, const char *ext, char *title, const char *last)
{
	/* Show savegame files
	 * .SAV OpenTTD saved game
	 * .SS1 Transport Tycoon Deluxe preset game
	 * .SV1 Transport Tycoon Deluxe (Patch) saved game
	 * .SV2 Transport Tycoon Deluxe (Patch) saved 2-player game */

	/* Don't crash if we supply no extension */
	if (ext == NULL) return FIOS_TYPE_INVALID;

	if (strcasecmp(ext, ".sav") == 0) {
		GetFileTitle(file, title, last, SAVE_DIR);
		return FIOS_TYPE_FILE;
	}

	if (mode == SLD_LOAD_GAME || mode == SLD_LOAD_SCENARIO) {
		if (strcasecmp(ext, ".ss1") == 0 || strcasecmp(ext, ".sv1") == 0 ||
				strcasecmp(ext, ".sv2") == 0) {
			if (title != NULL) GetOldSaveGameName(file, title, last);
			return FIOS_TYPE_OLDFILE;
		}
	}

	return FIOS_TYPE_INVALID;
}

/**
 * Get a list of savegames.
 * @param mode Save/load mode.
 * @see FiosGetFileList
 */
void FiosGetSavegameList(SaveLoadDialogMode mode)
{
	static char *fios_save_path = NULL;
	static char *fios_save_path_last = NULL;

	if (fios_save_path == NULL) {
		fios_save_path = MallocT<char>(MAX_PATH);
		fios_save_path_last = fios_save_path + MAX_PATH - 1;
		FioGetDirectory(fios_save_path, fios_save_path_last, SAVE_DIR);
	}

	_fios_path = fios_save_path;
	_fios_path_last = fios_save_path_last;

	FiosGetFileList(mode, &FiosGetSavegameListCallback, NO_DIRECTORY);
}

/**
 * Callback for FiosGetFileList. It tells if a file is a scenario or not.
 * @param mode Save/load mode.
 * @param file Name of the file to check.
 * @param ext A pointer to the extension identifier inside file
 * @param title Buffer if a callback wants to lookup the title of the file
 * @param last Last available byte in buffer (to prevent buffer overflows)
 * @return a FIOS_TYPE_* type of the found file, FIOS_TYPE_INVALID if not a scenario
 * @see FiosGetFileList
 * @see FiosGetScenarioList
 */
static FiosType FiosGetScenarioListCallback(SaveLoadDialogMode mode, const char *file, const char *ext, char *title, const char *last)
{
	/* Show scenario files
	 * .SCN OpenTTD style scenario file
	 * .SV0 Transport Tycoon Deluxe (Patch) scenario
	 * .SS0 Transport Tycoon Deluxe preset scenario */
	if (strcasecmp(ext, ".scn") == 0) {
		GetFileTitle(file, title, last, SCENARIO_DIR);
		return FIOS_TYPE_SCENARIO;
	}

	if (mode == SLD_LOAD_GAME || mode == SLD_LOAD_SCENARIO) {
		if (strcasecmp(ext, ".sv0") == 0 || strcasecmp(ext, ".ss0") == 0 ) {
			GetOldSaveGameName(file, title, last);
			return FIOS_TYPE_OLD_SCENARIO;
		}
	}

	return FIOS_TYPE_INVALID;
}

/**
 * Get a list of scenarios.
 * @param mode Save/load mode.
 * @see FiosGetFileList
 */
void FiosGetScenarioList(SaveLoadDialogMode mode)
{
	static char *fios_scn_path = NULL;
	static char *fios_scn_path_last = NULL;

	/* Copy the default path on first run or on 'New Game' */
	if (fios_scn_path == NULL) {
		fios_scn_path = MallocT<char>(MAX_PATH);
		fios_scn_path_last = fios_scn_path + MAX_PATH - 1;
		FioGetDirectory(fios_scn_path, fios_scn_path_last, SCENARIO_DIR);
	}

	_fios_path = fios_scn_path;
	_fios_path_last = fios_scn_path_last;

	char base_path[MAX_PATH];
	FioGetDirectory(base_path, lastof(base_path), SCENARIO_DIR);

	FiosGetFileList(mode, &FiosGetScenarioListCallback, (mode == SLD_LOAD_SCENARIO && strcmp(base_path, _fios_path) == 0) ? SCENARIO_DIR : NO_DIRECTORY);
}

static FiosType FiosGetHeightmapListCallback(SaveLoadDialogMode mode, const char *file, const char *ext, char *title, const char *last)
{
	/* Show heightmap files
	 * .PNG PNG Based heightmap files
	 * .BMP BMP Based heightmap files
	 */

	FiosType type = FIOS_TYPE_INVALID;

#ifdef WITH_PNG
	if (strcasecmp(ext, ".png") == 0) type = FIOS_TYPE_PNG;
#endif /* WITH_PNG */

	if (strcasecmp(ext, ".bmp") == 0) type = FIOS_TYPE_BMP;

	if (type == FIOS_TYPE_INVALID) return FIOS_TYPE_INVALID;

	TarFileList::iterator it = _tar_filelist[SCENARIO_DIR].find(file);
	if (it != _tar_filelist[SCENARIO_DIR].end()) {
		/* If the file is in a tar and that tar is not in a heightmap
		 * directory we are for sure not supposed to see it.
		 * Examples of this are pngs part of documentation within
		 * collections of NewGRFs or 32 bpp graphics replacement PNGs.
		 */
		bool match = false;
		Searchpath sp;
		FOR_ALL_SEARCHPATHS(sp) {
			char buf[MAX_PATH];
			FioAppendDirectory(buf, lastof(buf), sp, HEIGHTMAP_DIR);

			if (strncmp(buf, it->second.tar_filename, strlen(buf)) == 0) {
				match = true;
				break;
			}
		}

		if (!match) return FIOS_TYPE_INVALID;
	}

	GetFileTitle(file, title, last, HEIGHTMAP_DIR);

	return type;
}

/**
 * Get a list of heightmaps.
 * @param mode Save/load mode.
 */
void FiosGetHeightmapList(SaveLoadDialogMode mode)
{
	static char *fios_hmap_path = NULL;
	static char *fios_hmap_path_last = NULL;

	if (fios_hmap_path == NULL) {
		fios_hmap_path = MallocT<char>(MAX_PATH);
		fios_hmap_path_last = fios_hmap_path + MAX_PATH - 1;
		FioGetDirectory(fios_hmap_path, fios_hmap_path_last, HEIGHTMAP_DIR);
	}

	_fios_path = fios_hmap_path;
	_fios_path_last = fios_hmap_path_last;

	char base_path[MAX_PATH];
	FioGetDirectory(base_path, lastof(base_path), HEIGHTMAP_DIR);

	FiosGetFileList(mode, &FiosGetHeightmapListCallback, strcmp(base_path, _fios_path) == 0 ? HEIGHTMAP_DIR : NO_DIRECTORY);
}

/**
 * Get the directory for screenshots.
 * @return path to screenshots
 */
const char *FiosGetScreenshotDir()
{
	static char *fios_screenshot_path = NULL;

	if (fios_screenshot_path == NULL) {
		fios_screenshot_path = MallocT<char>(MAX_PATH);
		FioGetDirectory(fios_screenshot_path, fios_screenshot_path + MAX_PATH - 1, SCREENSHOT_DIR);
	}

	return fios_screenshot_path;
}

#if defined(ENABLE_NETWORK)
#include "network/network_content.h"
#include "3rdparty/md5/md5.h"

/** Basic data to distinguish a scenario. Used in the server list window */
struct ScenarioIdentifier {
	uint32 scenid;           ///< ID for the scenario (generated by content).
	uint8 md5sum[16];        ///< MD5 checksum of file.
	char filename[MAX_PATH]; ///< filename of the file.

	bool operator == (const ScenarioIdentifier &other) const
	{
		return this->scenid == other.scenid &&
				memcmp(this->md5sum, other.md5sum, sizeof(this->md5sum)) == 0;
	}

	bool operator != (const ScenarioIdentifier &other) const
	{
		return !(*this == other);
	}
};

/**
 * Scanner to find the unique IDs of scenarios
 */
class ScenarioScanner : protected FileScanner, public SmallVector<ScenarioIdentifier, 8> {
	bool scanned; ///< Whether we've already scanned
public:
	/** Initialise */
	ScenarioScanner() : scanned(false) {}

	/**
	 * Scan, but only if it's needed.
	 * @param rescan whether to force scanning even when it's not necessary
	 */
	void Scan(bool rescan)
	{
		if (this->scanned && !rescan) return;

		this->FileScanner::Scan(".id", SCENARIO_DIR, true, true);
		this->scanned = true;
	}

	/* virtual */ bool AddFile(const char *filename, size_t basepath_length, const char *tar_filename)
	{
		FILE *f = FioFOpenFile(filename, "r", SCENARIO_DIR);
		if (f == NULL) return false;

		ScenarioIdentifier id;
		int fret = fscanf(f, "%i", &id.scenid);
		FioFCloseFile(f);
		if (fret != 1) return false;
		strecpy(id.filename, filename, lastof(id.filename));

		Md5 checksum;
		uint8 buffer[1024];
		char basename[MAX_PATH]; ///< \a filename without the extension.
		size_t len, size;

		/* open the scenario file, but first get the name.
		 * This is safe as we check on extension which
		 * must always exist. */
		strecpy(basename, filename, lastof(basename));
		*strrchr(basename, '.') = '\0';
		f = FioFOpenFile(basename, "rb", SCENARIO_DIR, &size);
		if (f == NULL) return false;

		/* calculate md5sum */
		while ((len = fread(buffer, 1, (size > sizeof(buffer)) ? sizeof(buffer) : size, f)) != 0 && size != 0) {
			size -= len;
			checksum.Append(buffer, len);
		}
		checksum.Finish(id.md5sum);

		FioFCloseFile(f);

		this->Include(id);
		return true;
	}
};

/** Scanner for scenarios */
static ScenarioScanner _scanner;

/**
 * Find a given scenario based on its unique ID.
 * @param ci The content info to compare it to.
 * @param md5sum Whether to look at the md5sum or the id.
 * @return The filename of the file, else \c NULL.
 */
const char *FindScenario(const ContentInfo *ci, bool md5sum)
{
	_scanner.Scan(false);

	for (ScenarioIdentifier *id = _scanner.Begin(); id != _scanner.End(); id++) {
		if (md5sum ? (memcmp(id->md5sum, ci->md5sum, sizeof(id->md5sum)) == 0)
		           : (id->scenid == ci->unique_id)) {
			return id->filename;
		}
	}

	return NULL;
}

/**
 * Check whether we've got a given scenario based on its unique ID.
 * @param ci The content info to compare it to.
 * @param md5sum Whether to look at the md5sum or the id.
 * @return True iff we've got the scenario.
 */
bool HasScenario(const ContentInfo *ci, bool md5sum)
{
	return (FindScenario(ci, md5sum) != NULL);
}

/**
 * Force a (re)scan of the scenarios.
 */
void ScanScenarios()
{
	_scanner.Scan(true);
}

#endif /* ENABLE_NETWORK */
