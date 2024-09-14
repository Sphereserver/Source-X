#include "../game/uo_files/CUOMultiItemRec.h"
#include "../game/CServerConfig.h"
#include "../sphere/threads.h"
#include "CException.h"
#include "CLog.h"
#include "CUOInstall.h"
#include <sstream>

#define MAP_MAX_SUPPORTED_INDEX 6


//////////////////////////////////////////////////////////////////
// -CUOInstall

CUOInstall::CUOInstall()
{
	memset(m_FileFormat, 0, sizeof(m_FileFormat));
	memset(m_IsMapUopFormat, 0, sizeof(m_IsMapUopFormat));
	memset(m_UopMapAddress, 0, sizeof(m_UopMapAddress));
};

CSString CUOInstall::GetFullExePath( lpctstr pszName ) const
{
	return CSFile::GetMergedFileName( m_sExePath, pszName );
}

CSString CUOInstall::GetFullCDPath( lpctstr pszName ) const
{
	return CSFile::GetMergedFileName( m_sCDPath, pszName );
}

bool CUOInstall::FindInstall()
{
#ifdef _WIN32
	// Get the install path from the registry.

	static constexpr lpctstr m_szKeys[] =
	{
		"Software\\Origin Worlds Online\\Ultima Online\\1.0",
		"Software\\Origin Worlds Online\\Ultima Online Third Dawn\\1.0",
		"Software\\Origin Worlds Online\\Ultima Online\\KR Legacy Beta",
		"Software\\Electronic Arts\\EA Games\\Ultima Online Stygian Abyss Classic",
		"Software\\Electronic Arts\\EA Games\\Ultima Online Classic"
	};

	HKEY hKey = nullptr;
	LSTATUS lRet = 0;
	for ( size_t i = 0; i < ARRAY_COUNT(m_szKeys); ++i )
	{
		lRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
			m_szKeys[i], // address of name of subkey to query
			0, KEY_READ, &hKey );
		if ( lRet == ERROR_SUCCESS )
			break;
	}
	if ( lRet != ERROR_SUCCESS )
	{
		return false;
	}

	tchar szValue[ _MAX_PATH ];
	DWORD lSize = sizeof( szValue );
	DWORD dwType = REG_SZ;
	lRet = RegQueryValueEx(hKey, "ExePath", nullptr, &dwType, (byte*)szValue, &lSize);

	if ( lRet == ERROR_SUCCESS && dwType == REG_SZ )
	{
		tchar * pSlash = strrchr( szValue, '\\' );	// get rid of the client.exe part of the name
		if ( pSlash ) * pSlash = '\0';
		m_sExePath = szValue;
	}
	else
	{
		lRet = RegQueryValueEx(hKey, "InstallDir", nullptr, &dwType, (byte*)szValue, &lSize);
		if ( lRet == ERROR_SUCCESS && dwType == REG_SZ )
			m_sExePath = szValue;
	}

	// ??? Find CDROM install base as well, just in case.
	// uo.cfg CdRomDataPath=e:\uo

	lRet = RegQueryValueEx(hKey, "InstCDPath", nullptr, &dwType, (byte*)szValue, &lSize);

	if ( lRet == ERROR_SUCCESS && dwType == REG_SZ )
	{
		m_sCDPath = szValue;
	}

	RegCloseKey( hKey );

#else
	// Other OSes have no registry so we must have the INI file show us where it is installed.
#endif
	return true;
}

void CUOInstall::DetectMulVersions()
{
	ADDTOCALLSTACK("CUOInstall::DetectMulVersions");

	// assume all formats are original to start with
	for (size_t i = 0; i < ARRAY_COUNT(m_FileFormat); ++i)
		m_FileFormat[i] = VERFORMAT_ORIGINAL;

	// check for High Seas tiledata format
	// this can be tested for by checking the file size, which was 3188736 bytes at release
	if ( m_File[VERFILE_TILEDATA].IsFileOpen() && m_File[VERFILE_TILEDATA].GetLength() >= 3188736 )
		m_FileFormat[VERFILE_TILEDATA] = VERFORMAT_HIGHSEAS;

	// check for High Seas multi format
	// we can't use multi.mul length because it varies and multi.idx is always 98184 bytes, the best option
	// so far seems to be to check the size of the first entry to see if its length is divisible by the new
	// format length (risky if the first entry is custom and happens to be be divisible by both lengths)
	CUOIndexRec index;
	if (ReadMulIndex( VERFILE_MULTIIDX, VERFILE_MULTI, 0x00, index) && (index.GetBlockLength() % sizeof(CUOMultiItemRec_HS)) == 0)
		m_FileFormat[VERFILE_MULTIIDX] = VERFORMAT_HIGHSEAS;
}

