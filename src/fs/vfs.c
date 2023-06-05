/*****************************************
		NanoShell Operating System
		  (C) 2022 iProgramInCpp

        Virtual File System module
******************************************/

/**
 * A short description of what this module is responsible for:
 *
 * This module is responsible for handling the file system.
 * Its root is "/".  Storage drives will be later on mounted
 * here, but they will use storabs.c's functionality.
 */
#include <vfs.h>
#include <string.h>
#include <memory.h>
#include <task.h>
#include <misc.h>

char g_cwd[PATH_MAX+2];

const char* FiGetCwd ()
{
	extern char g_cwd[PATH_MAX + 2];
	return g_cwd;
}

void FsAddReference(FileNode* pNode)
{
	//SLogMsg("FsAddReference(%p) -> %d", pNode, pNode->m_refCount + 1);
	if (pNode->m_refCount == NODE_IS_PERMANENT) return;
	
	pNode->m_refCount++;
}
void FsReleaseReference(FileNode* pNode)
{
	//SLogMsg("FsReleaseReference(%p) -> %d", pNode, pNode->m_refCount - 1);
	
	// if it's permanent, return
	if (pNode->m_refCount == NODE_IS_PERMANENT) return;
	
	ASSERT(pNode->m_refCount > 0);
	pNode->m_refCount--;
	
	if (pNode->m_refCount == 0)
	{
		if (pNode->OnUnreferenced)
			pNode->OnUnreferenced(pNode);
	}
}

int FsRead(FileNode* pNode, uint32_t offset, uint32_t size, void* pBuffer, bool block)
{
	if (!pNode)
		return ERR_INVALID_PARM;
	
	if (pNode->m_bHasDirCallbacks)
		return ERR_IS_DIRECTORY;
	
	if (!pNode->Read)
		return ERR_NOT_SUPPORTED;
	
	return pNode->Read(pNode, offset, size, pBuffer, block);
}

int FsWrite(FileNode* pNode, uint32_t offset, uint32_t size, const void* pBuffer, bool block)
{
	if (!pNode)
		return ERR_INVALID_PARM;
	
	if (pNode->m_bHasDirCallbacks)
		return ERR_IS_DIRECTORY;
	
	if (!pNode->Read)
		return ERR_NOT_SUPPORTED;
	
	return pNode->Write(pNode, offset, size, pBuffer, block);
}

int FsIoControl(FileNode* pNode, unsigned long request, void * argp)
{
	if (pNode)
	{
		if (!pNode->m_bHasDirCallbacks && pNode->IoControl)
		{
			return pNode->IoControl(pNode, request, argp);
		}
		else return -ENOTTY;
	}
	else return -ENOTTY;
}

int FsOpen(FileNode* pNode, bool read, bool write)
{
	if (!pNode)
		return ERR_INVALID_PARM;
	
	if (pNode->m_bHasDirCallbacks)
		return ERR_IS_DIRECTORY;
	
	// Assume we could open it.
	if (!pNode->Open)
		return ERR_SUCCESS;
	
	return pNode->Open(pNode, read, write);
}

int FsClose(FileNode* pNode)
{
	if (!pNode)
		return ERR_INVALID_PARM;
	
	if (pNode->m_bHasDirCallbacks)
		return ERR_IS_DIRECTORY;
	
	// Assume we could close it.
	if (!pNode->Close)
		return ERR_SUCCESS;
	
	return pNode->Close(pNode);
}

int FsReadDir(FileNode* pNode, uint32_t* index, DirEnt* pOutputDent)
{
	if (!pNode)
		return ERR_INVALID_PARM;
	
	if (!pNode->m_bHasDirCallbacks)
		return ERR_NOT_DIRECTORY;
	
	if (!pNode->ReadDir)
		return ERR_NOT_SUPPORTED;
	
	return pNode->ReadDir(pNode, index, pOutputDent);
}

int FsFindDir(FileNode* pNode, const char* pName, FileNode** pFN)
{
	*pFN = NULL;
	
	if (!pNode)
		return ERR_INVALID_PARM;
	
	if (!pNode->m_bHasDirCallbacks)
		return ERR_NOT_DIRECTORY;
	
	if (!pNode->FindDir)
		return ERR_NOT_SUPPORTED;
	
	return pNode->FindDir(pNode, pName, pFN);
}

