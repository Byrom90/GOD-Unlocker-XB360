#include <xtl.h>
#include "AtgConsole.h"
#include "AtgUtil.h"
#include "AtgInput.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <fstream>
#include <sys/stat.h> 
#include "xfilecache.h"
#include <string>
#include "xbox.h"
#include <vector>
#include "mount.h"

using std::vector;
using std::string;
using std::ifstream;
using std::ofstream;
using std::fstream;

//class GOD {
class GOD {
private:
	void readTitle() 
	{
		string fullPath = path + fileName;
		ifstream file;
		file.open ((char*)fullPath.c_str(), ifstream::in);
		if (file.is_open())
		{
			char buffer[_MAX_PATH];
			file.seekg(0x00000411);
			file.read(buffer,_MAX_PATH);
			swprintf_s(title, _MAX_PATH, L"%s", buffer);
			file.close();
		}
	}
public:
		string fileName;
		string path;
		wchar_t title[_MAX_PATH];
		//NXE (string, string);
		GOD(string, string);
		bool status;	
};

string filePathzzz = "\\Content\\0000000000000000"; // Byrom - Temp change to dummy directory
//string filePathzzz = "\\Dummy-GOD-Content";
int fileCount = 0;
ATG::Console console;
bool debuglogexists = false;

extern "C" VOID XeCryptSha(LPVOID DataBuffer1, UINT DataSize1, LPVOID DataBuffer2, UINT DataSize2, LPVOID DataBuffer3, UINT DataSize3, LPVOID DigestBuffer, UINT DigestSize);

//GOD::GOD (string strFileName, string strPath) {
GOD::GOD(string strFileName, string strPath) {
	fileName = strFileName;
	path = strPath;
	readTitle();
	status = true;
}

//vector<NXE> allNXE;
vector<GOD> allGODRegular;
vector<GOD> allGODMSPSpoofed;
vector<GOD> allGODBundle;
vector<GOD> allGODUnlocked;
vector<string> devices;

int DeleteDirectory(const std::string &refcstrRootDirectory, bool bDeleteSubdirectories = true)
{
  bool            bSubdirectory = false;       // Flag, indicating whether
                                               // subdirectories have been found
  HANDLE          hFile;                       // Handle to directory
  std::string     strFilePath;                 // Filepath
  std::string     strPattern;                  // Pattern
  WIN32_FIND_DATA FileInformation;             // File information


  strPattern = refcstrRootDirectory + "\\*.*";
  hFile = ::FindFirstFile(strPattern.c_str(), &FileInformation);
  if(hFile != INVALID_HANDLE_VALUE)
  {
    do
    {
      if(FileInformation.cFileName[0] != '.')
      {
        strFilePath = refcstrRootDirectory + "\\" + FileInformation.cFileName;

        if(FileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
          if(bDeleteSubdirectories)
          {
            // Delete subdirectory
            int iRC = DeleteDirectory(strFilePath, bDeleteSubdirectories);
            if(iRC)
              return iRC;
          }
          else
            bSubdirectory = true;
        }
        else
        {
          // Set file attributes
          if(::SetFileAttributes(strFilePath.c_str(), FILE_ATTRIBUTE_NORMAL) == FALSE)
            return ::GetLastError();

          // Delete file
          if(::DeleteFile(strFilePath.c_str()) == FALSE)
			  return ::GetLastError();
        }
      }
    } while(::FindNextFile(hFile, &FileInformation) == TRUE);

    // Close handle
    ::FindClose(hFile);

    DWORD dwError = ::GetLastError();
    if(dwError != ERROR_NO_MORE_FILES)
      return dwError;
    else
    {
      if(!bSubdirectory)
      {
        // Set directory attributes
        if(::SetFileAttributes(refcstrRootDirectory.c_str(), FILE_ATTRIBUTE_NORMAL) == FALSE)
          return ::GetLastError();

        // Delete directory
        if(::RemoveDirectory(refcstrRootDirectory.c_str()) == FALSE)
          return ::GetLastError();
      }
    }
  }
  return 0;
}