bool CUOInstall::OpenFile( CSFile & file, lpctstr pszName, word wFlags )
{
	ADDTOCALLSTACK("CUOInstall::OpenFile");
	ASSERT(pszName);
	if ( !m_sPreferPath.IsEmpty() )
	{
		if ( file.Open(GetPreferPath(pszName), wFlags) )
			return true;
	}
	else
	{
		if ( file.Open(GetFullExePath(pszName), wFlags) )
			return true;
		if ( file.Open(GetFullCDPath(pszName), wFlags) )
			return true;
	}
	return false;
}

lpctstr CUOInstall::GetBaseFileName( VERFILE_TYPE i ) // static
{
	ADDTOCALLSTACK("CUOInstall::GetBaseFileName");
	static lpctstr constexpr sm_szFileNames[VERFILE_QTY] =
	{
		"artidx.mul",	// Index to ART
		"art.mul",		// Artwork such as ground, objects, etc.
		"anim.idx",
		"anim.mul",		// Animations such as monsters, people, and armor.
		"soundidx.mul", // Index into SOUND
		"sound.mul",	// Sampled sounds
		"texidx.mul",	// Index into TEXMAPS
		"texmaps.mul",	// Texture map data (the ground).
		"gumpidx.mul",	// Index to GUMPART
		"gumpart.mul",	// Gumps. Stationary controller bitmaps such as windows, buttons, paperdoll pieces, etc.
		"multi.idx",
		"multi.mul",	// Groups of art (houses, castles, etc)
		"skills.idx",
		"skills.mul",
		"verdata.mul",
		"map0.mul",		// MAP(s)
		"staidx0.mul",	// STAIDX(s)
		"statics0.mul",	// Static objects on MAP(s)
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		"tiledata.mul", // Data about tiles in ART. name and flags, etc
		"animdata.mul", //
		"hues.mul"		// the 16 bit color pallete we use for everything.
	};

	return ( i<0 || i>=VERFILE_QTY ) ? nullptr : sm_szFileNames[i];
}

CSFile * CUOInstall::GetMulFile( VERFILE_TYPE i )
{
	ASSERT( i<VERFILE_QTY );
	return( &(m_File[i]));
}

VERFILE_FORMAT CUOInstall::GetMulFormat( VERFILE_TYPE i )
{
	ASSERT( i<VERFILE_QTY );
	return( m_FileFormat[i] );
}

void CUOInstall::SetPreferPath( lpctstr pszName )
{
	m_sPreferPath = pszName;
}

CSString CUOInstall::GetPreferPath( lpctstr pszName ) const
{
	return CSFile::GetMergedFileName(m_sPreferPath, pszName);
}

bool CUOInstall::OpenFile( VERFILE_TYPE i )
{
	ADDTOCALLSTACK("CUOInstall::OpenFile");
	CSFile	*pFile = GetMulFile(i);
	if ( !pFile )
		return false;
	if ( pFile->IsFileOpen())
		return true;

    lpctstr ptcFilePath = pFile->GetFilePath();
	if ( !ptcFilePath || !strlen(ptcFilePath) )
	{
		if ( pFile->Open(pFile->GetFilePath(), OF_READ|OF_SHARE_DENY_WRITE) )
			return true;
	}

	lpctstr pszTitle = GetBaseFileName((VERFILE_TYPE)i);
	if ( !pszTitle )
        return false;

	return OpenFile(m_File[i], pszTitle, OF_READ|OF_SHARE_DENY_WRITE);
}