int FsOpenDir(FileNode* pNode)
{
	if (!pNode)
		return ERR_INVALID_PARM;
	
	if (!pNode->m_bHasDirCallbacks)
		return ERR_NOT_DIRECTORY;
	
	// Assume we could open it.
	if (!pNode->OpenDir)
		return ERR_SUCCESS;
	
	return pNode->OpenDir(pNode);
}

int FsCloseDir(FileNode* pNode)
{
	if (!pNode)
		return ERR_INVALID_PARM;
	
	if (!pNode->m_bHasDirCallbacks)
		return ERR_NOT_DIRECTORY;
	
	// Assume we could close it.
	if (!pNode->CloseDir)
		return ERR_SUCCESS;
	
	return pNode->CloseDir(pNode);
}

int FsClearFile(FileNode* pNode)
{
	if (!pNode)
		return ERR_INVALID_PARM;
	
	if (pNode->m_bHasDirCallbacks)
		return ERR_NOT_DIRECTORY;
	
	if (!pNode->EmptyFile)
		return ERR_NOT_SUPPORTED;
	
	return pNode->EmptyFile(pNode);
}

int FsUnlinkFile(FileNode* pNode, const char* pName)
{
	if (!pNode) return -ENOENT;
	
	// if the directory we're trying to remove from isn't actually a directory
	if (!pNode->m_bHasDirCallbacks) return -ENOTDIR;
	
	// if there's no way to unlink a file
	if (!pNode->UnlinkFile) return -ENOTSUP;
	
	return pNode->UnlinkFile(pNode, pName);
}

int FsCreateEmptyFile(FileNode* pDirNode, const char* pFileName)
{
	if (!pDirNode) return -EIO;
	
	if (!pDirNode->m_bHasDirCallbacks) return -ENOTDIR;
	
	if (!pDirNode->CreateFile) return -ENOTSUP;
	
	return pDirNode->CreateFile(pDirNode, pFileName);
}

int FsCreateDir(FileNode* pDirNode, const char *pFileName)
{
	if (!pDirNode) return -EIO;
	if (!pDirNode->CreateDir) return -ENOTSUP;
	if (!pDirNode->m_bHasDirCallbacks) return -ENOTDIR;
	
	FileNode* pChildIfExists = NULL;
	
	int result = FsFindDir(pDirNode, pFileName, &pChildIfExists);
	if (FAILED(result) && result != -ENOENT)
	{
		return result;
	}
	
	if (pChildIfExists)
	{
		FsReleaseReference(pChildIfExists);
		return -EEXIST;
	}
	
	return pDirNode->CreateDir(pDirNode, pFileName);
}

// pBuf needs to be PATH_MAX or bigger in size.
int FsReadSymbolicLink(FileNode* pNode, char* pBuf)
{
	if (pNode->m_length >= PATH_MAX)
		return -ENAMETOOLONG;
	
	if (!pNode->Read)
		return ERR_NOT_SUPPORTED;
	
	for (int i = 0; i < (int)pNode->m_length; i++)
		pBuf[i] = 'A';
	
	int read = pNode->Read(pNode, 0, pNode->m_length, pBuf, true);
	
	if (read < 0)
		return read;
	
	if (FAILED(read))
		return ERR_IO_ERROR;
	
	pBuf[pNode->m_length] = 0;
	return 0;
}