bool FileExists(const char* strFilename) 
{
  struct stat tmp;
  if(stat(strFilename, &tmp) == 0)
    return true;
  return false;
}

void debugLog(char* output)
{
	FILE* fd;
	if (fopen_s(&fd, "game:\\debug.log", "ab") == 0)
	{
		fwrite(output, strlen(output), 1, fd);
		fprintf(fd, "\r\n");
		fclose(fd);
	}
}

void genlog()
{
	if (FileExists("game:\\debug.log"))
		return;
	FILE* fd;
	if (fopen_s(&fd, "game:\\debug.log", "w") == 0)
		fclose(fd);
}

unsigned char HeaderMain[] = {
	0x4C, 0x49, 0x56, 0x45, 0x42, 0x79, 0x72, 0x6F, 0x6D, 0x57,
	0x61, 0x73, 0x48, 0x65, 0x72, 0x65, 0x55, 0x6E, 0x6C, 0x6F,
	0x63, 0x6B, 0x69, 0x6E, 0x67, 0x59, 0x6F, 0x75, 0x72, 0x47,
	0x4F, 0x44, 0x47, 0x61, 0x6D, 0x65, 0x42, 0x79, 0x72, 0x6F,
	0x6D, 0x57, 0x61, 0x73, 0x48, 0x65, 0x72, 0x65, 0x55, 0x6E,
	0x6C, 0x6F, 0x63, 0x6B, 0x69, 0x6E, 0x67, 0x59, 0x6F, 0x75,
	0x72, 0x47, 0x4F, 0x44, 0x47, 0x61, 0x6D, 0x65, 0x42, 0x79,
	0x72, 0x6F, 0x6D, 0x57, 0x61, 0x73, 0x48, 0x65, 0x72, 0x65,
	0x55, 0x6E, 0x6C, 0x6F, 0x63, 0x6B, 0x69, 0x6E, 0x67, 0x59,
	0x6F, 0x75, 0x72, 0x47, 0x4F, 0x44, 0x47, 0x61, 0x6D, 0x65,
	0x42, 0x79, 0x72, 0x6F, 0x6D, 0x57, 0x61, 0x73, 0x48, 0x65,
	0x72, 0x65, 0x55, 0x6E, 0x6C, 0x6F, 0x63, 0x6B, 0x69, 0x6E,
	0x67, 0x59, 0x6F, 0x75, 0x72, 0x47, 0x4F, 0x44, 0x47, 0x61,
	0x6D, 0x65, 0x42, 0x79, 0x72, 0x6F, 0x6D, 0x57, 0x61, 0x73,
	0x48, 0x65, 0x72, 0x65, 0x55, 0x6E, 0x6C, 0x6F, 0x63, 0x6B,
	0x69, 0x6E, 0x67, 0x59, 0x6F, 0x75, 0x72, 0x47, 0x4F, 0x44,
	0x47, 0x61, 0x6D, 0x65, 0x42, 0x79, 0x72, 0x6F, 0x6D, 0x57,
	0x61, 0x73, 0x48, 0x65, 0x72, 0x65, 0x55, 0x6E, 0x6C, 0x6F,
	0x63, 0x6B, 0x69, 0x6E, 0x67, 0x59, 0x6F, 0x75, 0x72, 0x47,
	0x4F, 0x44, 0x47, 0x61, 0x6D, 0x65, 0x42, 0x79, 0x72, 0x6F,
	0x6D, 0x57, 0x61, 0x73, 0x48, 0x65, 0x72, 0x65, 0x55, 0x6E,
	0x6C, 0x6F, 0x63, 0x6B, 0x69, 0x6E, 0x67, 0x59, 0x6F, 0x75,
	0x72, 0x47, 0x4F, 0x44, 0x47, 0x61, 0x6D, 0x65, 0x42, 0x79,
	0x72, 0x6F, 0x6D, 0x57, 0x61, 0x73, 0x48, 0x65, 0x72, 0x65,
	0x55, 0x6E, 0x6C, 0x6F, 0x63, 0x6B, 0x69, 0x6E, 0x67, 0x59,
	0x6F, 0x75, 0x72, 0x47, 0x4F, 0x44, 0x47, 0x61, 0x6D, 0x65
};