VERFILE_TYPE CUOInstall::OpenFiles( ullong ullMask )
{
	ADDTOCALLSTACK("CUOInstall::OpenFiles");
	// Now open all the required files.
	// RETURN: VERFILE_QTY = all open success.

    int i;
	for ( i = 0; i < VERFILE_QTY; ++i )
	{
		if ( ! ( ullMask & ( (ullong)1 << i )) )
			continue;
		if ( GetBaseFileName((VERFILE_TYPE)i) == nullptr )
			continue;

		bool fFileLoaded = true;
		switch (i)
		{
			default:
				if ( !OpenFile((VERFILE_TYPE)i))
				{
					//	make some MULs optional
					switch ( i )
					{
						case VERFILE_VERDATA:
							break;

						default:
							fFileLoaded = false;
							break;
					}
				}
				break;

			case VERFILE_MAP:
			{
				// map file is handled differently
				tchar verdata_str_buf[256];

				//	check for map files of custom maps
				for (int m = 0; m < MAP_SUPPORTED_QTY; ++m)
				{
					if (g_MapList.IsInitialized(m) || (m == 0)) //Need at least a minimum of map0... (Ben)
					{
                        const int index = g_MapList.m_mapGeoData.maps[m].num;
						if (index == -1)
						{
							g_MapList.m_mapGeoData.maps[m].enabled = false;
							continue;
						}

						if (!m_Maps[index].IsFileOpen())
						{
							sprintf(verdata_str_buf, "map%d.mul", index);
							OpenFile(m_Maps[index], verdata_str_buf, OF_READ | OF_SHARE_DENY_WRITE);

							if (m_Maps[index].IsFileOpen())
							{
								m_IsMapUopFormat[index] = false;
							}
							else
							{
								sprintf(verdata_str_buf, "map%dLegacyMUL.uop", index);
								OpenFile(m_Maps[index], verdata_str_buf, OF_READ | OF_SHARE_DENY_WRITE);

								//Should parse uop file here for faster reference later.
								if (m_Maps[index].IsFileOpen())
								{
									m_IsMapUopFormat[index] = true;

									dword dwHashLo, dwHashHi, dwCompressedSize, dwHeaderLenght, dwFilesInBlock, dwTotalFiles, dwLoop;
									uint64 qwUOPPtr;

									m_Maps[index].Seek(sizeof(dword) * 3, SEEK_SET);
									m_Maps[index].Read(&dwHashLo, sizeof(dword));
									m_Maps[index].Read(&dwHashHi, sizeof(dword));
									qwUOPPtr = ((uint64)dwHashHi << 32) + dwHashLo;
									m_Maps[index].Seek(sizeof(dword), SEEK_CUR);
									m_Maps[index].Read(&dwTotalFiles, sizeof(dword));
									m_Maps[index].Seek((int)qwUOPPtr, SEEK_SET);
									dwLoop = dwTotalFiles;

									while (qwUOPPtr > 0)
									{
										m_Maps[index].Read(&dwFilesInBlock, sizeof(dword));
										m_Maps[index].Read(&dwHashLo, sizeof(dword));
										m_Maps[index].Read(&dwHashHi, sizeof(dword));
										qwUOPPtr = ((uint64)dwHashHi << 32) + dwHashLo;

										while ((dwFilesInBlock > 0) && (dwTotalFiles > 0))
										{
											--dwTotalFiles;
											--dwFilesInBlock;

											m_Maps[index].Read(&dwHashLo, sizeof(dword));
											m_Maps[index].Read(&dwHashHi, sizeof(dword));
											m_Maps[index].Read(&dwHeaderLenght, sizeof(dword));
											m_Maps[index].Read(&dwCompressedSize, sizeof(dword));

											MapAddress pMapAddress;
											pMapAddress.qwAdress = (((uint64)dwHashHi << 32) + dwHashLo) + dwHeaderLenght;

											m_Maps[index].Seek(sizeof(dword), SEEK_CUR);
											m_Maps[index].Read(&dwHashLo, sizeof(dword));
											m_Maps[index].Read(&dwHashHi, sizeof(dword));
											uint64 qwHash = ((uint64)dwHashHi << 32) + dwHashLo;
											m_Maps[index].Seek(sizeof(dword) + sizeof(word), SEEK_CUR);

											for (dword x = 0; x < dwLoop; ++x)
											{
												sprintf(verdata_str_buf, "build/map%dlegacymul/%.8u.dat", index, x);
												if (HashFileName(verdata_str_buf) == qwHash)
												{
													pMapAddress.dwFirstBlock = x * 4096;
													pMapAddress.dwLastBlock = (x * 4096) + (dwCompressedSize / 196) - 1;
													m_UopMapAddress[index][x] = pMapAddress;
													break;
												}
											}
										}

										m_Maps[index].Seek((int)qwUOPPtr, SEEK_SET);
									}
								}//End of UOP Map parsing
								else if (index == 0) // neither file exists, map0 is required
								{
									fFileLoaded = false;
									break;
								}
							}
						}
						if (!m_Staidx[index].IsFileOpen())
						{
							sprintf(verdata_str_buf, "staidx%d.mul", index);
							OpenFile(m_Staidx[index], verdata_str_buf, OF_READ | OF_SHARE_DENY_WRITE);
						}
						if (!m_Statics[index].IsFileOpen())
						{
							sprintf(verdata_str_buf, "statics%d.mul", index);
							OpenFile(m_Statics[index], verdata_str_buf, OF_READ | OF_SHARE_DENY_WRITE);
						}
						if (g_Cfg.m_fUseMapDiffs)
						{
							if (!m_Mapdif[index].IsFileOpen())
							{
								sprintf(verdata_str_buf, "mapdif%d.mul", index);
								OpenFile(m_Mapdif[index], verdata_str_buf, OF_READ | OF_SHARE_DENY_WRITE);
							}
							if (!m_Mapdifl[index].IsFileOpen())
							{
								sprintf(verdata_str_buf, "mapdifl%d.mul", index);
								OpenFile(m_Mapdifl[index], verdata_str_buf, OF_READ | OF_SHARE_DENY_WRITE);
							}
							if (!m_Stadif[index].IsFileOpen())
							{
								sprintf(verdata_str_buf, "stadif%d.mul", index);
								OpenFile(m_Stadif[index], verdata_str_buf, OF_READ | OF_SHARE_DENY_WRITE);
							}
							if (!m_Stadifi[index].IsFileOpen())
							{
								sprintf(verdata_str_buf, "stadifi%d.mul", index);
								OpenFile(m_Stadifi[index], verdata_str_buf, OF_READ | OF_SHARE_DENY_WRITE);
							}
							if (!m_Stadifl[index].IsFileOpen())
							{
								sprintf(verdata_str_buf, "stadifl%d.mul", index);
								OpenFile(m_Stadifl[index], verdata_str_buf, OF_READ | OF_SHARE_DENY_WRITE);
							}
						}

						//	if any of the map files are not available, mark map as unavailable
						//	mapdif and stadif files are not required.
						if (!m_Maps[index].IsFileOpen() ||
							!m_Staidx[index].IsFileOpen() ||
							!m_Statics[index].IsFileOpen())
						{
							if (m_Maps[index].IsFileOpen())
								m_Maps[index].Close();
							if (m_Staidx[index].IsFileOpen())
								m_Staidx[index].Close();
							if (m_Statics[index].IsFileOpen())
								m_Statics[index].Close();

							if (index == 1 && m_Maps[0].IsFileOpen())
								g_MapList.m_mapGeoData.maps[m].num = 0;
							else
								g_MapList.m_mapGeoData.maps[m].id = 0;
						}

						//	mapdif and mapdifl are not required, but if one exists so should
						//	the other
						if (m_Mapdif[index].IsFileOpen() != m_Mapdifl[index].IsFileOpen())
						{
							if (m_Mapdif[index].IsFileOpen())
								m_Mapdif[index].Close();
							if (m_Mapdifl[index].IsFileOpen())
								m_Mapdifl[index].Close();
						}

						//	if one of the stadif files exissts, so should the others
						if (m_Stadif[index].IsFileOpen() != m_Stadifi[index].IsFileOpen() ||
							m_Stadif[index].IsFileOpen() != m_Stadifl[index].IsFileOpen())
						{
							if (m_Stadif[index].IsFileOpen())
								m_Stadif[index].Close();
							if (m_Stadifi[index].IsFileOpen())
								m_Stadifi[index].Close();
							if (m_Stadifl[index].IsFileOpen())
								m_Stadifl[index].Close();
						}
					}
				}
				break;
			}
		}

		// stop if we hit a failure
		if (fFileLoaded == false)
			return (VERFILE_TYPE)i;
	}

    // --

	DetectMulVersions();

    // --

	g_MapList.Init();

    std::stringstream maps_ss;
	tchar * mapname_str_buf = Str_GetTemp();
	for ( uchar j = 0; j <= MAP_MAX_SUPPORTED_INDEX; ++j )
	{
		if ( j == 5 )	// ML just added some changes on maps 0/1 instead of a new map
			continue;

		bool fSup = false;
		if ( j > 5 )	// SA+
			fSup = ( g_MapList.IsMapSupported(j - 1) );
		else
			fSup = ( g_MapList.IsMapSupported(j) );

		if ( fSup )
		{
			switch ( j )
			{
				case 0: sprintf(mapname_str_buf, "Felucca (%d)", j);			break;
				case 1: sprintf(mapname_str_buf, "Trammel (%d)", j);			break;
				case 2: sprintf(mapname_str_buf, "Ilshenar (%d)", j);		break;
				case 3: sprintf(mapname_str_buf, "Malas (%d)", j);			break;
				case 4: sprintf(mapname_str_buf, "Tokuno Islands (%d)", j);	break;
				case 6: sprintf(mapname_str_buf, "Ter Mur (%d)", j-1);		break;
			}
			if (maps_ss.peek() != std::char_traits<char>::eof()) //( !maps_ss.view().empty() )
				maps_ss << ", ";
			maps_ss << mapname_str_buf;
		}
	}

	const std::string maps_str(maps_ss.str());
	if ( !maps_str.empty() )
		g_Log.Event(LOGM_INIT, "Expansion maps supported: %s\n", maps_str.c_str());

    // --

    m_tiledata.Load();

    if (g_Cfg.m_fUseMobTypes)
    {
        m_mobtypes.Load();
    }

	return (VERFILE_TYPE)i;
}