// note: this does NO checks on pPath being longer than PATH_MAX, we assume it's already been done!
FileNode* FsResolvePathInternal(FileNode* pStartNode, const char* pPath, bool bResolveSymLinks, int nSymLinkDepth, int * errNo)
{
	*errNo = 0;
	
	if (!*pPath)
		return pStartNode;
	
	if (nSymLinkDepth >= C_MAX_SYMLINK_DEPTH)
	{
		*errNo = -ELOOP;
		return NULL;
	}
	
	char sCurrentFileName[PATH_MAX];
	const char *pPath2 = pPath;
	int index = 0;
	
	// since we will go through AT LEAST one iteration of the while loop (since the path isn't "nothing")
	// we need to add another reference to cancel out the one we would remove in the loop.
	FsAddReference(pStartNode);
	
	FileNode* pFileNode = pStartNode;
	
	while (true)
	{
		index = 0;
		while (*pPath2 != '/' && *pPath2 != '\0')
		{
			sCurrentFileName[index++] = *pPath2;
			pPath2++;
		}
		sCurrentFileName[index] = 0;
		
		bool bAtEnd = (*pPath2 == '\0'); //that, or it's at a '/'
		
		if (!bAtEnd)
		{
			pPath2++;
		}
		
		FileNode* pChild = NULL;
		
		int result = FsFindDir(pFileNode, sCurrentFileName, &pChild);
		if (FAILED(result))
		{
			FsReleaseReference(pFileNode);
			return NULL;
		}
		
		if (!pChild)
		{
			FsReleaseReference(pFileNode);
			return NULL;
		}
		
		// note. Currently, we keep the parent referenced, since we need to maybe resolve a symlink.
		// if we're a symbolic link, read it
		if (pChild->m_type == FILE_TYPE_SYMBOLIC_LINK && (!bAtEnd || bResolveSymLinks))
		{
			char slink[PATH_MAX];
			
			*errNo = FsReadSymbolicLink(pChild, slink);
			if (*errNo < 0)
			{
				// couldn't read. Unreference everything we know, and return.
				FsReleaseReference(pFileNode);
				FsReleaseReference(pChild);
				return NULL;
			}
			
			if (slink[0] == '/') // absolute symbolic link:
			{
				// start over from scratch.
				FsReleaseReference(pFileNode);
				FsReleaseReference(pChild);
				pFileNode = FsResolvePathInternal(FsGetRootNode(), slink, bResolveSymLinks, nSymLinkDepth + 1, errNo);
			}
			else // relative sym link
			{
				// release the child (the symlink), and start the lookup from our parent.
				// This is why kept it referenced.
				FsReleaseReference(pChild);
				FileNode* pNewNode = FsResolvePathInternal(pFileNode, slink, bResolveSymLinks, nSymLinkDepth + 1, errNo);
				FsReleaseReference(pFileNode);
				pFileNode = pNewNode;
			}
		}
		else
		{
			// dereference the parent, and update the pointer to the child.
			FsReleaseReference(pFileNode);
			pFileNode = pChild;
		}
		
		if (bAtEnd)
			return pFileNode;
	}
}

FileNode* FsResolvePath (const char* pPath, bool bResolveSymLinks)
{
	// ensure the path is long enough
	size_t pathLen = strlen(pPath);
	if (pathLen >= PATH_MAX)
		return NULL;
	
	char path[PATH_MAX];
	if (*pPath == '/')
	{
		strcpy(path, pPath + 1);
	}
	else
	{
		strcpy(path, FiGetCwd() + 1);
		
		size_t cwdLen = strlen(path);
		
		if (cwdLen + 2 + pathLen >= PATH_MAX)
			return NULL;
		
		if (*path != 0)
			strcat(path, "/");
		
		strcat(path, pPath);
	}
	
	UNUSED int errNo = 0;
	return FsResolvePathInternal(FsGetRootNode(), path, bResolveSymLinks, 0, &errNo);
}

// Default
#if 1
// Basic functions

void FsTempInit();

void FsInitializeDevicesDir();

//First time setup of the file manager
void FsInit ()
{
	strcpy(g_cwd, "/");
	FsTempInit();
	FsInitializeDevicesDir();
}

#endif

// File Descriptor handlers:
#if 1

FileDescriptor g_FileNodeToDescriptor[FD_MAX];
DirDescriptor  g_DirNodeToDescriptor [FD_MAX];

void FiPoolDebugDump(void);

void FiDebugDump()
{
	LogMsg("Listing opened files.");
	for (int i = 0; i < FD_MAX; i++)
	{
		FileDescriptor* p = &g_FileNodeToDescriptor[i];
		if (p->m_bOpen)
		{
			LogMsg("FD:%d\tFL:%d\tFS:%d\tP:%d FN:%x FC:%s:%d\tFN:%s",i,p->m_nFileEnd,p->m_nStreamOffset,
					p->m_bIsFIFO, p->m_pNode, p->m_openFile, p->m_openLine, p->m_sPath);
		}
	}
	LogMsg("Done");
}

static int FrFindFreeFileDescriptor(const char* reqPath)
{
	if (reqPath && *reqPath)
	{
		for (int i = 0; i < FD_MAX; i++)
		{
			if (g_FileNodeToDescriptor[i].m_bOpen)
				if (strcmp (g_FileNodeToDescriptor[i].m_sPath, reqPath) == 0)
					return -EAGAIN;
		}
	}
	
	for (int i = 0; i < FD_MAX; i++)
	{
		if (!g_FileNodeToDescriptor[i].m_bOpen)
			return i;
	}
	
	return -ENFILE;
}

static int FrFindFreeDirDescriptor(UNUSED const char* reqPath)
{
	for (int i = 0; i < FD_MAX; i++)
	{
		if (!g_DirNodeToDescriptor[i].m_bOpen)
			return i;
	}
	
	return -ENFILE;
}