unsigned char LicenseInfo[] = {
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00,
0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x01
};

bool DoNotSetLicense = false; // Enable when unlocking bundles

bool UnlockMe(const char* file)
{
	unsigned char* buffer;
	int size = 0;
	FILE* fd;
	if (fopen_s(&fd, file, "rb") == 0)
	{
		fseek(fd, 0, SEEK_END);
		size = ftell(fd);
		buffer = new unsigned char[size];
		fseek(fd, 0, SEEK_SET);
		if (fread(buffer, size, 1, fd) == 0)
		{
			fclose(fd);
			console.Format("Failed 1\n");
			return false;
		}
		fclose(fd);
	}
	else
	{
		console.Format("Failed 2\n");
		return false;
	}

	if (size < 0x413)
	{
		console.Format("Failed 3\n");
		return false;
	}
	// Add LIVE file header and custom package signature
	int i;
	for (i = 0x0; i < sizeof(HeaderMain); i++)
	{
		buffer[i] = HeaderMain[i];
	}
	// Wipe the license section completely
	for (i = 0x22C; i < 0x32C; i++)
	{
		buffer[i] = 0x0;
	}
	// Add our license info
	if (!DoNotSetLicense)
	{
		int bytecounter = 0;
		for (i = 0x22C; i < (0x22C + sizeof(LicenseInfo)); i++)
		{
			buffer[i] = LicenseInfo[bytecounter];
			bytecounter++;
		}
	}
	// Open the original file and write our new header data
	if (fopen_s(&fd, file, "wb") == 0)
	{
		fwrite(buffer, size, 1, fd);
		fclose(fd);
		return true;
	}
	console.Format("Failed 4\n");
	return false;
}

void UnlockGOD(GOD godGame)
{
	console.Format("Unlocking %ls...\n", godGame.title);

	string goddirectory = godGame.path;

	string godFile = godGame.path + godGame.fileName;

	string godBackupDir = goddirectory + "BACKUP";

	::CreateDirectory(godBackupDir.c_str(), 0);
	string godFileBACKUP = godBackupDir + "\\" + godGame.fileName;
		//if(::DeleteFile(godFileBACKUP.c_str()))
		//	console.Format("Backup deleted successfully!\n");
	//console.Format("Attempting to create backup file\n%s...\n", godFileBACKUP.c_str());
	if (::CopyFile(godFile.c_str(), godFileBACKUP.c_str(), false))
	{
		console.Format("Backup created successfully!\n");
		// Unlock GOD
		if (UnlockMe(godFile.c_str()) != true)
		{
			genlog();
			console.Format("FAILED\n");
			debugLog("Unable to open new Games On Demand Live file at:");
			debugLog((char*)goddirectory.c_str());
			godGame.status = false;
		}

		if (godGame.status)
		{
			debugLog("Unlocked: ");
			debugLog((char*)goddirectory.c_str());
			console.Format("Success!\n");
		}
	}
	else
		console.Format("Failed to create backup of original file! Aborting...\n");
	
}