void CUOInstall::CloseFiles()
{
	ADDTOCALLSTACK("CUOInstall::CloseFiles");
	int i;

	for ( i = 0; i < VERFILE_QTY; ++i )
	{
		if ( m_File[i].IsFileOpen() )
			m_File[i].Close();
	}

	for ( i = 0; i < MAP_SUPPORTED_QTY; ++i )
	{
		if ( m_Maps[i].IsFileOpen() )		m_Maps[i].Close();
		if ( m_Statics[i].IsFileOpen() )	m_Statics[i].Close();
		if ( m_Staidx[i].IsFileOpen() )		m_Staidx[i].Close();
		if ( m_Mapdif[i].IsFileOpen() )		m_Mapdif[i].Close();
		if ( m_Mapdifl[i].IsFileOpen() )	m_Mapdifl[i].Close();
		if ( m_Stadif[i].IsFileOpen() )		m_Stadif[i].Close();
		if ( m_Stadifi[i].IsFileOpen() )	m_Stadifi[i].Close();
		if ( m_Stadifl[i].IsFileOpen() )	m_Stadifl[i].Close();
	}
}

bool CUOInstall::ReadMulIndex(CSFile &file, dword id, CUOIndexRec &Index)
{
	ADDTOCALLSTACK("CUOInstall::ReadMulIndex");
	int iOffset = (int)(id * sizeof(CUOIndexRec));

	if ( file.Seek(iOffset, SEEK_SET) != iOffset )
		return false;

	if ( (uint)file.Read(static_cast<void *>(&Index), sizeof(CUOIndexRec)) != sizeof(CUOIndexRec) )
		return false;

	return Index.HasData();
}