bool FrIsValidDescriptor(int fd)
{
	if (fd < 0 || fd >= FD_MAX) return false;
	return g_FileNodeToDescriptor[fd].m_bOpen;
}

bool FrIsValidDirDescriptor(int fd)
{
	if (fd < 0 || fd >= FD_MAX) return false;
	return g_DirNodeToDescriptor[fd].m_bOpen;
}

int FrSeek (int fd, int offset, int whence);

int FrOpenInternal(const char* pFileName, FileNode* pFileNode, int oflag, const char* srcFile, int srcLine)
{
	// find a free fd to open:
	int fd = FrFindFreeFileDescriptor(pFileName);
	if (fd < 0)
		return fd;
	
	//find the node:
	bool hasClearedAlready = false;
	bool bPassedFileNodeDirectly = false;
	
	FileNode* pFile;
	
	if (pFileNode)
	{
		pFile = pFileNode;
		bPassedFileNodeDirectly = true;
	}
	else
	{
		pFile = FsResolvePath(pFileName, true);
		if (!pFile)
		{
			// Allow creation, if O_CREAT was specified and we need to write data
			if ((oflag & O_CREAT) && (oflag & O_WRONLY))
			{
				// Resolve the directory's name
				char fileName[strlen(pFileName) + 1];
				strcpy (fileName, pFileName);
				
				char* fileNameSimple = NULL;
				
				for (int i = strlen (pFileName); i >= 0; i--)
				{
					if (fileName[i] == '/')
					{
						fileName[i] = 0;
						fileNameSimple = fileName + i + 1;
						break;
					}
				}
				
				FileNode* pDir = FsResolvePath(fileName, false);
				if (!pDir)
				{
					SLogMsg("Warning: Couldn't even find parent dir '%s' ('%s')", fileName, pFileName);
					//couldn't even find parent dir
					return -ENOENT;
				}
				
				// this SHOULD be true...
				ASSERT(pDir->m_type & FILE_TYPE_DIRECTORY);
				
				// Try creating a file
				if (FsCreateEmptyFile (pDir, fileNameSimple) < 0)
					return -ENOSPC;
				
				int err = FsFindDir(pDir, fileNameSimple, &pFile);
				if (FAILED(err))
				{
					FsReleaseReference(pDir);
					return err;
				}
				
				FsReleaseReference(pDir);
				
				hasClearedAlready = true;
				
				if (!pFile)
					return -ENOSPC;
			}
			else
			{
				//Can't append to/read from a missing file!
				return -ENOENT;
			}
		}
		
		// if the file itself is a symlink (we asked it to resolve through sym links), simply return -ELOOP
		// to let the user know the symlink chain is too deep or some other issue happened
		if (pFile->m_type == FILE_TYPE_SYMBOLIC_LINK)
		{
			FsReleaseReference(pFile);
			
			return -ELOOP;
		}
	}
	
	//if we are trying to read, but we can't:
	if ((oflag & O_RDONLY) && !(pFile->m_perms & PERM_READ))
		return -EACCES;
	
	//if we are trying to write, but we can't:
	if ((oflag & O_WRONLY) && !(pFile->m_perms & PERM_WRITE))
		return -EACCES;
	
	//if we are trying to execute, but we can't:
	if ((oflag & O_EXEC) && !(pFile->m_perms & PERM_EXEC))
		return -EACCES;
	
	if (pFile->m_type & FILE_TYPE_DIRECTORY)
		return -EISDIR;
	
	//If we have should truncate the file:
	if ((oflag & O_WRONLY) && (oflag & O_TRUNC))
	{
		//If the filenode we opened isn't empty, empty it ourself
		if (!hasClearedAlready)
		{
			int result = FsClearFile(pFile);
			if (FAILED(result))
				return result;
		}
	}
	
	//open it:
	int res = FsOpen(pFile, (oflag & O_RDONLY) != 0, (oflag & O_WRONLY) != 0);
	if (res < 0)
		return res;
	
	//we have all the perms, let's write the filenode there:
	FileDescriptor *pDesc = &g_FileNodeToDescriptor[fd];
	
	if (pFileName)
		strcpy(pDesc->m_sPath, pFileName);
	else
		strcpy(pDesc->m_sPath, "");
	
	pDesc->m_bOpen 			= true;
	pDesc->m_pNode 			= pFile;
	pDesc->m_openFile	 	= srcFile;
	pDesc->m_openLine	 	= srcLine;
	pDesc->m_nStreamOffset 	= 0;
	pDesc->m_nFileEnd		= pFile->m_length;
	pDesc->m_bIsFIFO		= pFile->m_type == FILE_TYPE_CHAR_DEVICE || pFile->m_type == FILE_TYPE_PIPE;
	pDesc->m_bBlocking      = !(oflag & O_NONBLOCK);
	pDesc->m_ownerTask      = KeGetRunningTask();
	
	// if we passed the reference already, add 1 to the reference counter of this node.
	// otherwise, FsResolvePath or FsCreateFile have already done that.
	if (bPassedFileNodeDirectly)
	{
		FsAddReference(pFile);
	}
	
	if ((oflag & O_APPEND) && (oflag & O_WRONLY))
	{
		// Automatically seek to the end
		FrSeek(fd, SEEK_END, 0);
	}
	
	return fd;
}