int isGOD(WCHAR* path, string path2)
{
	int Result = 0;

	if (!path2.compare(path2.length() - 9, 8, "00007000") == 0)
		return Result;

	FILE* fd;
	if (_wfopen_s(&fd, path, L"rb") == 0)
	{
		// Confirm it's a GOD content package
		unsigned char buf[4];
		unsigned char GODID[] = { 0, 0, 0x70, 0 };
		fseek(fd, 0x344, SEEK_SET);
		fread(buf, sizeof(buf), 1, fd);
		if (memcmp(buf, GODID, sizeof(GODID)) != 0) // It's not a GOD package so we close it and return
		{
			fclose(fd);
			return Result;
		}

		// We know it's a GOD at this point so no we determine if it's already unlocked and which header it has

		// Lock state check. We replace the package signature with ByromWasHereUnlockingYourGODGame repeatedly when unlocking
		// Package signature is ignored on modified consoles.
		unsigned char buf2[4];
		unsigned char ByromHeader[] = { 0x42, 0x79, 0x72, 0x6F }; // B y r o
		fseek(fd, 4, SEEK_SET);
		fread(buf2, sizeof(buf2), 1, fd);
		if (memcmp(buf2, ByromHeader, sizeof(ByromHeader)) == 0) // If it's already unlocked we set Result to 3
		{
			Result = 3;
		}
		else
		{
			// Header type check
			unsigned char buf3[4];
			unsigned char LIVEHeader[] = { 0x4C, 0x49, 0x56, 0x45 }; // L I V E
			unsigned char PIRSHeader[] = { 0x50, 0x49, 0x52, 0x53 }; // P I R S
			fseek(fd, 0, SEEK_SET);
			fread(buf3, sizeof(buf3), 1, fd);
			if (memcmp(buf3, LIVEHeader, sizeof(LIVEHeader)) == 0) // Check it has LIVE header
				Result = 1; // We set Result to 1
			else if (memcmp(buf3, PIRSHeader, sizeof(PIRSHeader)) == 0) // Check it has PIRS header
				Result = 2; // We set Result to 2

			// Check if it's one of the game bundles. These don't work correctly when license info is set
			unsigned char buf4[4];
			unsigned char DestinyBundleTID[] = { 0x41, 0x56, 0x09, 0x28 }; // 41560928 - Collector's Edition
			unsigned char BO3BundleTID[] = { 0x41, 0x56, 0x09, 0x29 }; // 41560929 - Black Ops III Bundle
			unsigned char CODMWBundleTID[] = { 0x41, 0x56, 0x09, 0x31 }; // 41560931 - COD: MW Bundle
			unsigned char PrototypeBundleTID[] = { 0x41, 0x56, 0x09, 0x2D }; // 4156092D - Prototype® Bio Bundle
			unsigned char FableBundleTID[] = { 0x4D, 0x53, 0x0A, 0xA2 }; // 4D530AA2 - Fable Trilogy
			fseek(fd, 0x360, SEEK_SET);
			fread(buf4, sizeof(buf4), 1, fd);
			// Compare the title id to see if it's one of the bundles
			if (memcmp(buf4, DestinyBundleTID, sizeof(DestinyBundleTID)) == 0 || memcmp(buf4, BO3BundleTID, sizeof(BO3BundleTID)) == 0
				|| memcmp(buf4, CODMWBundleTID, sizeof(CODMWBundleTID)) == 0 || memcmp(buf4, PrototypeBundleTID, sizeof(PrototypeBundleTID)) == 0
				|| memcmp(buf4, FableBundleTID, sizeof(FableBundleTID)) == 0)
				Result = 4; // We set Result to 4

			// If it's none of the above we just do nothing (Result will remain 0). Header must be corrupted or something for this
		}

		fclose(fd); // Close the file
	}
	return Result; // Finally return the Result
}