bool CUOInstall::ReadMulData(CSFile &file, const CUOIndexRec &Index, void * pData)
{
	ADDTOCALLSTACK("CUOInstall::ReadMulData");
	if ( (uint)file.Seek(Index.GetFileOffset(), SEEK_SET) != Index.GetFileOffset() )
		return false;

	dword dwLength = Index.GetBlockLength();
	if ( (uint)file.Read(pData, dwLength) != dwLength )
		return false;

	return true;
}

bool CUOInstall::ReadMulIndex(VERFILE_TYPE fileindex, VERFILE_TYPE filedata, dword id, CUOIndexRec & Index)
{
	ADDTOCALLSTACK("CUOInstall::ReadMulIndex");
	// Read about this data type in one of the index files.
	// RETURN: true = we are ok.
	ASSERT(fileindex<VERFILE_QTY);

	// Is there an index for it in the VerData ?
	if ( g_VerData.FindVerDataBlock(filedata, id, Index) )
		return true;

	return ReadMulIndex(m_File[fileindex], id, Index);
}

bool CUOInstall::ReadMulData(VERFILE_TYPE filedata, const CUOIndexRec & Index, void * pData)
{
	ADDTOCALLSTACK("CUOInstall::ReadMulData");
	// Use CSFile::GetLastError() for error.
	if ( Index.IsVerData() ) filedata = VERFILE_VERDATA;

	return ReadMulData(m_File[filedata], Index, pData);
}

