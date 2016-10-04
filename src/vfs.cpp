/*
 * Copyright (c) 2006, Matt Whitlock and WhitSoft Development
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the names of Matt Whitlock and WhitSoft Development nor the
 *     names of their contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "vfs.h"
#include <algorithm>
#include "tree.cpp"
#include "mbcs_str.h"
#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>

VFS::VFS()
{
}

void VFS::Mount(const char *pszVirtual, const char *pszLocal)
// Creates a new mount point in the virtual file system.
{
	tree<MOUNTPOINT> *ptree, *pparent;

	ptree = &_root;
	size_t i = 0;
	string dir;
	while (pszVirtual[i] != 0) {
		++i;
		if (pszVirtual[i] == 0) break;
		size_t j = __mbscspn(pszVirtual + i, "/");
		dir.assign(pszVirtual + i, j);
		pparent = ptree;
		ptree = ptree->_pdown;
		while (ptree && __mbsicmp(ptree->_data.strVirtual.c_str(), dir.c_str())) ptree = ptree->_pright;
		if (!ptree) {
			ptree = new tree<MOUNTPOINT>(pparent);
			ptree->_data.strVirtual = dir;
		}
		i += j;
	}
	ptree->_data.strLocal = pszLocal;
}

DWORD VFS::GetDirectoryListing(const char *pszVirtual, DWORD dwIsNLST, listing_type &listing)
// Fills a map class with lines comprising an FTP-style directory listing.
// If dwIsNLST is non-zero, will return filenames only.
{
	char szLine[512];
	const char *pszMonthAbbr[]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
	LPVOID hFind;
	WIN32_FIND_DATA w32fd;
	SYSTEMTIME stCutoff, stFile;

	if (IsFolder(pszVirtual)) {
		string str;
		ResolveRelative(pszVirtual, "*", str);
		hFind = FindFirstFile(str.c_str(), &w32fd);
	}
	else {
		hFind = FindFirstFile(pszVirtual, &w32fd);
	}
	if (hFind) {
		GetSystemTime(&stCutoff);
		stCutoff.wYear--;
		do {
			if (!__mbscmp(w32fd.cFileName, ".") || !__mbscmp(w32fd.cFileName, "..")) continue;
			FileTimeToSystemTime(&w32fd.ftLastWriteTime, &stFile);
			if (dwIsNLST) {
				strcpy_s(szLine, w32fd.cFileName);
				if (w32fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					strcat_s(szLine, "/");
				}
			} else {
				wsprintf(szLine, "%c--------- 1 ftp ftp %10u %s %2u ", (w32fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? 'd' : '-', w32fd.nFileSizeLow, pszMonthAbbr[stFile.wMonth-1], stFile.wDay);
				if ((stFile.wYear > stCutoff.wYear) || ((stFile.wYear == stCutoff.wYear) && ((stFile.wMonth > stCutoff.wMonth) || ((stFile.wMonth == stCutoff.wMonth) && (stFile.wDay > stCutoff.wDay))))) {
					wsprintf(szLine + strlen(szLine), "%.2u:%.2u ", stFile.wHour, stFile.wMinute);
				} else {
					wsprintf(szLine + strlen(szLine), "%5u ", stFile.wYear);
				}
				strcat_s(szLine, w32fd.cFileName);
			}
			strcat_s(szLine,"\r\n");
			listing_type::iterator it = listing.find(w32fd.cFileName);
			if (it != listing.end()) {
				it->second = szLine;
			}
			else {
				listing.insert(std::make_pair(w32fd.cFileName, szLine));
			}
		} while (FindNextFile(hFind, &w32fd));
		FindClose(hFind);
		return 1;
	} else {
		return 0;
	}
}

DWORD VFS::Map(const char *pszVirtual, string &strLocal, tree<MOUNTPOINT> *ptree)
// Recursive function to map a virtual path to a local path.
{
	const char *psz;
	UINT_PTR dwLen;

	psz = __mbschr(pszVirtual, '/');
	if (psz) dwLen = psz - pszVirtual;
	else dwLen = __strlen(pszVirtual);
	while (ptree) {
		if ((ptree->_data.strVirtual.length() == dwLen) && (!dwLen || !__strnicmp(pszVirtual, ptree->_data.strVirtual.c_str(), dwLen))) {
			if (psz) {
				if (Map(__mbsinc(psz), strLocal, ptree->_pdown)) return 1;
				else {
					if (ptree->_data.strLocal.length() != 0) {
						strLocal = ptree->_data.strLocal;
						strLocal += psz;
						replace(strLocal.begin(), strLocal.end(), '/', '\\');
						return 1;
					} else {
						return 0;
					}
				}
			} else {
				strLocal = ptree->_data.strLocal;
				return 1;
			}
		} else {
			ptree = ptree->_pright;
		}
	}
	strLocal.clear();
	return 0;
} 

tree<VFS::MOUNTPOINT> * VFS::FindMountPoint(const char *pszVirtual, tree<MOUNTPOINT> *ptree)
// Returns a pointer to the tree node described by pszVirtual, or 0.
{
	const char *psz;
	UINT_PTR dwLen;

	if (!__mbscmp(pszVirtual, "/")) return ptree;
	psz = __mbschr(pszVirtual, '/');
	if (psz) dwLen = psz - pszVirtual;
	else dwLen = __strlen(pszVirtual);
	while (ptree) {
		if ((ptree->_data.strVirtual.length() == dwLen) && (!dwLen || !__strnicmp(pszVirtual, ptree->_data.strVirtual.c_str(), dwLen))) {
			if (psz) {
				return FindMountPoint(__mbsinc(psz), ptree->_pdown);
			} else {
				return ptree;
			}
		} else {
			ptree = ptree->_pright;
		}
	}
	return 0;
}

void VFS::CleanVirtualPath(const char *pszVirtual, string &strNewVirtual)
// Strips utter rubbish out of a virtual path.
// Ex: /home/./user//...\ftp/  =>  /home/ftp
{
	const char *in = pszVirtual;
	size_t bufSiz = strlen(pszVirtual) + 4;
	char *buf = new char[bufSiz];
	buf[0] = '\0'; buf[1] = '\0'; buf[2] = '\0';
	char *out = buf + 3;
	do {
		__mbsncpy_s(out, bufSiz - (out - buf), in, 1);
		if (__mbsnextc(out) == '\\') *out = '/'; // convert backslashes to forward slashes
		if ((__mbsnextc(out) == '\0') || (__mbsnextc(out) == '/')) {
			if (__mbsprevc(buf, out, 1) == '.') { // output ends with "."
				if (__mbsprevc(buf, out, 2) == '\0') out = __mbsdec(buf, out, 1); // entire output is "."
				else if (__mbsprevc(buf, out, 2) == '/') { // output ends with "/."
					if (__mbsprevc(buf, out, 3) == '\0') out = __mbsdec(buf, out, 1); // entire output is "/."
					else out  = __mbsdec(buf, out, 2);
				}
				else if (__mbsprevc(buf, out, 2) == '.') { // output ends with ".."
					if (__mbsprevc(buf, out, 3) == '\0') out = __mbsdec(buf, out, 2); // entire output is ".."
					else if (__mbsprevc(buf, out, 3) == '/') { // output ends with "/.."
						if (__mbsprevc(buf, out, 4) == '\0') out = __mbsdec(buf, out, 2); // entire output is "/.."
						else {
							out = __mbsdec(buf, out, 3);
							while ((__mbsprevc(buf, out, 1) != '\0') && (__mbsprevc(buf, out, 1) != '/')) out = __mbsdec(buf, out, 1);
						}
					}
				}
				else in = __mbsinc(in);
			}
			else {
				in = __mbsinc(in);
				if (__mbsprevc(buf, out, 1) != '/') out = __mbsinc(out);
			}
		}
		else in = __mbsinc(in), out = __mbsinc(out);
	} while (__mbsprevc(pszVirtual, in, 1) != '\0');
	strNewVirtual = buf + 3;
	delete[] buf;
}

void VFS::ResolveRelative(const char *pszCurrentVirtual, const char *pszRelativeVirtual, string &strNewVirtual)
// Concatenates pszRelativeVirtual to pszCurrentVirtual and resolves.
{
	if (*pszRelativeVirtual!='/') {
		strNewVirtual = pszCurrentVirtual;
		strNewVirtual += "/";
		strNewVirtual += pszRelativeVirtual;
		CleanVirtualPath(strNewVirtual.c_str(), strNewVirtual);
	}
	else {
		CleanVirtualPath(pszRelativeVirtual, strNewVirtual);
	}
}

bool VFS::WildcardMatch(const char *pszFilespec, const char *pszFilename)
// Returns true iff pszFilename matches wildcard pattern pszFilespec.
{
	if (*pszFilespec == 0) return true;
	while (*pszFilespec) {
		if (__mbsnextc(pszFilespec) == '*') {
			pszFilespec = __mbsinc(pszFilespec);
			do {
				if (WildcardMatch(pszFilespec, pszFilename)) return true;
				bool next = __mbsnextc(pszFilename) != 0;
				pszFilename = __mbsinc(pszFilename);
				if (!next) break;
			} while (true);
			return false;
		} else if (__mbctolower(__mbsnextc(pszFilespec)) != __mbctolower(__mbsnextc(pszFilename)) && (*pszFilespec != '?')) {
			return false;
		}
		pszFilespec = __mbsinc(pszFilespec);
		pszFilename = __mbsinc(pszFilename);
	}
	if (!*pszFilespec && !*pszFilename) return true;
	else return false;
}

LPVOID VFS::FindFirstFile(const char *pszVirtual, WIN32_FIND_DATA *pw32fd)
// Returns a find handle if a match was found. Otherwise returns 0.
// Supports wildcards.
{
	FINDDATA *pfd;
	const char *psz;
	string str;

	psz = __mbsrchr(pszVirtual, '/');
	if (psz == NULL) return NULL;
	str.assign(pszVirtual, psz);
	pfd = new FINDDATA;
	pfd->hFind = 0;
	pfd->strVirtual = pszVirtual;
	pfd->strFilespec = psz + 1;
	pfd->ptree = FindMountPoint(str.c_str(), &_root);
	if (pfd->ptree) pfd->ptree = pfd->ptree->_pdown;

	if (FindNextFile(pfd, pw32fd)) return pfd;
	else {
		delete pfd;
		return 0;
	}
}

bool VFS::FindNextFile(LPVOID lpFindHandle, WIN32_FIND_DATA *pw32fd)
{
	FINDDATA *pfd = (FINDDATA *)lpFindHandle;
	string str;

	while (pfd->ptree) {
		str = pfd->ptree->_data.strVirtual;
		if (__mbschr(str.c_str(), '.') == NULL) str.push_back('.');
		if (WildcardMatch(pfd->strFilespec.c_str(), str.c_str())) {
			GetMountPointFindData(pfd->ptree, pw32fd);
			pfd->ptree = pfd->ptree->_pright;
			return true;
		}
		pfd->ptree = pfd->ptree->_pright;
	}

	if (pfd->hFind) {
		return ::FindNextFile(pfd->hFind, pw32fd) ? true : false;
	} else {
		if (!Map(pfd->strVirtual.c_str(), str, &_root)) return false;
		if (str.length() != 0) {
			pfd->hFind = ::FindFirstFile(str.c_str(), pw32fd);
			return (pfd->hFind != INVALID_HANDLE_VALUE);
		} else {
			return false;
		}
	}
}

void VFS::FindClose(LPVOID lpFindHandle)
{
	FINDDATA *pfd = (FINDDATA *)lpFindHandle;

	if (pfd->hFind) ::FindClose(pfd->hFind);
	delete pfd;
}

bool VFS::FileExists(const char *pszVirtual)
// Returns true iff pszVirtual denotes an existing file or folder.
// Supports wildcards.
{
	LPVOID hFind;
	WIN32_FIND_DATA w32fd;

	hFind = FindFirstFile(pszVirtual, &w32fd);
	if (hFind) {
		FindClose(hFind);
		return true;
	} else {
		return false;
	}
}

bool VFS::IsFolder(const char *pszVirtual)
// Returns true iff pszVirtual denotes an existing folder.
// Does NOT support wildcards.
{
	string strLocal;
	DWORD dw;

	if (FindMountPoint(pszVirtual, &_root)) return true;
	if (!Map(pszVirtual, strLocal, &_root)) return true;
	dw = GetFileAttributes(strLocal.c_str());
	return ((dw != -1) && (dw & FILE_ATTRIBUTE_DIRECTORY));
}

void VFS::GetMountPointFindData(tree<MOUNTPOINT> *ptree, WIN32_FIND_DATA *pw32fd)
// Fills in the WIN32_FIND_DATA structure with data about the mount point.
{
	HANDLE hFind;
	SYSTEMTIME st = {1980, 1, 2, 1, 0, 0, 0, 0};

	if ((ptree->_data.strLocal.length() != 0) && ((hFind = ::FindFirstFile(ptree->_data.strLocal.c_str(), pw32fd)) != INVALID_HANDLE_VALUE)) {
		::FindClose(hFind);
	} else {
		memset(pw32fd, 0, sizeof(WIN32_FIND_DATA));
		pw32fd->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
		SystemTimeToFileTime(&st, &pw32fd->ftLastWriteTime);
	}
	strcpy_s(pw32fd->cFileName, sizeof(pw32fd->cFileName), ptree->_data.strVirtual.c_str());
}

HANDLE VFS::CreateFile(const char *pszVirtual, DWORD dwDesiredAccess, DWORD dwShareMode, DWORD dwCreationDisposition)
{
	string strLocal;

	if (Map(pszVirtual, strLocal, &_root)) {
		return ::CreateFile(strLocal.c_str(), dwDesiredAccess, dwShareMode, 0, dwCreationDisposition, FILE_FLAG_SEQUENTIAL_SCAN, 0);
	} else {
		return INVALID_HANDLE_VALUE;
	}
}

BOOL VFS::DeleteFile(const char *pszVirtual)
{
	string strLocal;

	return (Map(pszVirtual, strLocal, &_root) && ::DeleteFile(strLocal.c_str()));
}

BOOL VFS::MoveFile(const char *pszOldVirtual, const char *pszNewVirtual)
{
	string strOldLocal, strNewLocal;

	return (Map(pszOldVirtual, strOldLocal, &_root) && Map(pszNewVirtual, strNewLocal, &_root) && ::MoveFile(strOldLocal.c_str(), strNewLocal.c_str()));
}

BOOL VFS::CreateDirectory(const char *pszVirtual)
{
	string strLocal;

	return (Map(pszVirtual, strLocal, &_root) && ::CreateDirectory(strLocal.c_str(), NULL));
}

BOOL VFS::RemoveDirectory(const char *pszVirtual)
{
	string strLocal;

	return (Map(pszVirtual, strLocal, &_root) && ::RemoveDirectory(strLocal.c_str()));
}