HRESULT ScanDir(string strFind)
{
	HANDLE hFind;
	WIN32_FIND_DATA wfd;
	LPSTR lpPath = new char[MAX_PATH];
	LPWSTR lpPathW = new wchar_t[MAX_PATH];
	LPSTR lpFileName = new char[MAX_PATH];
	LPWSTR lpFileNameW = new wchar_t[MAX_PATH];
	string sFileName;
	strFind += "\\*";
	strFind._Copy_s(lpPath, strFind.length(),strFind.length());
	lpPath[strFind.length()]='\0';
	::MultiByteToWideChar( CP_ACP, NULL,lpPath, -1, lpPathW, MAX_PATH);
	hFind = FindFirstFile( lpPath, &wfd );
	int nIndex = 0;
	if(INVALID_HANDLE_VALUE == hFind)
	{
		//debugLog("Invalid handle type - Directory most likely empty");
		//debugLog(lpPath);
		//debugLog(lpFileName);
	}
	else
	{
		nIndex = 0;
		do
		{
			sFileName = wfd.cFileName;
			sFileName._Copy_s(lpFileName, sFileName.length(), sFileName.length());
			lpFileName[sFileName.length()]='\0';
			::MultiByteToWideChar( CP_ACP, NULL,lpFileName, -1, lpFileNameW, MAX_PATH);
			if(FILE_ATTRIBUTE_DIRECTORY == wfd.dwFileAttributes)
			{
				string nextDir;
				nextDir += lpPath;
				string nextDir1;
				nextDir1 += lpFileName;
				nextDir.erase(nextDir.size() - 1, 1);
				nextDir += nextDir1;
				ScanDir(nextDir);
			}
			else
			{
				string filePath;
				string filePathX;
				string fileNameX;
				filePath += lpPath;
				string fileName;
				fileName += lpFileName;
				filePath.erase(filePath.size() - 1, 1);
				filePathX += filePath;
				fileNameX += fileName;
				filePath += fileName;
				LPSTR FileA = new char[MAX_PATH];
				LPSTR FileNameA = new char[MAX_PATH];
				LPSTR FilePathA = new char[MAX_PATH];
				filePathX._Copy_s(FilePathA, filePathX.length(),filePathX.length());
				fileNameX._Copy_s(FileNameA , fileNameX.length(),fileNameX.length());
				filePath._Copy_s(FileA, filePath.length(),filePath.length());
				FileA[filePath.length()]='\0';
				LPWSTR FileB = new wchar_t[MAX_PATH];
				::MultiByteToWideChar(CP_ACP, NULL, FileA, -1, FileB, MAX_PATH);

				int GODType = isGOD(FileB, filePathX);
				if (GODType == 1) // Regular GOD file downloaded officially or created with nxe2god (LIVE header)
				{
					//NXE temp(fileNameX,filePathX);
					//allNXE.push_back(temp);
					GOD temp(fileNameX, filePathX);
					allGODRegular.push_back(temp);
				}
				else if (GODType == 2) // MSP Spoofed GOD file (PIRS header). Will fix these to have a live header
				{
					GOD temp(fileNameX, filePathX);
					allGODMSPSpoofed.push_back(temp);
				}
				else if (GODType == 3) // Already unlocked GOD file (Either format). Will just print how many of these were found
				{
					GOD temp(fileNameX, filePathX);
					allGODUnlocked.push_back(temp);
				}
				else if (GODType == 4) // Game bundle downloader. These load but give an error about using the correct account when license info is set so we'll just wipe the license info
				{
					GOD temp(fileNameX, filePathX);
					allGODBundle.push_back(temp);
				}
				// 0 is returned for none GOD files and therefore ignored
			}
			nIndex++;
		}
		while(FindNextFile(hFind, &wfd));
		FindClose(hFind);
	}
	return S_OK;
}

void MountDevice(const char* mountPath, char* path, char* msg, int* mounted){
	if (Map(mountPath, path) == S_OK)
	{
		devices.push_back(mountPath);
		debugLog(msg);
		*mounted++;
	}
}