//////////////////////////////////////////////////////////////////
// -CVerDataMul

CVerDataMul::CVerDataMul()
{
}

CVerDataMul::~CVerDataMul()
{
	Unload();
}

int CVerDataMul::QCompare(size_t left, dword dwRefIndex) const
{
	dword dwIndex2 = GetEntry(left)->GetIndex();
	return(dwIndex2 - dwRefIndex);
}

void CVerDataMul::QSort(size_t left, size_t right)
{
	ADDTOCALLSTACK("CVerDataMul::QSort");
	static uint uiReentrant = 0;
	if (uiReentrant > 1'000'000)
	{
		g_Log.EventError("VerData QSort iterated over 1 million entries, stopping.\n");
		return;
	}

	ASSERT(left <= right);
	size_t j = left;
	size_t i = right;

	dword dwRefIndex = GetEntry((left + right) / 2)->GetIndex();

	do
	{
		while (j < m_Data.size() && QCompare(j, dwRefIndex) < 0)
			++j;
		while (i > 0 && QCompare(i, dwRefIndex) > 0)
			--i;

		if (i >= j)
		{
			if (i != j)
			{
				CUOVersionBlock Tmp = m_Data[i];
				CUOVersionBlock block = m_Data[j];
				m_Data[i] = std::move(block);
				m_Data[j] = std::move(Tmp);
			}

			if (i > 0)
				--i;
			if (j < m_Data.size())
				++j;
		}

	} while (j <= i);

	++uiReentrant;
	if (left < i)
		QSort(left, i);
	if (j < right)
		QSort(j, right);
	--uiReentrant;
}

void CVerDataMul::Load(CSFile & file)
{
	ADDTOCALLSTACK("CVerDataMul::Load");
	// assume it is in sorted order.
	if (GetCount())	// already loaded.
	{
		return;
	}

	// #define g_fVerData g_Install.m_File[VERFILE_VERDATA]

	if (!file.IsFileOpen())		// T2a might not have this.
		return;

	file.SeekToBegin();
	dword dwQty;
	if (file.Read(static_cast<void *>(&dwQty), sizeof(dwQty)) <= 0)
	{
		throw CSError(LOGL_CRIT, CSFile::GetLastError(), "VerData: Read Qty");
	}

	Unload();
	m_Data.resize(dwQty);

	if (file.Read(static_cast<void *>(m_Data.data()), dwQty * sizeof(CUOVersionBlock)) <= 0)
	{
		throw CSError(LOGL_CRIT, CSFile::GetLastError(), "VerData: Read");
	}

	if (dwQty <= 0)
		return;

	// Now sort it for fast searching.
	// Make sure it is sorted.
	QSort(0, dwQty - 1);

#ifdef _DEBUG
	for (size_t i = 0; i < (dwQty - 1); ++i)
	{
		dword dwIndex1 = GetEntry(i)->GetIndex();
		dword dwIndex2 = GetEntry(i + 1)->GetIndex();
		if (dwIndex1 > dwIndex2)
		{
			DEBUG_ERR(("VerData Array is NOT sorted !\n"));
			throw CSError(LOGL_CRIT, (dword)-1, "VerData: NOT Sorted!");
		}
	}
#endif

}

size_t CVerDataMul::GetCount() const
{
	return(m_Data.size());
}

const CUOVersionBlock * CVerDataMul::GetEntry(size_t i) const
{
	return(&m_Data.at(i));
}

void CVerDataMul::Unload()
{
	m_Data.clear();
}

