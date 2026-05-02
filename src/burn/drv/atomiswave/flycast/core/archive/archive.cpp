/*
    Created on: Nov 23, 2018

	Copyright 2018 flyinghead

	This file is part of reicast.

    reicast is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    reicast is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with reicast.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "archive.h"
#include "7zArchive.h"
#include "ZipArchive.h"
#include <algorithm>
#include <cstring>
#if !defined(_WIN32)
#include <strings.h>
#endif



extern "C" {
	int NaomiDirectArchiveMatch(const char* path);
	int NaomiDirectArchiveEntryCount();
	const char* NaomiDirectArchiveEntryName(int index);
	unsigned int NaomiDirectArchiveEntryCrc(int index);
	unsigned int NaomiDirectArchiveRead(int index, unsigned int offset, void* buffer, unsigned int length);
}

static int fbneo_strcasecmp(const char* a, const char* b)
{
#if defined(_WIN32)
	return _stricmp(a, b);
#else
	return strcasecmp(a, b);
#endif
}

class FbneoArchiveFile : public ArchiveFile
{
public:
	FbneoArchiveFile(int index) : index(index), offset(0) {}
	virtual u32 Read(void* buffer, u32 length) override
	{
		u32 read = (u32)NaomiDirectArchiveRead(index, offset, buffer, length);
		offset += read;
		return read;
	}
private:
	int index;
	u32 offset;
};

class FbneoArchive : public Archive
{
public:
	virtual ArchiveFile* OpenFile(const char* name) override
	{
		if (name == NULL)
			return NULL;
		const int count = NaomiDirectArchiveEntryCount();
		for (int i = 0; i < count; i++)
		{
			const char* entryName = NaomiDirectArchiveEntryName(i);
			if (entryName != NULL && fbneo_strcasecmp(entryName, name) == 0)
				return new FbneoArchiveFile(i);
		}
		return NULL;
	}

	virtual ArchiveFile* OpenFileByCrc(u32 crc) override
	{
		if (crc == 0)
			return NULL;
		const int count = NaomiDirectArchiveEntryCount();
		for (int i = 0; i < count; i++)
		{
			if ((u32)NaomiDirectArchiveEntryCrc(i) == crc)
				return new FbneoArchiveFile(i);
		}
		return NULL;
	}

private:
	virtual bool Open(const char* path) override
	{
		return NaomiDirectArchiveMatch(path) != 0;
	}
};
Archive *OpenArchive(const char *path)
{
	Archive *fbneo_archive = new FbneoArchive();
	if (fbneo_archive->Open(path))
		return fbneo_archive;
	delete fbneo_archive;

	std::string base_path(path);

	Archive *sz_archive = new SzArchive();
	if (sz_archive->Open(base_path.c_str()) || sz_archive->Open((base_path + ".7z").c_str()) || sz_archive->Open((base_path + ".7Z").c_str()))
		return sz_archive;
	delete sz_archive;

	Archive *zip_archive = new ZipArchive();
	if (zip_archive->Open(base_path.c_str()) || zip_archive->Open((base_path + ".zip").c_str()) || zip_archive->Open((base_path + ".ZIP").c_str()))
		return zip_archive;
	delete zip_archive;

	INFO_LOG(COMMON, "OpenArchive: failed to open %s", path);
	return NULL;
}