int FrClose (int fd)
{
	if (!FrIsValidDescriptor(fd))
		return -EBADF;
	
	//closes the file:
	FileDescriptor *pDesc = &g_FileNodeToDescriptor[fd];
	pDesc->m_bOpen = false;
	strcpy(pDesc->m_sPath, "");
	
	FsClose(pDesc->m_pNode);
	
	FsReleaseReference(pDesc->m_pNode);
	
	pDesc->m_pNode = NULL;
	pDesc->m_nStreamOffset = 0;
	pDesc->m_ownerTask = NULL;
	
	return -ENOTHING;
}

int FrOpenDirD (const char* pFileName, const char* srcFile, int srcLine)
{
	// find a free fd to open:
	int dd = FrFindFreeDirDescriptor(pFileName);
	if (dd < 0)
		return dd;
	
	FileNode* pDir = FsResolvePath(pFileName, true);
	if (!pDir)
		// No File
		return -ENOENT;
	
	if (pDir->m_type == FILE_TYPE_SYMBOLIC_LINK)
	{
		FsReleaseReference(pDir);
		return -ELOOP;
	}
	
	if (!(pDir->m_type & FILE_TYPE_DIRECTORY))
	{
		FsReleaseReference(pDir);
		
		// Not a Directory
		return -ENOTDIR;
	}
	
	// Try to open the Directory
	int result = FsOpenDir(pDir);
	if (result < 0)
		return result; // Cannot open the directory
	
	//we have all the perms, let's write the filenode there:
	DirDescriptor *pDesc = &g_DirNodeToDescriptor[dd];
	pDesc->m_bOpen 			= true;
	strcpy(pDesc->m_sPath, pFileName);
	pDesc->m_pNode 			= pDir;
	pDesc->m_openFile	 	= srcFile;
	pDesc->m_openLine	 	= srcLine;
	pDesc->m_nStreamOffset 	= 0;
	pDesc->m_ownerTask      = KeGetRunningTask();
	
	return dd;
}

int FrCloseDir (int dd)
{
	if (!FrIsValidDirDescriptor(dd))
		return -EBADF;
	
	//closes the file:
	DirDescriptor *pDesc = &g_DirNodeToDescriptor[dd];
	pDesc->m_bOpen = false;
	strcpy(pDesc->m_sPath, "");
	
	FsCloseDir(pDesc->m_pNode);
	
	FsReleaseReference(pDesc->m_pNode);
	
	pDesc->m_pNode = NULL;
	pDesc->m_nStreamOffset = 0;
	pDesc->m_ownerTask     = NULL;
	
	return -ENOTHING;
}

DirEnt* FrReadDirLegacy(int dd)
{
	if (!FrIsValidDirDescriptor(dd))
		return NULL;
	
	DirDescriptor *pDesc = &g_DirNodeToDescriptor[dd];
	
	int errCode = FsReadDir (pDesc->m_pNode, &pDesc->m_nStreamOffset, &pDesc->m_sCurDirEnt);
	if (errCode != 0) // if we've reached a failure OR the end
	{
		// issue with this API is that there's no way to know what exactly failed.
		return NULL;
	}
	
	return &pDesc->m_sCurDirEnt;
}

int FrReadDir(DirEnt* pDirEnt, int dd)
{
	if (!FrIsValidDirDescriptor(dd))
		return ERR_BAD_FILE_DES;
	
	DirDescriptor *pDesc = &g_DirNodeToDescriptor[dd];
	
	return FsReadDir(pDesc->m_pNode, &pDesc->m_nStreamOffset, pDirEnt);
}