bool CVerDataMul::FindVerDataBlock(VERFILE_TYPE type, dword id, CUOIndexRec & Index) const
{
	ADDTOCALLSTACK("CVerDataMul::FindVerDataBlock");
	// Search the verdata.mul for changes to stuff.
	// search for a specific block.
	// assume it is in sorted order.

	int iHigh = (int)(GetCount()) - 1;
	if (iHigh < 0)
	{
		return false;
	}

	dword dwIndex = VERDATA_MAKE_INDEX(type, id);
	const CUOVersionBlock * pArray = m_Data.data();
	int iLow = 0;
	while (iLow <= iHigh)
	{
		int i = (iHigh + iLow) / 2;
		dword dwIndex2 = pArray[i].GetIndex();
		int iCompare = dwIndex - dwIndex2;
		if (iCompare == 0)
		{
			Index.CopyIndex(&(pArray[i]));
			return true;
		}
		if (iCompare > 0)
		{
			iLow = i + 1;
		}
		else
		{
			iHigh = i - 1;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////
// -UOP Filename Hash function
ullong HashFileName(CSString csFile)
{
	uint eax, ecx, edx, ebx, esi, edi;

	eax = ecx = edx = 0;
	ebx = edi = esi = (int32) csFile.GetLength() + 0xDEADBEEF;

	int i = 0;

	for ( ; i + 12 < csFile.GetLength(); i += 12 )
	{
		edi = (int32) ( ( csFile[ i + 7 ] << 24 ) | ( csFile[ i + 6 ] << 16 ) | ( csFile[ i + 5 ] << 8 ) | csFile[ i + 4 ] ) + edi;
		esi = (int32) ( ( csFile[ i + 11 ] << 24 ) | ( csFile[ i + 10 ] << 16 ) | ( csFile[ i + 9 ] << 8 ) | csFile[ i + 8 ] ) + esi;
		edx = (int32) ( ( csFile[ i + 3 ] << 24 ) | ( csFile[ i + 2 ] << 16 ) | ( csFile[ i + 1 ] << 8 ) | csFile[ i ] ) - esi;

		edx = ( edx + ebx ) ^ ( esi >> 28 ) ^ ( esi << 4 );
		esi += edi;
		edi = ( edi - edx ) ^ ( edx >> 26 ) ^ ( edx << 6 );
		edx += esi;
		esi = ( esi - edi ) ^ ( edi >> 24 ) ^ ( edi << 8 );
		edi += edx;
		ebx = ( edx - esi ) ^ ( esi >> 16 ) ^ ( esi << 16 );
		esi += edi;
		edi = ( edi - ebx ) ^ ( ebx >> 13 ) ^ ( ebx << 19 );
		ebx += esi;
		esi = ( esi - edi ) ^ ( edi >> 28 ) ^ ( edi << 4 );
		edi += ebx;
	}

	if ( csFile.GetLength() - i > 0 )
	{
		switch ( csFile.GetLength() - i )
		{
			case 12:
				esi += (int32) csFile[ i + 11 ] << 24;
			case 11:
				esi += (int32) csFile[ i + 10 ] << 16;
			case 10:
				esi += (int32) csFile[ i + 9 ] << 8;
			case 9:
				esi += (int32) csFile[ i + 8 ];
			case 8:
				edi += (int32) csFile[ i + 7 ] << 24;
			case 7:
				edi += (int32) csFile[ i + 6 ] << 16;
			case 6:
				edi += (int32) csFile[ i + 5 ] << 8;
			case 5:
				edi += (int32) csFile[ i + 4 ];
			case 4:
				ebx += (int32) csFile[ i + 3 ] << 24;
			case 3:
				ebx += (int32) csFile[ i + 2 ] << 16;
			case 2:
				ebx += (int32) csFile[ i + 1 ] << 8;
			case 1:
				ebx += (int32) csFile[ i ];
				break;
		}

		esi = ( esi ^ edi ) - ( ( edi >> 18 ) ^ ( edi << 14 ) );
		ecx = ( esi ^ ebx ) - ( ( esi >> 21 ) ^ ( esi << 11 ) );
		edi = ( edi ^ ecx ) - ( ( ecx >> 7 ) ^ ( ecx << 25 ) );
		esi = ( esi ^ edi ) - ( ( edi >> 16 ) ^ ( edi << 16 ) );
		edx = ( esi ^ ecx ) - ( ( esi >> 28 ) ^ ( esi << 4 ) );
		edi = ( edi ^ edx ) - ( ( edx >> 18 ) ^ ( edx << 14 ) );
		eax = ( esi ^ edi ) - ( ( edi >> 8 ) ^ ( edi << 24 ) );

		return ( (int64) edi << 32 ) | eax;
	}

	return ( (int64) esi << 32 ) | eax;
}
