/*
 * Contains global data structures and pointers only.
 * Utility functions go into D3D11Utils.h
 */

#pragma once

// system
#include <windows.h>

//====================================================================
//                      (#define)s
//====================================================================

typedef struct DDS_HEADER
{
    DWORD           dwSize;
    DWORD           dwFlags;
    DWORD           dwHeight;
    DWORD           dwWidth;
    DWORD           dwPitchOrLinearSize;
    DWORD           dwDepth;
    DWORD           dwMipMapCount;
    DWORD           dwReserved1[11];
    struct DDS_PIXELFORMAT {
        DWORD dwSize;
        DWORD dwFlags;
        DWORD dwFourCC;
        DWORD dwRGBBitCount;
        DWORD dwRBitMask;
        DWORD dwGBitMask;
        DWORD dwBBitMask;
        DWORD dwABitMask;
    } ddspf;
    DWORD           dwCaps;
    DWORD           dwCaps2;
    DWORD           dwCaps3;
    DWORD           dwCaps4;
    DWORD           dwReserved2;
} DDS_HEADER;

//#define DEBUG
#define LOG0 1      //always.
#define LOG1 1      //main functions.
#define LOG2 1      //causes perfomance drop.

#define IDS_INFO        1000
#define IDS_TYPE        1001
#define IDS_STRING1     1

#define VID_DEVICE      1019
#define VID_FULL        1020
#define VID_WINDOW      1021
#define VID_STATIC5     1022//?
#define VID_STATIC6     1023//?
#define VID_MODE        1024
#define VID_BPP         1025
#define VID_VSYNC       1026
#define VID_PAGEFLIP    1027
#define VID_STATIC7     1028
#define VID_STATIC8     1029
#define VID_STATIC9     1030
#define VID_WIDTH       1031
#define VID_HEIGHT      1032
#define VID_ASPECT      1033
#define VID_4X3         1034
#define VID_16X10       1035
#define VID_16X9        1036
#define VID_ENUM        1037
#define VID_D3D11CONFIG 9038
#define VID_STENCIL     1188

#define CTRL_OK         9000
#define CTRL_CANCEL     9001
#define CTRL_ABOUT      9002

#define CTRL_GENERAL    9003
#define CTRL_VESSELS    9004
#define CTRL_PLANETS    9005

#define CTRL_G_THREADS      9101    //+9102
#define CTRL_G_THREADS_HELP 9113

#define CTRL_G_AA           9103    //+9104
#define CTRL_G_AA_HELP      9114

#define CTRL_G_SKPAD        9105    //+9106
#define CTRL_G_SKPAD_HELP   9115

#define CTRL_G_MFDTEX       9107    //+9108
#define CTRL_G_MFDTRF       9111    //+9112

#define CTRL_G_PP_EFFECTS   9121    //+9122

#define CTRL_G_CREATE_SYM   9131

//9202
#define CTRL_V_MIPMAP       9201
#define CTRL_V_MIPMAP_HLP   9224

//9204
#define CTRL_V_TEXFILTER        9203
#define CTRL_V_TEXFILTER_HLP    9223

//9207
#define CTRL_V_NORM     9205
#define CTRL_V_NORM_HLP 9206

//9210
#define CTRL_V_BUMP     9208
#define CTRL_V_BUMP_HLP 9209

//9213
#define CTRL_V_SPEC     9211
#define CTRL_V_SPEC_HLP 9212

//9216
#define CTRL_V_EMIS     9214
#define CTRL_V_EMIS_HLP 9215

//9219
#define CTRL_V_PSLLIGHTS        9217
#define CTRL_V_PSLLIGHTS_HLP    9218

//9222
#define CTRL_V_BASELLIGHTS      9220
#define CTRL_V_BASELLIGHTS_HLP  9221

//9227
#define CTRL_V_TILELLIGHTS      9225
#define CTRL_V_TILELLIGHTS_HLP  9226

#define CTRL_P_LMODE    9301    //+9302
#define CTRL_P_LFREQ    9303    //+9304
#define CTRL_P_TEXFILTER    9305    //+9306
#define CTRL_P_TEXFILTER_HLP 9306
#define CTRL_P_LLIGHTS  9307    //+9308

#define SPEC_DEFAULT 0xFFFFFFFF
#define SPEC_INHERIT 0xFFFFFFFE

#define LABEL_DISTLIMIT 0.6
#define MAXPLANET 512
#define SURF_MAX_PATCHLEVEL 14

#define C2504 0 //used for debugging C2504 "base class is undefined" error

//====================================================================
//                      Macros
//====================================================================

#if LOG0
    #ifndef WLOG0
    #define WLOG0( x ) {    \
        oapiWriteLog( x );  \
    }
    #endif
#else
    #ifndef WLOG0
    #define WLOG0( x )
    #endif
#endif

#if LOG1
    #ifndef WLOG1
    #define WLOG1( x ) {    \
        oapiWriteLog( x );  \
    }
    #endif
#else
    #ifndef WLOG1
    #define WLOG1( x )
    #endif
#endif

#if LOG2
    #ifndef WLOG2
    #define WLOG2( x ) {    \
        oapiWriteLog( x );  \
    }
    #endif
#else
    #ifndef WLOG2
    #define WLOG2( x )
    #endif
#endif

#define SHOW_MESSAGE(cap,msg) MessageBoxA( 0, msg, cap, MB_ICONERROR | MB_OK )

#define CLAMP_VALUE(val,min,max) val = (val < min) ? min : (val > max)? max : val

#pragma pack(push,1)
struct TILEFILESPEC {
    DWORD sidx;       // index for surface texture (-1: not present)
    DWORD midx;       // index for land-water mask texture (-1: not present)
    DWORD eidx;       // index for elevation data blocks (not used yet; always -1)
    DWORD flags;      // tile flags: bit 0: has diffuse component; bit 1: has specular component; bit 2: has city lights
    DWORD subidx[4];  // subtile indices
};

struct LMASKFILEHEADER {
    char id[8];          //    ID+version string
    DWORD hsize;         //    header size
    DWORD flag;          //    bitflag content information
    DWORD npatch;        //    number of patches
    BYTE minres;         //    min. resolution level
    BYTE maxres;         //    max. resolution level
};
#pragma pack(pop)


