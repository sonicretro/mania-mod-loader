/**
 * SADX Mod Loader
 * File remapper.
 */

#include "stdafx.h"
#include "FileMap.hpp"
#include "include\ManiaModLoader.h"

#include <cctype>
#include <cstring>
#include <algorithm>

using std::list;
using std::string;
using std::transform;
using std::unordered_map;

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlwapi.h>

FileMap::FileMap()
{ }

FileMap::~FileMap()
{ }

/**
 * Replace slash characters with backslashes.
 * @param c Character.
 * @return If c == '/', '\\'; otherwise, c.
 */
static inline int backslashes(int c)
{
	if (c == '/')
		return '\\';
	else
		return c;
}

/**
 * Normalize a filename for the file replacement map.
 * @param filename Filename.
 * @return Normalized filename.
 */
string FileMap::normalizePath(const string &filename)
{
	return normalizePath(filename.c_str());
}

/**
 * Normalize a filename for the file replacement map.
 * @param filename Filename.
 * @return Normalized filename.
 */
string FileMap::normalizePath(const char *filename)
{
	string path = filename;
	transform(path.begin(), path.end(), path.begin(), backslashes);
	if (path.length() > 2 && (path[0] == '.' && path[1] == '\\'))
		path = path.substr(2, path.length() - 2);
	transform(path.begin(), path.end(), path.begin(), ::tolower);
	return path;
}

/**
 * Ignore a file.
 * @param ignoreFile File to ignore.
 * @param modIdx Index of the current mod.
 */
void FileMap::addIgnoreFile(const string &ignoreFile, int modIdx)
{
	string path = normalizePath(ignoreFile);
	m_fileMap[path] = { "nullfile", modIdx };
}

/**
 * Add a file replacement.
 * @param origFile Original filename.
 * @param modFile Mod filename.
 */
void FileMap::addReplaceFile(const std::string &origFile, const std::string &modFile)
{
	string origFile_norm = normalizePath(origFile);
	string modFile_norm = normalizePath(modFile);

	// Check if the destination file is being replaced.
	auto iter = m_fileMap.find(modFile_norm);
	if (iter != m_fileMap.end())
	{
		// Destination file is being replaced.
		// Use the replacement for the original file.
		setReplaceFile(origFile_norm, iter->second.fileName, iter->second.modIndex);
	}
	else
	{
		// Destination file is not already in the map.
		setReplaceFile(origFile_norm, modFile_norm, 0);
	}
}

/**
 * Recursively scan a directory and add all files to the replacement map.
 * Destination is always relative to system/.
 * @param srcPath Path to scan.
 * @param modIdx Index of the current mod.
 */
void FileMap::scanFolder(const string &srcPath, int modIdx)
{
	scanFolder_int(srcPath, srcPath.length() + 1, modIdx);
}

/**
 * Recursively scan a directory and add all files to the replacement map.
 * Destination is always relative to system/.
 * (Internal recursive function)
 * @param srcPath Path to scan.
 * @param srcLen Length of original srcPath. (used for recursion)
 * @param modIdx Index of the current mod.
 */
void FileMap::scanFolder_int(const string &srcPath, int srcLen, int modIdx)
{
	WIN32_FIND_DATAA data;
	char path[MAX_PATH];
	snprintf(path, sizeof(path), "%s\\*", srcPath.c_str());
	HANDLE hFind = FindFirstFileA(path, &data);

	// No files found.
	if (hFind == INVALID_HANDLE_VALUE)
	{
		return;
	}

	do
	{
		// NOTE: This will hide *all* files starting with '.'.
		// SADX doesn't use any files starting with '.',
		// so this won't cause any problems.
		if (data.cFileName[0] == '.')
		{
			continue;
		}

		if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			// Recursively scan this directory.
			const string newSrcPath = srcPath + '\\' + string(data.cFileName);
			scanFolder_int(newSrcPath, srcLen, modIdx);
		}
		else
		{
			// Create the mod filename and original filename.
			string modFile = srcPath + '\\' + string(data.cFileName);
			transform(modFile.begin(), modFile.end(), modFile.begin(), ::tolower);

			// Original filename.
			string origFile = "data\\" + modFile.substr(srcLen);

			setReplaceFile(origFile, modFile, modIdx);
		}
	} while (FindNextFileA(hFind, &data) != 0);

	FindClose(hFind);
}

/**
 * Set a replacement file in the map.
 * Filenames must already be normalized!
 * (Internal function; handles memory allocation)
 * @param origFile Original file.
 * @param destFile Replacement filename.
 * @param modIdx Index of the current mod.
 */
void FileMap::setReplaceFile(const std::string &origFile, const std::string &destFile, int modIdx)
{
	// Update the main map.
	m_fileMap[origFile] = { destFile, modIdx };

}

/**
 * Get a filename from the file replacement map.
 * @param lpFileName Filename.
 * @return Replaced filename, or original filename if not replaced by a mod.
 */
const char *FileMap::replaceFile(const char *lpFileName) const
{
	// Check if the normalized filename is in the file replacement map.
	string path = normalizePath(lpFileName);
	unordered_map<string, Entry>::const_iterator iter = m_fileMap.find(path);
	if (iter != m_fileMap.cend())
	{
		const string &newFileName = iter->second.fileName;
		return newFileName.c_str();
	}

	// File was not replaced by a mod.
	// Return the filename as-is.
	return lpFileName;
}

/**
 * Get the index of the mod that replaced a given file.
 * @param lpFileName Filename.
 * @return Index of the mod that replaced a file, or 0 if no mod replaced it.
 */
int FileMap::getModIndex(const char *lpFileName) const
{
	// Check if the normalized filename is in the file replacement map.
	string path = normalizePath(lpFileName);
	unordered_map<string, Entry>::const_iterator iter = m_fileMap.find(path);
	if (iter != m_fileMap.cend())
	{
		return iter->second.modIndex;
	}

	// File was not replaced by a mod.
	return 0;
}

/**
 * Clear the file replacement map.
 */
void FileMap::clear(void)
{
	m_fileMap.clear();
}