void mountdrives()
{
	int mounted = 0;
	MountDevice("HDD:", "\\Device\\Harddisk0\\Partition1", "hdd mounted", &mounted);
	MountDevice("USB0:", "\\Device\\Mass0", "usb0 mounted", &mounted);
	MountDevice("USB1:", "\\Device\\Mass1", "usb1 mounted", &mounted);
	MountDevice("USB2:", "\\Device\\Mass2", "usb2 mounted", &mounted);
	MountDevice("USB3:", "\\Device\\Mass3", "usb3 mounted", &mounted);
	MountDevice("USB4:", "\\Device\\Mass4", "usb4 mounted", &mounted);
	MountDevice("USBMU0:", "\\Device\\Mass0PartitionFile\\Storage", "usbmu0 mounted", &mounted);
	MountDevice("USBMU1:", "\\Device\\Mass1PartitionFile\\Storage", "usbmu1 mounted", &mounted);
	MountDevice("USBMU2:", "\\Device\\Mass2PartitionFile\\Storage", "usbmu2 mounted", &mounted);
	MountDevice("USBMU3:", "\\Device\\Mass3PartitionFile\\Storage", "usbmu3 mounted", &mounted);
	MountDevice("USBMU4:", "\\Device\\Mass4PartitionFile\\Storage", "usbmu4 mounted", &mounted);
	if (mounted != 0)
	{
		//debugLog("Drive(s) mounted, Scanning folders");
		console.Format("Scanning folders, please wait...");
	}
	else
	{
		//debugLog("No Drives found!");
		//console.Format("No drives found!\n");
	}
}