int FrSeekDir (int dd, int loc)
{
	if (!FrIsValidDirDescriptor(dd))
		return -EBADF;
	
	if (loc < 0)
		return -EOVERFLOW;
	
	DirDescriptor *pDesc = &g_DirNodeToDescriptor[dd];
	pDesc->m_nStreamOffset = loc;
	
	return -ENOTHING;
}

int FrTellDir (int dd)
{
	if (!FrIsValidDirDescriptor(dd))
		return -EBADF;
	
	DirDescriptor *pDesc = &g_DirNodeToDescriptor[dd];
	return pDesc->m_nStreamOffset;
}

int FrStatAt (int dd, const char *pFileName, StatResult* pOut)
{
	if (!FrIsValidDirDescriptor(dd))
		return -EBADF;
	
	DirDescriptor *pDesc = &g_DirNodeToDescriptor[dd];
	FileNode *pNode = NULL;
	
	int err = FsFindDir(pDesc->m_pNode, pFileName, &pNode);
	if (FAILED(err))
		return err;
	
	if (!pNode)
		return -ENOENT;
	
	pOut->m_type       = pNode->m_type;
	pOut->m_size       = pNode->m_length;
	pOut->m_inode      = pNode->m_inode;
	pOut->m_perms      = pNode->m_perms;
	pOut->m_modifyTime = pNode->m_modifyTime;
	pOut->m_createTime = pNode->m_createTime;
	pOut->m_blocks     = (pNode->m_length / 512) + ((pNode->m_length % 512) != 0);
	
	FsReleaseReference(pNode);
	return -ENOTHING;
}

int FrStat (const char *pFileName, StatResult* pOut)
{
	FileNode *pNode = FsResolvePath(pFileName, true);
	if (!pNode)
		return -ENOENT;
	
	pOut->m_type       = pNode->m_type;
	pOut->m_size       = pNode->m_length;
	pOut->m_inode      = pNode->m_inode;
	pOut->m_perms      = pNode->m_perms;
	pOut->m_modifyTime = pNode->m_modifyTime;
	pOut->m_createTime = pNode->m_createTime;
	pOut->m_blocks     = (pNode->m_length / 512) + ((pNode->m_length % 512) != 0);
	
	FsReleaseReference(pNode);
	return -ENOTHING;
}

int FrLinkStat (const char *pFileName, StatResult* pOut)
{
	FileNode *pNode = FsResolvePath(pFileName, false);
	if (!pNode)
		return -ENOENT;
	
	pOut->m_type       = pNode->m_type;
	pOut->m_size       = pNode->m_length;
	pOut->m_inode      = pNode->m_inode;
	pOut->m_perms      = pNode->m_perms;
	pOut->m_modifyTime = pNode->m_modifyTime;
	pOut->m_createTime = pNode->m_createTime;
	pOut->m_blocks     = (pNode->m_length / 512) + ((pNode->m_length % 512) != 0);
	
	FsReleaseReference(pNode);
	return -ENOTHING;
}

size_t FrRead (int fd, void *pBuf, int nBytes)
{
	if (!FrIsValidDescriptor(fd))
		return -EBADF;
	
	if (nBytes < 0)
		return -EINVAL;
	
	int rv = FsRead (g_FileNodeToDescriptor[fd].m_pNode, (uint32_t)g_FileNodeToDescriptor[fd].m_nStreamOffset, (uint32_t)nBytes, pBuf, g_FileNodeToDescriptor[fd].m_bBlocking);
	if (rv < 0) 
		return rv;
	
	g_FileNodeToDescriptor[fd].m_nStreamOffset += rv;
	return rv;
}

size_t FrWrite (int fd, void *pBuf, int nBytes)
{
	if (!FrIsValidDescriptor(fd))
		return -EBADF;
	
	if (nBytes < 0)
		return -EINVAL;
	
	int rv = FsWrite (g_FileNodeToDescriptor[fd].m_pNode, (uint32_t)g_FileNodeToDescriptor[fd].m_nStreamOffset, (uint32_t)nBytes, pBuf, g_FileNodeToDescriptor[fd].m_bBlocking);
	if (rv < 0)
		return rv;
	
	g_FileNodeToDescriptor[fd].m_nStreamOffset += rv;
	return rv;
}

int FrIoControl(int fd, unsigned long request, void * argp)
{
	if (!FrIsValidDescriptor(fd))
		return -EBADF;
	
	return FsIoControl(g_FileNodeToDescriptor[fd].m_pNode, request, argp);
}

