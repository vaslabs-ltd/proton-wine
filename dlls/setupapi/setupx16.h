#ifndef __WINE_SETUPX16_H
#define __WINE_SETUPX16_H

#include "wine/windef16.h"

typedef UINT16 HINF16;
typedef UINT16 LOGDISKID16;

/* error codes stuff */

typedef UINT16 RETERR16;
#define OK		0
#define IP_ERROR	(UINT16)100

enum _IP_ERR {
	ERR_IP_INVALID_FILENAME = IP_ERROR+1,
	ERR_IP_ALLOC_ERR,
	ERR_IP_INVALID_SECT_NAME,
	ERR_IP_OUT_OF_HANDLES,
	ERR_IP_INF_NOT_FOUND,
	ERR_IP_INVALID_INFFILE,
	ERR_IP_INVALID_HINF,
	ERR_IP_INVALID_FIELD,
	ERR_IP_SECTION_NOT_FOUND,
	ERR_IP_END_OF_SECTION,
	ERR_IP_PROFILE_NOT_FOUND,
	ERR_IP_LINE_NOT_FOUND,
	ERR_IP_FILEREAD,
	ERR_IP_TOOMANYINFFILES,
	ERR_IP_INVALID_SAVERESTORE,
	ERR_IP_INVALID_INFTYPE
};

typedef struct {
    HFILE16 hInfFile;
    LPCSTR lpInfFileName;
} INF_HANDLE;

extern INF_HANDLE *InfList;
extern WORD InfNumEntries;

#endif /* __WINE_SETUPX16_H */