//--------------------------------------------------------------------------------------
// Name: main
// Desc: Entry point to the program
//--------------------------------------------------------------------------------------
VOID __cdecl main()
{
	bool keypush = false;
	console.Create("embed:\\font", 0x00000000, 0xFFFF6600);
	console.Format("--GOD Unlocker v1.0 by Byrom--\n");
	console.Format("This application will unlock GOD format games that were orginally purchased on a different console or KV.bin.\n");
	console.Format("Credits to Swizzy & Dstruktiv for the NXE2GOD source from which this is based.\n");
	console.Format("NOTE:\n          Only titles unlocked by this application will be detected as \"Previously Unlocked\".\n          This is to ensure all titles are unlocked using the same method and license flags etc.\n");
	debuglogexists = FileExists("game:\\debug.log");
	if (!debuglogexists) genlog();
	mountdrives();
	console.Format("\n");

	unsigned int CombinedResultSize = 0;
	console.Format("Scanning storage devices for GOD titles...\n");
	for (unsigned int i = 0; i < devices.size(); i++)
	{
		if (devices.size() == 0)
			break;
		//console.Format("Scanning %s\\ for GOD titles...", devices[i].c_str());
		string tmp = devices[i] + filePathzzz;
		ScanDir(tmp);
		//console.Format("%s\\ Complete\n", devices[i].c_str());
		
	}
	CombinedResultSize = allGODRegular.size() + allGODMSPSpoofed.size() + allGODUnlocked.size() + allGODBundle.size();
	
	if ((CombinedResultSize == 0) && (devices.size() == 0))
		console.Format("\nNo GOD titles found\n\nPush any key to exit");
	else if (CombinedResultSize == 0)
		console.Format("\nNo GOD titles found\n\nPush any key to exit");
	else
	{
		console.Format("Found %d GOD titles!\n", CombinedResultSize);
		console.Format("%d Previously Unlocked.\n", allGODUnlocked.size());
		console.Format("%d Regular GOD. (LIVE Header)\n", allGODRegular.size());
		console.Format("%d MSP Spoofed GOD. (PIRS Header)\n", allGODMSPSpoofed.size());
		console.Format("%d Game Bundle Downloader GOD. (Need to be patched differently)\n", allGODBundle.size());
		//console.Format("\nGOD Files found:\n\n");
		
		//console.Format("--Regular--\n");
		debugLog("--Regular--");
		for (unsigned int i = 0; i < allGODRegular.size(); i++)
		{
			char* TitleInfo;
			sprintf(TitleInfo, "%ls: %s", allGODRegular[i].title, allGODRegular[i].path.c_str());
			debugLog(TitleInfo);
			//console.Format("%ls at location: %s%s\n", allGODRegular[i].title, allGODRegular[i].path.c_str(), allGODRegular[i].fileName.c_str());
		//	console.Format("%ls: %s\n", allGODRegular[i].title, allGODRegular[i].path.c_str());
		}
		//console.Format("--MSP Spoofed--\n");
		debugLog("--MSP Spoofed--");
		for (unsigned int i = 0; i < allGODMSPSpoofed.size(); i++)
		{
			char* TitleInfo;
			sprintf(TitleInfo, "%ls: %s", allGODMSPSpoofed[i].title, allGODMSPSpoofed[i].path.c_str());
			debugLog(TitleInfo);
			//	console.Format("%ls: %s\n", allGODMSPSpoofed[i].title, allGODMSPSpoofed[i].path.c_str());
		}
		//console.Format("--Bundle Downloader--\n");
		debugLog("--Bundle Downloader--");
		for (unsigned int i = 0; i < allGODBundle.size(); i++)
		{
			char* TitleInfo;
			sprintf(TitleInfo, "%ls: %s", allGODBundle[i].title, allGODBundle[i].path.c_str());
			debugLog(TitleInfo);
			//	console.Format("%ls: %s\n", allGODBundle[i].title, allGODBundle[i].path.c_str());
		}

		int OptionSelected = 0;
		console.Format("\nSelect an option:\n - A to unlock all GOD titles\n - X to fix & unlock only MSP Spoofed GOD titles\n - Y to unlock only Bundle Downloaders\n - B to cancel and quit\n\n");
		while (!keypush)
		{
			ATG::GAMEPAD* pGamepad = ATG::Input::GetMergedInput();
			if (pGamepad->wPressedButtons & XINPUT_GAMEPAD_A)
			{
				OptionSelected = 1;
				keypush = true;
			}
			if (pGamepad->wPressedButtons & XINPUT_GAMEPAD_X)
			{
				OptionSelected = 2;
				keypush = true;
			}
			if (pGamepad->wPressedButtons & XINPUT_GAMEPAD_Y)
			{
				OptionSelected = 3;
				keypush = true;
			}
			if (pGamepad->wPressedButtons & XINPUT_GAMEPAD_B)
				XLaunchNewImage(XLAUNCH_KEYWORD_DEFAULT_APP, 0);
		}
		if (OptionSelected == 1)
		{
			console.Format("Unlocking all GOD titles, please wait...\n\n");
			for (unsigned int i = 0; i < allGODRegular.size(); i++)
				UnlockGOD(allGODRegular[i]);
			for (unsigned int i = 0; i < allGODMSPSpoofed.size(); i++)
				UnlockGOD(allGODMSPSpoofed[i]);
			DoNotSetLicense = true;
			for (unsigned int i = 0; i < allGODBundle.size(); i++)
				UnlockGOD(allGODBundle[i]);
		}
		else if (OptionSelected == 2)
		{
			console.Format("Fixing & unlocking all MSP Spoofed GOD titles, please wait...\n\n");
			for (unsigned int i = 0; i < allGODMSPSpoofed.size(); i++)
				UnlockGOD(allGODMSPSpoofed[i]);
		}
		else if (OptionSelected == 3)
		{
			console.Format("Unlocking all Game Bundle Downloaders, please wait...\n\n");
			DoNotSetLicense = true;
			for (unsigned int i = 0; i < allGODBundle.size(); i++)
				UnlockGOD(allGODBundle[i]);
		}
		console.Format("Processing complete!\nBackups of the original files can be found here: \\Content\\0000000000000000\\<TitleID>\\00007000\\BACKUP\nPush any key to exit");
	}

	keypush = false;
	while (!keypush)
	{
		ATG::GAMEPAD* pGamepad = ATG::Input::GetMergedInput();
		if (pGamepad->wPressedButtons)
			XLaunchNewImage(XLAUNCH_KEYWORD_DEFAULT_APP, 0);
	}
}