int FrSeek (int fd, int offset, int whence)
{
	if (!FrIsValidDescriptor(fd))
		return -EBADF;
	
	if (g_FileNodeToDescriptor[fd].m_bIsFIFO)
		return -ESPIPE;
	
	if (whence < 0 || whence > SEEK_END)
		return -EINVAL;
	
	int realOffset = offset;
	
	switch (whence)
	{
		case SEEK_CUR:
			realOffset += g_FileNodeToDescriptor[fd].m_nStreamOffset;
			break;
		case SEEK_END:
			realOffset += g_FileNodeToDescriptor[fd].m_nFileEnd;
			break;
	}
	
	if (realOffset > g_FileNodeToDescriptor[fd].m_nFileEnd)
		return -EOVERFLOW;
	
	g_FileNodeToDescriptor[fd].m_nStreamOffset = realOffset;
	return -ENOTHING;
}

int FrTell (int fd)
{
	if (!FrIsValidDescriptor(fd))
		return -EBADF;
	
	return g_FileNodeToDescriptor[fd].m_nStreamOffset;
}

int FrTellSize (int fd)
{
	if (!FrIsValidDescriptor(fd))
		return -EBADF;
	
	return g_FileNodeToDescriptor[fd].m_nFileEnd;
}

extern char g_cwd[PATH_MAX+2];

int FrUnlinkInDir(const char* pDirName, const char* pFileName)
{
	FileNode *pDir = FsResolvePath (pDirName, false);
	if (!pDir)
		return -ENOENT;
	
	// note: The file node is as valid as there is a reference to it.
	// FsResolvePath adds a reference to the node, so it will only be invalid
	// when we release the reference.
	int errorCode = FsUnlinkFile (pDir, pFileName);
	FsReleaseReference(pDir);
	return errorCode;
}

int FrChangeDir(const char *pfn)
{
	FileNode *pNode = FsResolvePath(pfn, true);
	if (!pNode) return -ENOENT;
	
	if (pNode->m_type == FILE_TYPE_SYMBOLIC_LINK)
	{
		FsReleaseReference(pNode);
		return -ELOOP;
	}
	
	if (!(pNode->m_type & FILE_TYPE_DIRECTORY))
	{
		FsReleaseReference(pNode);
		return -ENOTDIR;
	}
	
	FsReleaseReference(pNode);
	
	//this should work!
	strcpy (g_cwd, pfn);
	
	return -ENOTHING;
}

//note: This only works with absolute paths that have been checked for length.
int FrRename(const char* pDirOld, const char* pNameOld, const char* pDirNew, const char* pNameNew)
{
	// resolve the directories
	FileNode* pDirNodeOld = FsResolvePath(pDirOld, true);
	
	if (!pDirNodeOld)
		return -ENOENT;
	
	FileNode* pDirNodeNew = FsResolvePath(pDirNew, true);
	
	if (!pDirNodeNew)
	{
		FsReleaseReference(pDirNodeOld);
		return -ENOENT;
	}
	
	if (pDirNodeOld->m_pFileSystemHandle != pDirNodeNew->m_pFileSystemHandle || pDirNodeOld->RenameOp != pDirNodeNew->RenameOp)
	{
		// No cross file system action allowed. Must use manual copy / delete combo.
		FsReleaseReference(pDirNodeOld);
		FsReleaseReference(pDirNodeNew);
		return -EXDEV;
	}
	
	if (!pDirNodeOld->m_bHasDirCallbacks || !pDirNodeNew->m_bHasDirCallbacks)
	{
		FsReleaseReference(pDirNodeOld);
		FsReleaseReference(pDirNodeNew);
		return -ENOTDIR;
	}
	
	if (!pDirNodeOld->RenameOp)
	{
		FsReleaseReference(pDirNodeOld);
		FsReleaseReference(pDirNodeNew);
		return -ENOTSUP;
	}
	
	// If the file already exists we must overwrite it. If we can't do that, simply bail.
	FileNode* pNodeWeWillOverwrite = NULL;
	
	int result = FsFindDir(pDirNodeNew, pNameNew, &pNodeWeWillOverwrite);
	
	// if we failed, and it wasn't because of a 'file not found' error:
	if (FAILED(result) && result != ERR_NO_FILE)
	{
		// bail right away
		FsReleaseReference(pDirNodeOld);
		FsReleaseReference(pDirNodeNew);
		return result;
	}
	
	if (pNodeWeWillOverwrite)
	{
		int result = -ENOTSUP; // can't overwrite the entry
		
		// If we have an unlink function in the new dir node:
		if (pDirNodeNew->UnlinkFile)
		{
			// Try to unlink this.
			result = pDirNodeNew->UnlinkFile(pDirNodeNew, pNameNew);
		}
		
		// if we couldn't
		if (result != -ENOTHING)
		{
			FsReleaseReference(pDirNodeOld);
			FsReleaseReference(pDirNodeNew);
			return result;
		}
		
		FsReleaseReference(pNodeWeWillOverwrite);
	}
	
	// okay, now, we should be able to just perform the rename operation
	result = pDirNodeOld->RenameOp(pDirNodeOld, pDirNodeNew, pNameOld, pNameNew);
	
	FsReleaseReference(pDirNodeOld);
	FsReleaseReference(pDirNodeNew);
	return result;
}

int FrMakeDir(const char* pDirName, const char* pFileName)
{
	FileNode *pNode = FsResolvePath(pDirName, false);
	if (!pNode) return -ENOENT;
	
	int status = FsCreateDir(pNode, pFileName);
	
	FsReleaseReference(pNode);
	return status;
}

int FrRemoveDir(const char* pPath)
{
	FileNode* pNode = FsResolvePath(pPath, false);
	if (!pNode)
		return -ENOENT;
	
	if (!pNode->RemoveDir)
		return -ENOTSUP;
	
	int status = pNode->RemoveDir(pNode);
	FsReleaseReference(pNode);
	return status;
}

void FrCloseAllFilesFromTask(void* task)
{
	// ok, the lock is now free, we should start closing files.
	for (int i = 0; i < (int)ARRAY_COUNT(g_FileNodeToDescriptor); i++)
	{
		if (g_FileNodeToDescriptor[i].m_ownerTask == task)
			FrClose(i);
	}
	for (int i = 0; i < (int)ARRAY_COUNT(g_DirNodeToDescriptor); i++)
	{
		if (g_DirNodeToDescriptor[i].m_ownerTask == task)
			FrCloseDir(i);
	}
}

#endif

static const char* ErrorStrings[] = {
	"Success",
	"Permission denied",
	"File exists",
	"Interrupted system call",
	"Invalid argument",
	"Input/output error",
	"Is a directory",
	"Symbolic link chain too deep",
	"Too many open files",
	"File or path name too long",
	"Too many open files in system",
	"No such file or directory",
	"Out of stream resources",
	"No space left on device",
	"Not a directory",
	"No such device or address",
	"Value is too large for defined data type",
	"Read only file system",
	"Already open",
	"Not enough memory",
	"Text file busy",
	"Bad file descriptor",
	"Illegal seek (is FIFO)",
	"Computer bought the farm",
	"Operation not supported",
	"Cross device operation not supported",
	"Resource is busy",
	"Directory is not empty",
	"Invalid input/output control request",
	"Range error",
	"Domain error",
};

STATIC_ASSERT(ARRAY_COUNT(ErrorStrings) == ECOUNT, "Change this if adding error codes.");

const char *GetErrNoString(int errno)
{
	errno = -errno;
	if (errno < 0)
		return "Unknown Error";
	if (errno >= ECOUNT)
		return "Unknown Error";
	
	return ErrorStrings[errno];
}

void FsUtilAddArbitraryFileNode(const char* pDirPath, const char* pFileName, FileNode* pSrcNode)
{
	FileNode* pFN = FsResolvePath(pDirPath, false);
	int failure = 0;
	if (!pFN)
	{
	_fail:
		SLogMsg("Couldn't FsUtilAddArbitraryFileNode('%s', '%s')! (failure ID: %d)", pDirPath, pFileName, failure);
		
		if (pFN)
			FsReleaseReference(pFN);
		
		return;
	}
	
	failure = pFN->CreateFile(pFN, pFileName);
	if (failure < 0)
		goto _fail;
	
	// it worked, look it up:
	FileNode* pNewNode = NULL;
	FsFindDir(pFN, pFileName, &pNewNode);
	
	if (!pNewNode)
	{
		failure = 1;
		goto _fail;
	}
	
	void* oldParentSpecific = pNewNode->m_pParentSpecific;
	
	// copy the new node's contents over the old one:
	*pNewNode = *pSrcNode;
	
	// make sure to pin it
	pNewNode->m_refCount = NODE_IS_PERMANENT;
	
	// restore the old "parent specific" stuff...
	pNewNode->m_pParentSpecific = oldParentSpecific;
	
	// unreference the parent.
	FsReleaseReference(pFN);
}
