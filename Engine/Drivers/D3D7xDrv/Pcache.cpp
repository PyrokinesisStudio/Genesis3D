/****************************************************************************************/
/*  PCache.cpp                                                                          */
/*                                                                                      */
/*  Author: John Pollard                                                                  */
/*  Description: D3D poly cache                                                         */
/*                                                                                      */
/*  Edit History:                                                                       */           
/*   08/24/2004 Wendell Buckner                                                         */
/*    No reason to clear the textures... just disable the texture stages your not using */
/*    (D3DTOP_DISABLE)...                                                               */
/*    Make sure you render regular polys properly...                                    */
/*   03/02/2004 Wendell Buckner                                                         */
/*    DOT3BUMPMAPPING                                                                   */   
/*	  The bump thandles have to be manipulated because base map has to come be assigned */
/*	  to the second texture unit! argh! this is getting messy!                          */
/*   01/11/2004 Wendell Buckner                                                         */
/*    FULLBRIGHT BUG - Fix lightmap being displayed when rendering non-lightmap poly's  */
/*    like full bright polys                                                            */
/*   01/03/2004 Wendell Buckner                                                         */ 
/*    CONFIG DRIVER - Make the driver configurable by "ini" file settings               */
/*   11/11/2003 Wendell Buckner                                                         */
/*    Bumpmapping for the World                                                         */
/*   10/15/2003 Wendell Buckner                                                         */
/*    Bumpmapping for the World                                                         */
/*   08/23/2003 Wendell Buckner                                                         */
/*    BUMPMAPPING                                                                       */
/*	   Don't allow bleeding of bumpmap effect onto actors that shouldn't receive the    */
/*     effect                                                                           */
/*   08/18/2003 Wendell Buckner                                                         */
/*    TODO: Allow mip map levels greater than 0                                         */
/*	 04/23/2003 Wendell Buckner                                                         */
/*	  Send Misc Poly's as triangle lists instead of fans...                             */
/*	 04/21/2003 Wendell Buckner                                                         */
/*	  Send Misc Poly's as triangle lists instead of fans...                             */
/*   04/08/2003 Wendell Buckner                                                         */
/*    BUMPMAPPING                                                                       */
/*   01/28/2003 Wendell Buckner                                                         */
/*	  Send World Poly's as triangle lists instead of fans...                            */
/*    Cache decals so that they can be drawn after all the 3d stuff...                  */
/*   01/18/2003 Wendell Buckner                                                         */
/*    Don't call fog api if it's no set in the global fog flag...                       */
/*    To continually turn this off and on really is ineffecient... turn it on when you  */
/*    need it turn if off when you don'tOptimization from GeForce_Optimization2.doc     */
/*    Basically when you leave the above loop, make sure fog is on                      */
/*    Optimization from GeForce_Optimization2.doc                                       */
/*    9. Do not duplicate render state commands.  Worse is useless renderstates. Do not */
/*       set a renderstate unless it is needed.                                         */
/*   01/10/2003 Wendell Buckner                                                         */
/*    Allow 32-bit color depth lightmaps...                                             */
/*   03/10/2002 Wendell Buckner                                                         */
/*    Procedural Textures                                                               */
/*    if you must lock a texture specify the flags WRITEONLY and DISCARDCONTENTS when   */
/*	 01/24/2002 Wendell Buckner                                                         */ 
/*    Change flags for speed...                                                         */ 
/*   02/28/2001 Wendell Buckner                                                         */ 
/*    These render states are unsupported d3d 7.0                                       */ 
/*   02/25/2001 Wendell Buckner                                                         */ 
/*    This texture pointer is no longer valid under directx 7.  Set it to TRUE so there */ 
/*    is something there when  the code does assert checks.                             */ 
/*   07/16/2000 Wendell Buckner                                                         */ 
/*    Convert to Directx7...                                                            */ 
/*                                                                                      */
/*  The contents of this file are subject to the Genesis3D Public License               */
/*  Version 1.01 (the "License"); you may not use this file except in                   */
/*  compliance with the License. You may obtain a copy of the License at                */
/*  http://www.genesis3d.com                                                            */
/*                                                                                      */
/*  Software distributed under the License is distributed on an "AS IS"                 */
/*  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See                */
/*  the License for the specific language governing rights and limitations              */
/*  under the License.                                                                  */
/*                                                                                      */
/*  The Original Code is Genesis3D, released March 25, 1999.                            */
/*Genesis3D Version 1.1 released November 15, 1999                            */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/
#include <Windows.h>
#include <stdio.h>

#include "D3DCache.h"
#include "D3D_Fx.h"

#include "PCache.h"

/*  07/16/2000 Wendell Buckner
/*   Convert to Directx7...    
#include "D3DDrv.h"              */
#include "D3DDrv7x.h"

#include "THandle.h"
#include "D3D_Err.h"

/*   01/28/2003 Wendell Buckner                                                         */
/*    Cache decals so that they can be drawn after all the 3d stuff...                  */
#include "Render.h"

//#define D3D_MANAGE_TEXTURES
#define SUPER_FLUSH

//====================================================================================
//	Local static variables
//====================================================================================

DRV_CacheInfo						CacheInfo;

//
//	World Cache
//

#if 1

#define MAX_WORLD_POLYS				256
#define MAX_WORLD_POLY_VERTS		1024

#define MAX_MISC_POLYS				256
#define MAX_MISC_POLY_VERTS			1024

// changed QD Shadows
#define MAX_STENCIL_POLYS			2048
#define MAX_STENCIL_POLY_VERTS		8192
// end change
#else

#define MAX_WORLD_POLYS				256
#define MAX_WORLD_POLY_VERTS		4096

#define MAX_MISC_POLYS				256
#define MAX_MISC_POLY_VERTS			4096

// changed QD Shadows
#define MAX_STENCIL_POLYS			256
#define MAX_STENCIL_POLY_VERTS		4096
// end change
#endif



/*  01/28/2003 Wendell Buckner                                                          */
/*   Cache decals so that they can be drawn after all the 3d stuff...                   */
#define MAX_DECAL_RECTS             256

typedef struct
{
 geRDriver_THandle *THandle;
 RECT  SRect;
 int32 x;
 int32 y;
} Decal_Rect;

typedef struct
{
	Decal_Rect		Decals[MAX_DECAL_RECTS];
	int32			NumDecals;
} Decal_Cache;

static Decal_Cache		DecalCache;



typedef struct
{
	float		u;
	float		v;
	//float		a;
	uint32		Color;
} PCache_TVert;

typedef struct
{
	geRDriver_THandle	*THandle;

	DRV_LInfo	*LInfo;						// Original pointer to linfo
	uint32		Flags;						// Flags for this poly
	float		ShiftU;
	float		ShiftV;
	float		ScaleU;
	float		ScaleV;
	int32		MipLevel;
	uint32		SortKey;
	int32		FirstVert;
	int32		NumVerts;
} World_Poly;

#define MAX_TEXTURE_STAGES		2			// Up to 2 tmu's (stages)

// Verts we defined in the D3D flexible vertex format (FVF)
// This is a transformed and lit vertex definition, with up to 8 sets of uvs
typedef struct
{
	float			u,v;
} PCache_UVSet;

typedef struct
{
	float			x,y,z;					// Screen x, y, z
	float			rhw;					// homogenous w
	DWORD			color;					// color
	DWORD			specular;
	PCache_UVSet	uv[MAX_TEXTURE_STAGES];	// uv sets for each stage
} PCache_Vert;

typedef struct
{
	World_Poly		Polys[MAX_WORLD_POLYS];
	World_Poly		*SortedPolys[MAX_WORLD_POLYS];
	World_Poly		*SortedPolys2[MAX_WORLD_POLYS];

/* 04/21/2003 Wendell Buckner
	Send World Poly's as triangle lists instead of fans... */
	World_Poly		*SortedPolys3[MAX_WORLD_POLYS];

	PCache_Vert		Verts[MAX_WORLD_POLY_VERTS];

	PCache_TVert	TVerts[MAX_WORLD_POLY_VERTS];		// Original uv

	int32			NumPolys;
	int32			NumPolys2;

/* 04/21/2003 Wendell Buckner
	Send World Poly's as triangle lists instead of fans... */
	int32			NumPolys3;

	int32			NumVerts;
} World_Cache;

static World_Cache		WorldCache;

#define PREP_WORLD_VERTS_NORMAL			1				// Prep verts as normal
#define PREP_WORLD_VERTS_LMAP			2				// Prep verts as lightmaps
#define PREP_WORLD_VERTS_SINGLE_PASS	3				// Prep verts for a single pass

#define RENDER_WORLD_POLYS_NORMAL		1				// Render polys as normal
#define RENDER_WORLD_POLYS_LMAP			2				// Render polys as lightmaps
#define RENDER_WORLD_POLYS_SINGLE_PASS	3

//
//	Misc cache
//

typedef struct
{
	geRDriver_THandle	*THandle;
	uint32		Flags;						// Flags for this poly
	int32		MipLevel;
	int32		FirstVert;
	int32		NumVerts;

	uint32		SortKey;
} Misc_Poly;

typedef struct
{
	Misc_Poly		Polys[MAX_MISC_POLYS];
	Misc_Poly		*SortedPolys[MAX_MISC_POLYS];
	PCache_Vert		Verts[MAX_MISC_POLY_VERTS];
	//float			ZVert[MAX_MISC_POLY_VERTS];

	int32			NumPolys;
	int32			NumVerts;
} Misc_Cache;

static Misc_Cache		MiscCache;

// changed QD Shadows
//
//	Stencil cache
//

typedef struct
{
	uint32		Flags;						// Flags for this poly
	int32		FirstVert;
	int32		NumVerts;
} Stencil_Poly;

typedef struct
{
	Stencil_Poly	Polys[MAX_STENCIL_POLYS];
	D3DXYZRHWVERTEX	Verts[MAX_STENCIL_POLY_VERTS];

	int32			NumPolys;
	int32			NumVerts;
} Stencil_Cache;

static Stencil_Cache		StencilCache;
// end change
//====================================================================================
//	Local static functions prototypes
//====================================================================================
geBoolean World_PolyPrepVerts(World_Poly *pPoly, int32 PrepMode, int32 Stage1, int32 Stage2);

static BOOL RenderWorldPolys(int32 RenderMode);
static BOOL ClearWorldCache(void);
static int32 GetMipLevel(DRV_TLVertex *Verts, int32 NumVerts, float ScaleU, float ScaleV, int32 MaxMipLevel);

#include <Math.h>

/* 01/03/2004 Wendell Buckner
    CONFIG DRIVER - Make the driver configurable by "ini" file settings */
extern DWORD BltSurfFlags;

/*  01/28/2003 Wendell Buckner                                                          */
/*   Cache decals so that they can be drawn after all the 3d stuff...                   */
BOOL DCache_FlushDecalRects(void)
{
	int32				i;
	Decal_Rect			*pRect;

	if (!DecalCache.NumDecals)
		return TRUE;

	if (!THandle_CheckCache())
		return GE_FALSE;

	for (i=0; i< DecalCache.NumDecals; i++)
	{
	  pRect = &DecalCache.Decals[i];

	  if ( pRect->SRect.bottom == -1 )
	  {
       if ( !DrawDecal(pRect->THandle, NULL, pRect->x, pRect->y) )
	   {
	      D3DMain_Log("DCache_FlushDecalRects: Failed\n");
		  return FALSE;
	   }
	  }
	  else
	  {
	   if ( !DrawDecal(pRect->THandle, &pRect->SRect, pRect->x, pRect->y) )
	   {
	      D3DMain_Log("DCache_FlushDecalRects: Failed\n");
		  return FALSE;
	   }
	  }

    }

	DecalCache.NumDecals = 0;

	return TRUE;
}

BOOL DCache_InsertDecalRect(geRDriver_THandle *THandle, RECT *SRect, int32 x, int32 y)
{
	Decal_Rect		*pDecalCacheRect;

	if ( DecalCache.NumDecals == MAX_DECAL_RECTS)
	{
		// If the cache is full, we must flush it before going on...
		DCache_FlushDecalRects();
	}

	// Store info about this poly in the cache
	pDecalCacheRect = &DecalCache.Decals[DecalCache.NumDecals];

    pDecalCacheRect->THandle      = THandle;

    pDecalCacheRect->SRect.bottom = -1;
	pDecalCacheRect->SRect.left   = -1;
	pDecalCacheRect->SRect.right  = -1;
	pDecalCacheRect->SRect.top    = -1;

    if( SRect != NULL )
	{
      pDecalCacheRect->SRect.bottom = SRect->bottom;
	  pDecalCacheRect->SRect.left   = SRect->left;
	  pDecalCacheRect->SRect.right  = SRect->right;
	  pDecalCacheRect->SRect.top    = SRect->top;
	}

    pDecalCacheRect->x            = x;
    pDecalCacheRect->y            = y;

	// Update globals about the misc poly cache
	DecalCache.NumDecals++;

	return TRUE;
}

//====================================================================================
//	PCache_InsertWorldPoly
//====================================================================================
BOOL PCache_InsertWorldPoly(DRV_TLVertex *Verts, int32 NumVerts, geRDriver_THandle *THandle, DRV_TexInfo *TexInfo, DRV_LInfo *LInfo, uint32 Flags)
{
	int32			Mip;
	float			ZRecip, DrawScaleU, DrawScaleV;
	World_Poly		*pCachePoly;
	DRV_TLVertex	*pVerts;
	PCache_TVert	*pTVerts;
	PCache_Vert		*pD3DVerts;
	int32			i;
	uint32			Alpha;

	#ifdef _DEBUG
		if (LInfo)
		{
			assert(LInfo->THandle);
		}
	#endif

	if ((WorldCache.NumVerts + NumVerts) >= MAX_WORLD_POLY_VERTS)
	{
		// If the cache is full, we must flush it before going on...
		if (!PCache_FlushWorldPolys())
			return GE_FALSE;
	}
	else if (WorldCache.NumPolys+1 >= MAX_WORLD_POLYS)
	{
		// If the cache is full, we must flush it before going on...
		if (!PCache_FlushWorldPolys())
			return GE_FALSE;
	}

	DrawScaleU = 1.0f / TexInfo->DrawScaleU;
	DrawScaleV = 1.0f / TexInfo->DrawScaleV;

	Mip = GetMipLevel(Verts, NumVerts, DrawScaleU, DrawScaleV, THandle->NumMipLevels-1);

	// Get a pointer to the original polys verts
	pVerts = Verts;
	
	// Store info about this poly in the cache
	pCachePoly = &WorldCache.Polys[WorldCache.NumPolys];

	pCachePoly->THandle = THandle;
	pCachePoly->LInfo = LInfo;
	pCachePoly->Flags = Flags;
	pCachePoly->FirstVert = WorldCache.NumVerts;
	pCachePoly->NumVerts = NumVerts;
	pCachePoly->ShiftU = TexInfo->ShiftU;
	pCachePoly->ShiftV = TexInfo->ShiftV;
	pCachePoly->ScaleU = DrawScaleU;
	pCachePoly->ScaleV = DrawScaleV;
	pCachePoly->MipLevel = Mip;

	// Don't forget the sort key:
	pCachePoly->SortKey = ((THandle - TextureHandles)<<4)+Mip;

	// Get a pointer into the world verts
	pD3DVerts = &WorldCache.Verts[WorldCache.NumVerts];
	pTVerts = &WorldCache.TVerts[WorldCache.NumVerts];

	if (Flags & DRV_RENDER_ALPHA)
		Alpha = (uint32)pVerts->a<<24;
	else
		Alpha = (uint32)(255<<24);

	for (i=0; i< NumVerts; i++)
	{
		ZRecip = 1.0f/(pVerts->z);

		pD3DVerts->x = pVerts->x;
		pD3DVerts->y = pVerts->y;

		pD3DVerts->z = (1.0f - ZRecip);	// ZBUFFER
		pD3DVerts->rhw = ZRecip;

		if (AppInfo.FogEnable && !(Flags & DRV_RENDER_POLY_NO_FOG)) // poly fog
		{
			DWORD	FogVal;
			float	Val;

			Val = pVerts->z;

			if (Val > AppInfo.FogEnd)
				Val = AppInfo.FogEnd;

			FogVal = (DWORD)((AppInfo.FogEnd-Val)/(AppInfo.FogEnd-AppInfo.FogStart)*255.0f);
		
			if (FogVal < 0)
				FogVal = 0;
			else if (FogVal > 255)
				FogVal = 255;
		
			pD3DVerts->specular = (FogVal<<24);		// Alpha component in specular is the fog value (0...255)
		}
		else
			pD3DVerts->specular = 0;
		
		// Store the uv's so the prep pass can use them...
		pTVerts->u = pVerts->u;
		pTVerts->v = pVerts->v;

		pTVerts->Color = Alpha | ((uint32)pVerts->r<<16) | ((uint32)pVerts->g<<8) | (uint32)pVerts->b;

		pTVerts++;
		pVerts++;	 
		pD3DVerts++;

	}
	
	// Update globals about the world poly cache
	WorldCache.NumVerts += NumVerts;
	WorldCache.NumPolys++;

	return TRUE;
}

//====================================================================================
//	PCache_FlushWorldPolys
//====================================================================================
BOOL PCache_FlushWorldPolys(void)
{
	if (!WorldCache.NumPolys)
		return TRUE;

	if (!THandle_CheckCache())
		return GE_FALSE;
	
	if (AppInfo.CanDoMultiTexture)
	{
		RenderWorldPolys(RENDER_WORLD_POLYS_SINGLE_PASS);
	}
	else
	{
		// Render them as normal
		if (!RenderWorldPolys(RENDER_WORLD_POLYS_NORMAL))
			return GE_FALSE;

		// Render them as lmaps
		RenderWorldPolys(RENDER_WORLD_POLYS_LMAP);
	}

	ClearWorldCache();

	return TRUE;
}

//====================================================================================
//====================================================================================
static int MiscBitmapHandleComp(const void *a, const void *b)
{
	uint32	Id1, Id2;

	Id1 = (uint32)(*(Misc_Poly**)a)->SortKey;
	Id2 = (uint32)(*(Misc_Poly**)b)->SortKey;

	if ( Id1 == Id2)
		return 0;

	if (Id1 < Id2)
		return -1;

	return 1;
}

//====================================================================================
//====================================================================================
static void SortMiscPolysByHandle(void)
{
	Misc_Poly	*pPoly;
	int32		i;

	pPoly = MiscCache.Polys;

	for (i=0; i<MiscCache.NumPolys; i++)
	{
		MiscCache.SortedPolys[i] = pPoly;
		pPoly++;
	}
	
	// Sort the polys
	qsort(&MiscCache.SortedPolys, MiscCache.NumPolys, sizeof(MiscCache.SortedPolys[0]), MiscBitmapHandleComp);
}

//=====================================================================================
//	FillLMapSurface
//=====================================================================================
static void FillLMapSurface(DRV_LInfo *LInfo, int32 LNum)
{
	U16					*pTempBits;
	int32				w, h, Width, Height, Size;
	U8					*pBitPtr;
	RGB_LUT				*Lut;
	geRDriver_THandle	*THandle;
	int32				Extra;

/* 01/10/2003 Wendell Buckner
     Allow 32-bit color depth lightmaps...  */
	U32					*pTempBits2;

	THandle = LInfo->THandle;

	pBitPtr = (U8*)LInfo->RGBLight[LNum];

	Width = LInfo->Width;
	Height = LInfo->Height;
	Size = 1<<THandle->Log;

/* 01/10/2003 Wendell Buckner
     Allow 32-bit color depth lightmaps... 
	Lut = &AppInfo.Lut1;
	THandle_Lock(THandle, 0, (void**)&pTempBits);                 */
	if ( AppInfo.ddTexFormat.ddpfPixelFormat.dwRGBBitCount == 32 )
	{
	 Lut = &AppInfo.Lut4;
     THandle_Lock(THandle, 0, (void**)&pTempBits2);
	}
    else
	{
	 Lut = &AppInfo.Lut1;
	 THandle_Lock(THandle, 0, (void**)&pTempBits);
	}
	
	Extra = Size - Width;

	for (h=0; h< Height; h++)
	{
		for (w=0; w< Width; w++)
		{
			U8	R, G, B;
			U16	Color;

/* 01/10/2003 Wendell Buckner
     Allow 32-bit color depth lightmaps... */
			U8  A = 0;
			U32 Color2;

			R = *pBitPtr++;
			G = *pBitPtr++;
			B = *pBitPtr++;

/* 01/10/2003 Wendell Buckner
     Allow 32-bit color depth lightmaps... 
			Color = (U16)(Lut->R[R] | Lut->G[G] | Lut->B[B]);
		    *pTempBits++  = Color;                                       */
	        if ( AppInfo.ddTexFormat.ddpfPixelFormat.dwRGBBitCount == 32 )
			{ 			
             Color2 = (U32)(Lut->A[A] | Lut->R[R] | Lut->G[G] | Lut->B[B]);
			 *pTempBits2++ = Color2;
			}
			else
			{
			 Color = (U16)(Lut->R[R] | Lut->G[G] | Lut->B[B]);
			 *pTempBits++  = Color;
			}
			
		}

/* 01/10/2003 Wendell Buckner
     Allow 32-bit color depth lightmaps... 
		pTempBits += Extra;                */
		if ( AppInfo.ddTexFormat.ddpfPixelFormat.dwRGBBitCount == 32 )
		 pTempBits2 += Extra;
		else
		 pTempBits  += Extra;

	}

	THandle_UnLock(THandle, 0);
}

#ifdef USE_TPAGES
//=====================================================================================
//	FillLMapSurface
//=====================================================================================
static void FillLMapSurface2(DRV_LInfo *LInfo, int32 LNum)
{
	U16					*pTempBits;
	int32				w, h, Width, Height, Stride;
	U8					*pBitPtr;
	RGB_LUT				*Lut;
	geRDriver_THandle	*THandle;
	HRESULT				Result;
	const RECT			*pRect;
    DDSURFACEDESC2		SurfDesc;

/* 01/10/2003 Wendell Buckner
     Allow 32-bit color depth lightmaps...  */
	U32					*pTempBits2;

/* 07/16/2000 Wendell Buckner
    Convert to Directx7...    
	LPDIRECTDRAWSURFACE4	Surface; */
	LPDIRECTDRAWSURFACE7	Surface;

	int32				Extra;

	THandle = LInfo->THandle;

	pBitPtr = (U8*)LInfo->RGBLight[LNum];

	Width = LInfo->Width;
	Height = LInfo->Height;

/* 01/10/2003 Wendell Buckner
     Allow 32-bit color depth lightmaps... 
	Lut = &AppInfo.Lut1;                   */
	if ( AppInfo.ddTexFormat.ddpfPixelFormat.dwRGBBitCount == 32 )
     Lut = &AppInfo.Lut4;
	else
	 Lut = &AppInfo.Lut1;

    pRect = TPage_BlockGetRect(THandle->Block);
	Surface = TPage_BlockGetSurface(THandle->Block);

    memset(&SurfDesc, 0, sizeof(DDSURFACEDESC2));
    SurfDesc.dwSize = sizeof(DDSURFACEDESC2);

/* 03/10/2002 Wendell Buckner
    Procedural Textures
    if you must lock a texture specify the flags WRITEONLY and DISCARDCONTENTS when 
	Result = Surface->Lock((RECT*)pRect, &SurfDesc, DDLOCK_WAIT, NULL); */
    Result = Surface->Lock((RECT*)pRect, &SurfDesc, DDLOCK_WAIT | DDLOCK_WRITEONLY | DDLOCK_DISCARDCONTENTS , NULL);

	assert(Result == DD_OK);

	Stride = SurfDesc.dwWidth;

/* 01/10/2003 Wendell Buckner
     Allow 32-bit color depth lightmaps... 
	pTempBits = (U16*)SurfDesc.lpSurface; */
    if ( AppInfo.ddTexFormat.ddpfPixelFormat.dwRGBBitCount == 32 )
     pTempBits2 = (U32*) SurfDesc.lpSurface;
    else
	 pTempBits  = (U16*) SurfDesc.lpSurface;

	Extra = Stride - Width; 

	for (h=0; h< Height; h++)
	{
		for (w=0; w< Width; w++)
		{
			U8	R, G, B;
			U16	Color;

/* 01/10/2003 Wendell Buckner
     Allow 32-bit color depth lightmaps... */
			U8 A = 0;
			U32 Color2;

			R = *pBitPtr++;
			G = *pBitPtr++;
			B = *pBitPtr++;

/* 01/10/2003 Wendell Buckner
     Allow 32-bit color depth lightmaps... 
			Color = (U16)(Lut->R[R] | Lut->G[G] | Lut->B[B]);
			*pTempBits++  = Color;                                       */
	        if ( AppInfo.ddTexFormat.ddpfPixelFormat.dwRGBBitCount == 32 )
			{ 						
             Color2 = (U32)(Lut->A[A] | Lut->R[R] | Lut->G[G] | Lut->B[B]);
			 *pTempBits2++ = Color2;
			}
			else
			{
			 Color = (U16)(Lut->R[R] | Lut->G[G] | Lut->B[B]);
			 *pTempBits++ = Color;
			}

		}

/* 01/10/2003 Wendell Buckner
     Allow 32-bit color depth lightmaps... 
		pTempBits += Extra;                */
		if ( AppInfo.ddTexFormat.ddpfPixelFormat.dwRGBBitCount == 32 )
		 pTempBits2 += Extra;
		else
		 pTempBits  += Extra;
	}

    Result = Surface->Unlock((RECT*)pRect);

	assert(Result == DD_OK);
}
#endif

//=====================================================================================
//	LoadLMapFromSystem
//=====================================================================================
static void LoadLMapFromSystem(DRV_LInfo *LInfo, int32 Log, int32 LNum)
{
	U16					*pTempBits;
	int32				w, h, Width, Height, Size, Extra;
	U8					*pBitPtr;

/* 01/10/2003 Wendell Buckner
     Allow 32-bit color depth lightmaps...  */
	U32					*pTempBits2;

/*   07/16/2000 Wendell Buckner                                                          
/*    Convert to Directx7...                                                             
	LPDIRECTDRAWSURFACE4 Surface;                                                       */
	LPDIRECTDRAWSURFACE7 Surface;

	RGB_LUT				*Lut;
    DDSURFACEDESC2		ddsd;
    HRESULT				ddrval;

	pBitPtr = (U8*)LInfo->RGBLight[LNum];

	Width = LInfo->Width;
	Height = LInfo->Height;
	Size = 1<<Log;

	Extra = Size - Width;

/* 01/10/2003 Wendell Buckner
     Allow 32-bit color depth lightmaps... 
	Lut = &AppInfo.Lut1;                   */
	if ( AppInfo.ddTexFormat.ddpfPixelFormat.dwRGBBitCount == 32 )
     Lut = &AppInfo.Lut4;
	else
	 Lut = &AppInfo.Lut1;

	Surface = SystemToVideo[Log].Surface;

    memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
    ddsd.dwSize = sizeof(DDSURFACEDESC2);

/* 03/10/2002 Wendell Buckner
    Procedural Textures
    if you must lock a texture specify the flags WRITEONLY and DISCARDCONTENTS when 
    ddrval = Surface->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL);*/
    ddrval = Surface->Lock(NULL, &ddsd, DDLOCK_WAIT | DDLOCK_WRITEONLY | DDLOCK_DISCARDCONTENTS, NULL);

	assert(ddrval == DD_OK);

/* 01/10/2003 Wendell Buckner
     Allow 32-bit color depth lightmaps... 
	pTempBits = (USHORT*)ddsd.lpSurface;   */
    if ( AppInfo.ddTexFormat.ddpfPixelFormat.dwRGBBitCount == 32 )
     pTempBits2 = (ULONG*)  ddsd.lpSurface;	 
	else
	 pTempBits  = (USHORT*) ddsd.lpSurface;	 

	for (h=0; h< Height; h++)
	{
		for (w=0; w< Width; w++)
		{
			U8	R, G, B;
			U16	Color;

/* 01/10/2003 Wendell Buckner
     Allow 32-bit color depth lightmaps... */
			U8 A = 0;
			U32 Color2;

			R = *pBitPtr++;
			G = *pBitPtr++;
			B = *pBitPtr++;
			
/* 01/10/2003 Wendell Buckner
     Allow 32-bit color depth lightmaps... 
			Color = (U16)(Lut->R[R] | Lut->G[G] | Lut->B[B]);
			*pTempBits++  = Color;                                       */
	        if ( AppInfo.ddTexFormat.ddpfPixelFormat.dwRGBBitCount == 32 )
			{ 						
             Color2 = (U32)(Lut->A[A] | Lut->R[R] | Lut->G[G] | Lut->B[B]);
			 *pTempBits2++ = Color2;
			}
			else
			{				
			 Color = (U16)(Lut->R[R] | Lut->G[G] | Lut->B[B]);
			 *pTempBits++ = Color;
			}

		}

/* 01/10/2003 Wendell Buckner
     Allow 32-bit color depth lightmaps... 
		pTempBits += Extra;                */
		if ( AppInfo.ddTexFormat.ddpfPixelFormat.dwRGBBitCount == 32 )
		 pTempBits2 += Extra;
		else
		 pTempBits  += Extra;
	}

    ddrval = Surface->Unlock(NULL);
	assert(ddrval == DD_OK);
}

static BOOL IsKeyDown(int KeyCode)
{
	if (GetAsyncKeyState(KeyCode) & 0x8000)
		return TRUE;

	return FALSE;
}

extern uint32 CurrentLRU;

//=====================================================================================
//	SetupMipData
//=====================================================================================
geBoolean SetupMipData(THandle_MipData *MipData)
{
	if (!MipData->Slot || D3DCache_SlotGetUserData(MipData->Slot) != MipData)
	{
		MipData->Slot = D3DCache_TypeFindSlot(MipData->CacheType);
		assert(MipData->Slot);

		D3DCache_SlotSetUserData(MipData->Slot, MipData);


	#ifdef SUPER_FLUSH
/* 02/28/2001 Wendell Buckner
   These render states are unsupported d3d 7.0
		AppInfo.lpD3DDevice->SetRenderState(D3DRENDERSTATE_FLUSHBATCH, 0);*/		
	#endif

		return GE_FALSE;
	}

	return GE_TRUE;
}

//=====================================================================================
//	SetupLMap
//=====================================================================================
geBoolean SetupLMap(int32 Stage, DRV_LInfo *LInfo, int32 LNum, geBoolean Dynamic)
{
#ifdef D3D_MANAGE_TEXTURES
	#ifdef USE_TPAGES
	{
		geRDriver_THandle		*THandle;

		THandle = LInfo->THandle;

		if (Dynamic)
			THandle->Flags |= THANDLE_UPDATE;

		if (!THandle->Block)
		{
			THandle->Block = TPage_MgrFindOptimalBlock(TPageMgr, CurrentLRU);
			THandle->Flags |= THANDLE_UPDATE;
			TPage_BlockSetUserData(THandle->Block, THandle);
			assert(THandle->Block);
		}
		else if (TPage_BlockGetUserData(THandle->Block) != THandle)
		{
			// Find another block
			THandle->Block = TPage_MgrFindOptimalBlock(TPageMgr, CurrentLRU);
			assert(THandle->Block);

			THandle->Flags |= THANDLE_UPDATE;
			TPage_BlockSetUserData(THandle->Block, THandle);
		}

		if (THandle->Flags & THANDLE_UPDATE)
			FillLMapSurface2(LInfo, LNum);

		TPage_BlockSetLRU(THandle->Block, CurrentLRU);
		D3DSetTexture(Stage, TPage_BlockGetTexture(THandle->Block));
	
		if (Dynamic)
			THandle->Flags |= THANDLE_UPDATE;
		else
			THandle->Flags &= ~THANDLE_UPDATE;

		return GE_TRUE;
	}
	#else
	{
		geRDriver_THandle		*THandle;

		THandle = LInfo->THandle;

		if (Dynamic)
			THandle->MipData[0].Flags |= THANDLE_UPDATE;

		if (THandle->MipData[0].Flags & THANDLE_UPDATE)
			FillLMapSurface(LInfo, LNum);

/*   02/25/2001 Wendell Buckner
/*    This texture pointer is no longer valid under directx 7.  Set it to TRUE so there is
/*    something there when  the code does assert checks.
		D3DSetTexture(Stage, THandle->MipData[0].Texture);*/
		D3DSetTexture(Stage, THandle->MipData[0].Surface);
	
		if (Dynamic)
			THandle->MipData[0].Flags |= THANDLE_UPDATE;
		else
			THandle->MipData[0].Flags &= ~THANDLE_UPDATE;

		return GE_TRUE;
	}
	#endif

#else
	geRDriver_THandle	*THandle;
	THandle_MipData		*MipData;

	THandle = LInfo->THandle;
	MipData = &THandle->MipData[0];

	if (Dynamic)
		MipData->Flags |= THANDLE_UPDATE;

	if (!SetupMipData(MipData))
	{
		MipData->Flags |= THANDLE_UPDATE;		// Force an upload
		CacheInfo.LMapMisses++;
	}

	if (MipData->Flags & THANDLE_UPDATE)
	{
		HRESULT					Error;

/* 07/16/2000 Wendell Buckner
    Convert to Directx7...    
        LPDIRECTDRAWSURFACE4	Surface; */
		LPDIRECTDRAWSURFACE7	Surface;
		

		assert(MipData->Slot);
		
		Surface = D3DCache_SlotGetSurface(MipData->Slot);

		assert(Surface);
		assert(THandle->Log < MAX_LMAP_LOG_SIZE);
		assert(SystemToVideo[THandle->Log].Surface);

		LoadLMapFromSystem(LInfo, THandle->Log, LNum);

/*  03/10/2002 Wendell Buckner
     Optimization from GeForce_Optimization2.doc                                        
     Procedural Textures
     Also, Load is even faster than BLT under Dx7.
	 Error = AppInfo.lpD3DDevice->Load ( Surface, NULL, SystemToVideo[THandle->Log].Surface, NULL, NULL );  */

/* 01/03/2004 Wendell Buckner
    CONFIG DRIVER - Make the driver configurable by "ini" file settings */
/*	01/24/2002 Wendell Buckner
    Change flags for speed... (DDBLT_ASYNC) is not a valid parameter
		Error = Surface->BltFast(0, 0, SystemToVideo[THandle->Log].Surface, NULL, DDBLTFAST_WAIT); *
		Error = Surface->BltFast(0, 0, SystemToVideo[THandle->Log].Surface, NULL, DDBLTFAST_DONOTWAIT);*/
        Error = Surface->BltFast(0, 0, SystemToVideo[THandle->Log].Surface, NULL, BltSurfFlags);
        
		
		//Error = Surface->BltFast(0, 0, SystemToVideo[THandle->Log].Surface, NULL, 0);
		//Error = Surface->Blt(NULL, SystemToVideo[THandle->Log].Surface, NULL, DDBLT_WAIT, NULL);
		//Error = Surface->Blt(NULL, SystemToVideo[THandle->Log].Surface, NULL, 0, NULL);
		
		if (Error != DD_OK)
		{
			if(Error==DDERR_SURFACELOST)
			{
				if (!D3DMain_RestoreAllSurfaces())
					return GE_FALSE;
			}
			else
			{
				D3DMain_Log("SetupTexture: System to Video cache Blt failed.\n %s", D3DErrorToString(Error));
				return GE_FALSE;
			}
		}
	}

	if (Dynamic)		// If it was dynmamic, force an update for one more frame
		MipData->Flags |= THANDLE_UPDATE;
	else
		MipData->Flags &= ~THANDLE_UPDATE;

	D3DCache_SlotSetLRU(MipData->Slot, CurrentLRU);
	D3DSetTexture(Stage, D3DCache_SlotGetTexture(MipData->Slot));

	return GE_TRUE;
#endif
}

//=====================================================================================
//	SetupTexture
//=====================================================================================
geBoolean SetupTexture(int32 Stage, geRDriver_THandle *THandle, int32 MipLevel)
{
#ifdef D3D_MANAGE_TEXTURES
	D3DSetTexture(Stage, THandle->MipData[MipLevel].Texture);
	return GE_TRUE;
#else
	THandle_MipData		*MipData;

	MipData = &THandle->MipData[MipLevel];
	
	if (!SetupMipData(MipData))
	{
		MipData->Flags |= THANDLE_UPDATE;		// Force an upload
		CacheInfo.TexMisses++;
	}

	if (MipData->Flags & THANDLE_UPDATE)
	{
		HRESULT					Error;

/* 07/16/2000 Wendell Buckner
    Convert to Directx7...    
        LPDIRECTDRAWSURFACE4	Surface; */
		LPDIRECTDRAWSURFACE7	Surface;
		

		Surface = D3DCache_SlotGetSurface(MipData->Slot);

/*  03/10/2002 Wendell Buckner
     Optimization from GeForce_Optimization2.doc                                        
     Procedural Textures
     Also, Load is even faster than BLT under Dx7.
     Error = AppInfo.lpD3DDevice->Load ( Surface, NULL, MipData->Surface, NULL, NULL );*/

/* 01/03/2004 Wendell Buckner
    CONFIG DRIVER - Make the driver configurable by "ini" file settings */
/*	01/24/2002 Wendell Buckner
    Change flags for speed... (DDBLT_ASYNC) is not a valid parameter
		Error = Surface->BltFast(0, 0, MipData->Surface, NULL, DDBLTFAST_WAIT);*
		Error = Surface->BltFast(0, 0, MipData->Surface, NULL, DDBLTFAST_DONOTWAIT);*/
		Error = Surface->BltFast(0, 0, MipData->Surface, NULL, BltSurfFlags);


		if (Error != DD_OK)
		{
			if(Error==DDERR_SURFACELOST)
			{
				if (!D3DMain_RestoreAllSurfaces())
					return FALSE;
			}
			else
			{
				D3DMain_Log("SetupTexture: System to Video cache Blt failed.\n %s", D3DErrorToString(Error));
				return GE_FALSE;
			}
		}
	}

	MipData->Flags &= ~THANDLE_UPDATE;

	D3DCache_SlotSetLRU(MipData->Slot, CurrentLRU);
	D3DSetTexture(Stage, D3DCache_SlotGetTexture(MipData->Slot));

	return GE_TRUE;
#endif
}

//====================================================================================
//	PCache_FlushMiscPolys
//====================================================================================

/*	04/23/2003 Wendell Buckner
	Send World Poly's as triangle lists instead of fans... */
static PCache_Vert		Verts2[MAX_MISC_POLY_VERTS];

inline DWORD F2DW( FLOAT f ) { return *((DWORD*)&f); }


BOOL PCache_FlushMiscPolys(void)
{
	int32				i;
	Misc_Poly			*pPoly;

/*	04/23/2003 Wendell Buckner
	Send Misc Poly's as triangle lists instead of fans... */
	Misc_Poly			*pPoly2   = NULL;
    int32				vi        = 0;
    int32               VertCount = 0;
	int32               MaxVerts  = 0; 
	int32			    MatchPolyCount = 0;
	int32				MatchVertCount =  0;
	int32				PolyCount = 0;
	BOOL				LastPoly  = FALSE;
	BOOL				MatchPoly = FALSE;
	BOOL				PendPoly  = FALSE;
	BOOL				AddPoly   = FALSE;
    BOOL				FlushPoly = FALSE;
    BOOL				OnlyPoly = FALSE;
	BOOL				FullPoly = FALSE;

	if (!MiscCache.NumPolys)
		return TRUE;

	if (!THandle_CheckCache())
		return GE_FALSE;

	// Set the render states
	if (AppInfo.CanDoMultiTexture)
	{

/* 04/08/2003 Wendell Buckner
    BUMPMAPPING
/* 01/18/2003 Wendell Buckner
    Optimization from GeForce_Optimization2.doc  
9.	Do not duplicate render state commands.  Worse is useless renderstates.  Do not set a renderstate unless it is needed. */
        D3DSetTextureStageState1(D3DTA_TEXTURE,D3DTA_CURRENT,D3DTOP_DISABLE,D3DTA_DIFFUSE,D3DTA_CURRENT,D3DTOP_DISABLE,1);

/* 08/24/2004 Wendell Buckner 
    No reason to clear the textures... just disable the texture stages your not using (D3DTOP_DISABLE)...
		D3DSetTexture(1, NULL);		// Reset texture stage 1
		D3DSetTexture(2, NULL);		// Reset texture stage 1*/
	}


/* 04/08/2003 Wendell Buckner
    BUMPMAPPING
/* 01/18/2003 Wendell Buckner
    Optimization from GeForce_Optimization2.doc  
9.	Do not duplicate render state commands.  Worse is useless renderstates.  Do not set a renderstate unless it is needed. 	*/
    D3DSetTextureStageState0(D3DTA_TEXTURE,D3DTA_DIFFUSE,D3DTOP_MODULATE,D3DTA_TEXTURE,D3DTA_DIFFUSE,D3DTOP_MODULATE,0);

	D3DBlendFunc (D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA);
	D3DBlendEnable(TRUE);

	// Sort the polys by handle
	SortMiscPolysByHandle();

	for (i=0; i< MiscCache.NumPolys; i++)
	{
		pPoly = MiscCache.SortedPolys[i];

		if (pPoly->Flags & DRV_RENDER_NO_ZMASK)		// We are assuming that this is not going to change all that much
			D3DZEnable(FALSE);
		else
			D3DZEnable(TRUE);

		if (pPoly->Flags & DRV_RENDER_NO_ZWRITE)	// We are assuming that this is not going to change all that much
			D3DZWriteEnable(FALSE);	
		else
			D3DZWriteEnable(TRUE);
									  
		if (pPoly->Flags & DRV_RENDER_CLAMP_UV)
			D3DTexWrap(0, FALSE);
		else
			D3DTexWrap(0, TRUE);

/* 04/08/2003 Wendell Buckner
    BUMPMAPPING */
		if (pPoly->Flags & DRV_RENDER_CLAMP_UV)
			D3DTexWrap(1, FALSE);
		else
			D3DTexWrap(1, TRUE);

		if (pPoly->Flags & DRV_RENDER_CLAMP_UV)
			D3DTexWrap(2, FALSE);
		else
			D3DTexWrap(2, TRUE);

/* 03/02/2004 Wendell Buckner
    DOT3BUMPMAPPING
	The bump thandles have to be manipulated because base map has to come be assigned
	to the second texture unit! argh! this is getting messy!
		 if (!SetupTexture(0, pPoly->THandle, pPoly->MipLevel))
			return GE_FALSE;		*/		
		{         
			int j = 0;

			do
			{
				if ( !(pPoly->Flags & DRV_RENDER_DOT3BUMPMAP) ) break;

				geRDriver_THandle	*pBumpTHandle = pPoly->THandle->NextTHandle;

				if ( !pBumpTHandle ) break;

				if ( !SetupTexture(j, pBumpTHandle, 0) ) return GE_FALSE;

				j++;
			} while(GE_FALSE);

			if (!SetupTexture(j, pPoly->THandle, pPoly->MipLevel))	return GE_FALSE;
		}

/*  01/18/2003 Wendell Buckner                                                         
     To continually turn this off and on really is ineffecient... turn it on when you  
     need it turn if off when you don't   *
/*	01/13/2003 Wendell Buckner
    Optimization from GeForce_Optimization2.doc
    9.	Do not duplicate render state commands.  Worse is useless renderstates.  Do not set a renderstate unless it is needed. */
        if ( AppInfo.FogEnable ) // poly fog
		{
			if ( pPoly->Flags & DRV_RENDER_POLY_NO_FOG ) 
			 D3DFogEnable ( FALSE, 0  );
			else
			 D3DFogEnable ( TRUE, ((DWORD)AppInfo.FogR<<16)|((DWORD)AppInfo.FogG<<8)|(DWORD)AppInfo.FogB ); 
		}

/* 08/23/2003 Wendell Buckner
    BUMPMAPPING 
	 Don't allow bleeding of bumpmap effect onto actors that shouldn't receive the effect */

//BEGIN *******************************************************************************************

/*	01/01/2004 Wendell Buckner
    DOT3 BUMPMAPPING 
		if(pPoly->Flags & DRV_RENDER_BUMPMAP )			*/
		int32 MultiTextureFlags = (DRV_RENDER_BUMPMAP | DRV_RENDER_DOT3BUMPMAP);

		if ( pPoly->Flags & MultiTextureFlags )
		{
			geRDriver_THandle	*pTHandle;
			int32 j; 
			for(j=1, pTHandle = pPoly->THandle->NextTHandle; pTHandle ; pTHandle = pTHandle->NextTHandle,j++)
			{

/*	01/01/2004 Wendell Buckner
    DOT3 BUMPMAPPING */
				if ((pPoly->Flags & DRV_RENDER_DOT3BUMPMAP) && (j >= 3)) break;

				if ((pPoly->Flags & DRV_RENDER_BUMPMAP) && (j >= 3)) break;

/* 03/02/2004 Wendell Buckner
    DOT3BUMPMAPPING
	The bump thandles have to be manipulated because base map has to come be assigned
	to the second texture unit! argh! this is getting messy! */
				if ( (pPoly->Flags & DRV_RENDER_DOT3BUMPMAP) && (j < 2) ) continue;

/* 08/18/2003 Wendell Buckner
    BUMPMAPPING 
    TODO: Allow mip map levels greater than 0 */
				if (!SetupTexture(j, pTHandle, 0)) 
					return GE_FALSE;

				if(!(pPoly->Flags & DRV_RENDER_BUMPMAP)) break;
			}
        }
/* 08/24/2004 Wendell Buckner 
    No reason to clear the textures... just disable the texture stages your not using (D3DTOP_DISABLE)...
        else
		{
         D3DSetTexture(1, NULL);		// Reset texture stage 1
	     D3DSetTexture(2, NULL);		// Reset texture stage 2
		}*/

/*	01/01/2004 Wendell Buckner
    DOT3 BUMPMAPPING  
		if (pPoly->Flags & DRV_RENDER_BUMPMAP)*/
		if (pPoly->Flags & DRV_RENDER_DOT3BUMPMAP)
		{
			D3DBlendEnable(FALSE);
			D3DSetTextureStageState0(D3DTA_TEXTURE, D3DTA_DIFFUSE, D3DTOP_DOTPRODUCT3, D3DTA_TEXTURE, D3DTA_CURRENT, D3DTOP_DISABLE, 0);
			D3DSetTextureStageState1(D3DTA_TEXTURE, D3DTA_CURRENT, D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_CURRENT, D3DTOP_DISABLE, 0);
/* 08/24/2004 Wendell Buckner 
    No reason to clear the textures... just disable the texture stages your not using (D3DTOP_DISABLE)...*/
			D3DSetTextureStageState2(D3DTA_TEXTURE,D3DTA_CURRENT,D3DTOP_DISABLE,D3DTA_DIFFUSE,D3DTA_CURRENT,D3DTOP_DISABLE,0);
        } 
		else if (pPoly->Flags & DRV_RENDER_BUMPMAP)
		{
// Set the color operations and arguments to prepare for bump mapping.

// Stage 0: the base texture
			D3DSetTextureStageState0(D3DTA_TEXTURE, D3DTA_DIFFUSE, D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_CURRENT, D3DTOP_SELECTARG1, 0);

// Stage 1: the bump map 
			D3DSetTextureStageState1(D3DTA_TEXTURE, D3DTA_CURRENT, D3DTOP_BUMPENVMAPLUMINANCE, D3DTA_TEXTURE, D3DTA_CURRENT, D3DTOP_DISABLE, 0);

// Use luminance for this example.
			D3DSetTextureStageState2(D3DTA_TEXTURE, D3DTA_CURRENT, D3DTOP_ADD, D3DTA_TEXTURE, D3DTA_CURRENT, D3DTOP_DISABLE , 0);

    // Once the blending operations and arguments are set, the following code sets the 2�2 bump mapping matrix to the identity matrix, by setting the D3DTSS_BUMPENVMAT00 and D3DTSS_BUMPENVMAT11 texture stage states to 1.0. Setting the matrix to the identity causes the system to use the delta-values in the bump map unmodified, but this is not a requirement. 
    // Set the bump mapping matrix. 
    // If you set the bump mapping operation to include luminance (D3DTOP_BUMPENVMAPLUMINANCE), you must set the luminance controls. The luminance controls configure how the system computes luminance before modulating the color from the texture in the next stage. (For details, see Bump Mapping Formulas.)
    // Set luminance controls. This is only needed when using a bump map 
    // that contains luminance, and when the D3DTOP_BUMPENVMAPLUMINANCE 
    // texture blending operation is being used.

			D3DSetTextureStageState1BM (0.5f, 0.0f, 0.0f, 0.5f, 1.0f, 0.0f );		
        } 
/* 08/24/2004 Wendell Buckner 
    Make sure you render regular polys properly... */
		else
		{
			D3DBlendEnable(TRUE);

			D3DSetTextureStageState0(D3DTA_TEXTURE,D3DTA_DIFFUSE,D3DTOP_MODULATE,D3DTA_TEXTURE,D3DTA_DIFFUSE,D3DTOP_MODULATE,0);
			D3DSetTextureStageState1(D3DTA_TEXTURE,D3DTA_CURRENT,D3DTOP_DISABLE,D3DTA_DIFFUSE,D3DTA_CURRENT,D3DTOP_DISABLE,0);
		}

//END ********************************************************************************************/

/*	04/23/2003 Wendell Buckner
	Send Misc Poly's as triangle lists instead of fans... 	*/

//* BEGIN**********************************************************************************************

		LastPoly  = ( (i + 1) == MiscCache.NumPolys );
        OnlyPoly  = ( MiscCache.NumPolys == 1 );

		if ( !LastPoly )
			pPoly2 = MiscCache.SortedPolys[i+1];
		else 
		{
			if( !OnlyPoly )
				pPoly2 = MiscCache.SortedPolys[i-1];				 
			else
			pPoly2 = MiscCache.SortedPolys[i];				 
		}

		MatchPoly = ( ( pPoly->THandle == pPoly2->THandle ) && (pPoly->Flags == pPoly2->Flags ) && (pPoly->MipLevel == pPoly2->MipLevel) && (!OnlyPoly) );
		PendPoly  = ( VertCount > 0 );
		AddPoly   = ( MatchPoly || PendPoly );

		while ( AddPoly )
		{
			memcpy( &Verts2[VertCount], &MiscCache.Verts[pPoly->FirstVert], sizeof(PCache_Vert) * 3 );
			VertCount += 3;

			if ( pPoly->NumVerts == 3 ) break;

			MaxVerts = pPoly->NumVerts - 1; 

			for ( vi = 2; vi < MaxVerts; vi++, VertCount +=3 )
			{	
				memcpy( &Verts2[VertCount], &MiscCache.Verts[pPoly->FirstVert], sizeof(PCache_Vert) );
				memcpy( &Verts2[VertCount+1], &MiscCache.Verts[pPoly->FirstVert+vi], sizeof(PCache_Vert) * 2 );
			}

			break;
		}

		if ( MatchPoly )
		{
			MatchPolyCount = pPoly2->NumVerts - 2;
			MatchVertCount =  MatchPolyCount * 3;				
			PolyCount = VertCount/3;
			FullPoly = ( ((VertCount + MatchVertCount) > MAX_MISC_POLY_VERTS - 1) || ((PolyCount + MatchPolyCount) > MAX_MISC_POLYS) );
		}
        else FullPoly = FALSE;

		FlushPoly = ( (!MatchPoly) || LastPoly || FullPoly);

		if ( FlushPoly )
		{
			if ( PendPoly )
			{					 
				D3DTexturedPoly3( Verts2, VertCount );
				VertCount = 0;
			}
			else
			{
				D3DTexturedPoly(&MiscCache.Verts[pPoly->FirstVert], pPoly->NumVerts);  
			}
		}

//* END **********************************************************************************************

/*  01/18/2003 Wendell Buckner                                                         
     To continually turn this off and on really is ineffecient... turn it on when you  
     need it turn if off when you don't   *
/*	01/13/2003 Wendell Buckner
    Optimization from GeForce_Optimization2.doc
    9.	Do not duplicate render state commands.  Worse is useless renderstates.  Do not set a renderstate unless it is needed. */

	}

/*  01/18/2003 Wendell Buckner                                                         
     Basically when you leave the above loop, make sure fog is on  */
	if ( AppInfo.FogEnable ) D3DFogEnable ( TRUE, ((DWORD)AppInfo.FogR<<16)|((DWORD)AppInfo.FogG<<8)|(DWORD)AppInfo.FogB ); 

	// Turn z stuff back on...
	D3DZWriteEnable (TRUE);
	D3DZEnable(TRUE);
	
	MiscCache.NumPolys = 0;
	MiscCache.NumVerts = 0;

#ifdef SUPER_FLUSH
/* 02/28/2001 Wendell Buckner
   These render states are unsupported d3d 7.0 */
	AppInfo.lpD3DDevice->EndScene();
	AppInfo.lpD3DDevice->BeginScene();
#endif

/* 04/08/2003 Wendell Buckner
    BUMPMAPPING 
	Get all kinds of weird blending on the walls if I don't do this... */
	if (AppInfo.CanDoMultiTexture)
	{
     D3DSetTextureStageState1(D3DTA_TEXTURE,D3DTA_CURRENT,D3DTOP_DISABLE,D3DTA_DIFFUSE,D3DTA_CURRENT,D3DTOP_DISABLE,0);

/* 08/24/2004 Wendell Buckner 
    No reason to clear the textures... just disable the texture stages your not using (D3DTOP_DISABLE)... *
     D3DSetTexture(1, NULL);		// Reset texture stage 1
	 D3DSetTexture(2, NULL);		// Reset texture stage 1 */
	}

	return TRUE;
}

//====================================================================================
//	PCache_InsertMiscPoly
//====================================================================================
BOOL PCache_InsertMiscPoly(DRV_TLVertex *Verts, int32 NumVerts, geRDriver_THandle *THandle, uint32 Flags)
{
	int32			Mip;
	float			ZRecip, u, v, ScaleU, ScaleV, InvScale;
	Misc_Poly		*pCachePoly;
	DRV_TLVertex	*pVerts;
	PCache_Vert		*pD3DVerts;
	int32			i, SAlpha;

	if ((MiscCache.NumVerts + NumVerts) >= MAX_MISC_POLY_VERTS)
	{
		// If the cache is full, we must flush it before going on...
		PCache_FlushMiscPolys();
	}
	else if (MiscCache.NumPolys+1 >= MAX_MISC_POLYS)
	{
		// If the cache is full, we must flush it before going on...
		PCache_FlushMiscPolys();
	}

	Mip = GetMipLevel(Verts, NumVerts, (float)THandle->Width, (float)THandle->Height, THandle->NumMipLevels-1);

	// Store info about this poly in the cache
	pCachePoly = &MiscCache.Polys[MiscCache.NumPolys];

	pCachePoly->THandle = THandle;
	pCachePoly->Flags = Flags;
	pCachePoly->FirstVert = MiscCache.NumVerts;
	pCachePoly->NumVerts = NumVerts;
	pCachePoly->MipLevel = Mip;
	pCachePoly->SortKey = ((THandle - TextureHandles)<<4)+Mip;

	// Get scale value for vertices
	//TCache_GetUVInvScale(Bitmap, Mip, &InvScale);
	InvScale = 1.0f / (float)((1<<THandle->Log));

	// Convert them to take account that the vertices are allready from 0 to 1
	ScaleU = (float)THandle->Width * InvScale;
	ScaleV = (float)THandle->Height * InvScale;

	// Precompute the alpha value...
	SAlpha = ((int32)Verts->a)<<24;

	// Get a pointer to the original polys verts
	pVerts = Verts;
	// Get a pointer into the world verts
	pD3DVerts = &MiscCache.Verts[MiscCache.NumVerts];

	for (i=0; i< NumVerts; i++)
	{
		ZRecip = 1/(pVerts->z);

		pD3DVerts->x = pVerts->x;
		pD3DVerts->y = pVerts->y;

		pD3DVerts->z = (1.0f - ZRecip);		// ZBUFFER
		pD3DVerts->rhw = ZRecip;
		
		u = pVerts->u * ScaleU;
		v = pVerts->v * ScaleV;

		pD3DVerts->uv[0].u = u;
		pD3DVerts->uv[0].v = v;

		pD3DVerts->color = SAlpha | ((int32)pVerts->r<<16) | ((int32)pVerts->g<<8) | (int32)pVerts->b;

		if (AppInfo.FogEnable && !(Flags & DRV_RENDER_POLY_NO_FOG) ) // poly fog
		{
			DWORD	FogVal;
			float	Val;

			Val = pVerts->z;

			if (Val > AppInfo.FogEnd)
				Val = AppInfo.FogEnd;

			FogVal = (DWORD)((AppInfo.FogEnd-Val)/(AppInfo.FogEnd-AppInfo.FogStart)*255.0f);
		
			if (FogVal < 0)
				FogVal = 0;
			else if (FogVal > 255)
				FogVal = 255;
		
			pD3DVerts->specular = (FogVal<<24);		// Alpha component in specular is the fog value (0...255)
		}
		else 
			pD3DVerts->specular = 0;

		pVerts++;
		pD3DVerts++;
	}
	
	// Update globals about the misc poly cache
	MiscCache.NumVerts += NumVerts;
	MiscCache.NumPolys++;

	return TRUE;
}

// changed QD Shadows

//====================================================================================
//	PCache_FlushShadowPolys
//====================================================================================
BOOL PCache_FlushStencilPolys(void)
{
	int32				i;
	Stencil_Poly		*pPoly;

	if (!StencilCache.NumPolys)
		return TRUE;

	// Set the render states
	// Turn depth buffer off, and stencil buffer on
    D3DZWriteEnable (FALSE);
	D3DStencilEnable(TRUE);

    // Dont bother with interpolating color
    AppInfo.lpD3DDevice->SetRenderState( D3DRENDERSTATE_SHADEMODE,     D3DSHADE_FLAT );

    // Set up stencil compare fuction, reference value, and masks
    // Stencil test passes if ((ref & mask) cmpfn (stencil & mask)) is true
    D3DStencilFunc(D3DCMP_ALWAYS);
	AppInfo.lpD3DDevice->SetRenderState( D3DRENDERSTATE_STENCILFAIL, D3DSTENCILOP_KEEP );
	
	switch(D3DDRV.StencilTestMode)
	{
	case 0:
		// zfail method
		AppInfo.lpD3DDevice->SetRenderState( D3DRENDERSTATE_STENCILPASS, D3DSTENCILOP_KEEP);

		{
	        // If ztest passes/failes, increment stencil buffer value
			AppInfo.lpD3DDevice->SetRenderState( D3DRENDERSTATE_STENCILREF,       0x1 );
			AppInfo.lpD3DDevice->SetRenderState( D3DRENDERSTATE_STENCILMASK,      0xffffffff );
			AppInfo.lpD3DDevice->SetRenderState( D3DRENDERSTATE_STENCILWRITEMASK, 0xffffffff );
			// zfail method:
			AppInfo.lpD3DDevice->SetRenderState( D3DRENDERSTATE_STENCILZFAIL, D3DSTENCILOP_INCR);
		}

		D3DBlendEnable(TRUE);
		D3DBlendFunc (D3DBLEND_ZERO, D3DBLEND_ONE);

		// zfail:(with z-fail algo, back faces must be rendered first) (for actors CCW renders backfaces)
		AppInfo.lpD3DDevice->SetRenderState( D3DRENDERSTATE_CULLMODE,   D3DCULL_CCW);
		// draw front/back-side of shadow volume in stencil/z only
		for (i=0; i< StencilCache.NumPolys; i++)
		{
			pPoly = &StencilCache.Polys[i];
			AppInfo.lpD3DDevice->DrawPrimitive(	D3DPT_TRIANGLEFAN, 
											D3DFVF_XYZRHW | D3DFVF_DIFFUSE, 
											&(StencilCache.Verts[pPoly->FirstVert]), 
											pPoly->NumVerts, NULL);
		}

		{
			// decrement stencil buffer value
			// zfail method:
			AppInfo.lpD3DDevice->SetRenderState( D3DRENDERSTATE_STENCILZFAIL, D3DSTENCILOP_DECR);
		}

		// Now reverse cull order so back/front sides of shadow volume are written.
		// zfail method
		AppInfo.lpD3DDevice->SetRenderState( D3DRENDERSTATE_CULLMODE,   D3DCULL_CW );
		break;

	case 1:
		// zpass method
		AppInfo.lpD3DDevice->SetRenderState( D3DRENDERSTATE_STENCILZFAIL, D3DSTENCILOP_KEEP );
		
		{
	        // If ztest passes/failes, increment stencil buffer value
			AppInfo.lpD3DDevice->SetRenderState( D3DRENDERSTATE_STENCILREF,       0x1 );
			AppInfo.lpD3DDevice->SetRenderState( D3DRENDERSTATE_STENCILMASK,      0xffffffff );
			AppInfo.lpD3DDevice->SetRenderState( D3DRENDERSTATE_STENCILWRITEMASK, 0xffffffff );
			// for zpass method
			AppInfo.lpD3DDevice->SetRenderState( D3DRENDERSTATE_STENCILPASS, D3DSTENCILOP_INCR );
		}

		D3DBlendEnable(TRUE);
		D3DBlendFunc (D3DBLEND_ZERO, D3DBLEND_ONE);

		// zpass method (with zpass algo, front faces must be rendered first) (for actors CW renders frontfaces)
		AppInfo.lpD3DDevice->SetRenderState( D3DRENDERSTATE_CULLMODE,   D3DCULL_CW );
		// draw front/back-side of shadow volume in stencil/z only
		for (i=0; i< StencilCache.NumPolys; i++)
		{
			pPoly = &StencilCache.Polys[i];
			AppInfo.lpD3DDevice->DrawPrimitive(	D3DPT_TRIANGLEFAN, 
											D3DFVF_XYZRHW | D3DFVF_DIFFUSE, 
											&(StencilCache.Verts[pPoly->FirstVert]), 
											pPoly->NumVerts, NULL);
		}

		{
			// decrement stencil buffer value
			// zpass method
			AppInfo.lpD3DDevice->SetRenderState( D3DRENDERSTATE_STENCILPASS, D3DSTENCILOP_DECR);
		}

		// Now reverse cull order so back sides of shadow volume are written.
		// zpass method
		AppInfo.lpD3DDevice->SetRenderState( D3DRENDERSTATE_CULLMODE,   D3DCULL_CCW );
		break;
	}
	
	// Draw back/front-side (zpass/zfail) of shadow volume in stencil/z only
	for (i=0; i< StencilCache.NumPolys; i++)
	{
		pPoly = &StencilCache.Polys[i];
		AppInfo.lpD3DDevice->DrawPrimitive(	D3DPT_TRIANGLEFAN, 
											D3DFVF_XYZRHW | D3DFVF_DIFFUSE, 
											&(StencilCache.Verts[pPoly->FirstVert]), 
											pPoly->NumVerts, NULL);
	}

    // Restore render states
    AppInfo.lpD3DDevice->SetRenderState( D3DRENDERSTATE_CULLMODE, D3DCULL_NONE );
	D3DZWriteEnable (TRUE);
	D3DStencilEnable(FALSE);
	D3DBlendEnable(FALSE);
    AppInfo.lpD3DDevice->SetRenderState( D3DRENDERSTATE_SHADEMODE, D3DSHADE_GOURAUD );


	StencilCache.NumPolys = 0;
	StencilCache.NumVerts = 0;

	return TRUE;
}

//====================================================================================
//	PCache_InsertShadowPoly
//====================================================================================
BOOL PCache_InsertStencilPoly(DRV_XYZVertex *Verts, int32 NumVerts, uint32 Flags)
{
	int32			i;
	float			ZRecip;
	Stencil_Poly	*pCachePoly;
	DRV_XYZVertex	*pVerts;
	D3DXYZRHWVERTEX	*pD3DVerts;
	//D3DTLVERTEX		*pD3DVerts;


	if ((StencilCache.NumVerts + NumVerts) >= MAX_STENCIL_POLY_VERTS)
	{
		// If the cache is full, we must flush it before going on...
		PCache_FlushStencilPolys();
	}
	else if (StencilCache.NumPolys+1 >= MAX_STENCIL_POLYS)
	{
		// If the cache is full, we must flush it before going on...
		PCache_FlushStencilPolys();
	}

	// Store info about this poly in the cache
	pCachePoly = &StencilCache.Polys[StencilCache.NumPolys];
	pCachePoly->Flags = Flags;
	pCachePoly->FirstVert = StencilCache.NumVerts;
	pCachePoly->NumVerts = NumVerts;
	
	pVerts = Verts;
	pD3DVerts = &StencilCache.Verts[StencilCache.NumVerts];
	for (i=0; i< NumVerts; i++)
	{
		ZRecip = 1/pVerts->z;


		pD3DVerts->sx = pVerts->x;
		pD3DVerts->sy = pVerts->y;
		pD3DVerts->sz = (1.0f - ZRecip);		// ZBUFFER
		pD3DVerts->rhw = ZRecip;
		pD3DVerts->color = 0xff111111;

		pVerts++;
		pD3DVerts++;
	}	 
	
	// Update globals about the misc poly cache
	StencilCache.NumVerts += NumVerts;
	StencilCache.NumPolys++;

	return TRUE;
}
// end change
//====================================================================================
//	**** LOCAL STATIC FUNCTIONS *****
//====================================================================================

//====================================================================================
//	World_PolyPrepVerts
//====================================================================================
geBoolean World_PolyPrepVerts(World_Poly *pPoly, int32 PrepMode, int32 Stage1, int32 Stage2)
{
	float			InvScale, u, v;
	PCache_TVert	*pTVerts;
	PCache_Vert		*pVerts;
	float			ShiftU, ShiftV, ScaleU, ScaleV;
	int32			j;

	switch (PrepMode)
	{
		case PREP_WORLD_VERTS_NORMAL:
		{
			pTVerts = &WorldCache.TVerts[pPoly->FirstVert];

			ShiftU = pPoly->ShiftU;
			ShiftV = pPoly->ShiftV;
		 	ScaleU = pPoly->ScaleU;
			ScaleV = pPoly->ScaleV;

			// Get scale value for vertices
			InvScale = 1.0f / (float)((1<<pPoly->THandle->Log));

			pVerts = &WorldCache.Verts[pPoly->FirstVert];
			
			for (j=0; j< pPoly->NumVerts; j++)
			{
				u = pTVerts->u*ScaleU+ShiftU;
				v = pTVerts->v*ScaleV+ShiftV;

				pVerts->uv[Stage1].u = u * InvScale;
				pVerts->uv[Stage1].v = v * InvScale;

				pVerts->color = pTVerts->Color;

				pTVerts++;
				pVerts++;
			}

			break;
		}

		case PREP_WORLD_VERTS_LMAP:
		{
			if (!pPoly->LInfo)
				return GE_TRUE;

			ShiftU = (float)-pPoly->LInfo->MinU + 8.0f;
			ShiftV = (float)-pPoly->LInfo->MinV + 8.0f;

			// Get scale value for vertices
			InvScale = 1.0f/(float)((1<<pPoly->LInfo->THandle->Log)<<4);
				
			pTVerts = &WorldCache.TVerts[pPoly->FirstVert];
			pVerts = &WorldCache.Verts[pPoly->FirstVert];

			for (j=0; j< pPoly->NumVerts; j++)
			{
				u = pTVerts->u + ShiftU;
				v = pTVerts->v + ShiftV;

				pVerts->uv[Stage1].u = u * InvScale;
				pVerts->uv[Stage1].v = v * InvScale;

				pVerts->color = 0xffffffff;

				pTVerts++;
				pVerts++;
			}
			break;
		}

		case PREP_WORLD_VERTS_SINGLE_PASS:
		{
			float InvScale2, ShiftU2, ShiftV2;

			assert(pPoly->LInfo);

			pTVerts = &WorldCache.TVerts[pPoly->FirstVert];

			// Set up shifts and scaled for texture uv's
			ShiftU = pPoly->ShiftU;
			ShiftV = pPoly->ShiftV;
		 	ScaleU = pPoly->ScaleU;
			ScaleV = pPoly->ScaleV;

			// Get scale value for vertices
			InvScale = 1.0f / (float)((1<<pPoly->THandle->Log));

			// Set up shifts and scaled for lightmap uv's
			ShiftU2 = (float)-pPoly->LInfo->MinU + 8.0f;
			ShiftV2 = (float)-pPoly->LInfo->MinV + 8.0f;
			InvScale2 = 1.0f/(float)((1<<pPoly->LInfo->THandle->Log)<<4);

			pVerts = &WorldCache.Verts[pPoly->FirstVert];

			for (j=0; j< pPoly->NumVerts; j++)
			{
				u = pTVerts->u*ScaleU+ShiftU;
				v = pTVerts->v*ScaleV+ShiftV;

				pVerts->uv[Stage1].u = u * InvScale;
				pVerts->uv[Stage1].v = v * InvScale;
			
				u = pTVerts->u + ShiftU2;
				v = pTVerts->v + ShiftV2;

				pVerts->uv[Stage2].u = u * InvScale2;
				pVerts->uv[Stage2].v = v * InvScale2;

				pVerts->color = pTVerts->Color;

				pTVerts++;
				pVerts++;
			}

			break;
		}

		default:
			return FALSE;
	}

	return TRUE;
}

//====================================================================================
//====================================================================================
static int BitmapHandleComp(const void *a, const void *b)
{
	int32	Id1, Id2;

	Id1 = (*(World_Poly**)a)->SortKey;
	Id2 = (*(World_Poly**)b)->SortKey;

	if ( Id1 == Id2)
		return 0;

	if (Id1 < Id2)
		return -1;

	return 1;
}

//====================================================================================
//====================================================================================
static void SortWorldPolysByHandle(void)
{
	World_Poly	*pPoly;
	int32		i;

	pPoly = WorldCache.Polys;

	for (i=0; i<WorldCache.NumPolys; i++)
	{
		WorldCache.SortedPolys[i] = pPoly;
		pPoly++;
	}
	
	// Sort the polys
	qsort(&WorldCache.SortedPolys, WorldCache.NumPolys, sizeof(WorldCache.SortedPolys[0]), BitmapHandleComp);
}

#define TSTAGE_0			0
#define TSTAGE_1			1

/* 10/15/2003 Wendell Buckner
    Bumpmapping for the World */
#define TSTAGE_2			2

D3DTEXTUREHANDLE		OldId;

/*   07/16/2000 Wendell Buckner
/*    Convert to Directx7...    
LPDIRECT3DTEXTURE2		OldTexture[8];*/
LPDIRECTDRAWSURFACE7    OldTexture[8];

/*	01/28/2003 Wendell Buckner
	Send World Poly's as triangle lists instead of fans... */
PCache_Vert		Verts[MAX_WORLD_POLY_VERTS];

//====================================================================================
//	RenderWorldPolys
//====================================================================================
static BOOL RenderWorldPolys(int32 RenderMode)
{
	World_Poly			*pPoly;
	int32				i;

/*	01/28/2003 Wendell Buckner
	Send World Poly's as triangle lists instead of fans... */
	World_Poly			*pPoly2   = NULL;
    int32				vi        = 0;
    int32               VertCount = 0;
	int32               MaxVerts  = 0; 
	int32			    MatchPolyCount = 0;
	int32				MatchVertCount =  0;
	int32				PolyCount = 0;
	BOOL				LastPoly  = FALSE;
	BOOL				MatchPoly = FALSE;
	BOOL				PendPoly  = FALSE;
	BOOL				AddPoly   = FALSE;
    BOOL				FlushPoly = FALSE;
    BOOL				OnlyPoly = FALSE;
	BOOL				FullPoly = FALSE;

/*	04/21/2003 Wendell Buckner
	Send fog World Poly's as triangle lists instead of fans... */
    int32               fVertCount = 0;

	if(!AppInfo.RenderingIsOK)
	{
		return	TRUE;
	}

// changed QD Shadows
	D3DZWriteEnable (TRUE);
	D3DZEnable(TRUE);
// end change

	switch (RenderMode)
	{	
		case RENDER_WORLD_POLYS_NORMAL:
		{

			AppInfo.lpD3DDevice->SetTextureStageState( 0, D3DTSS_TEXCOORDINDEX, 0 );

			// Set the default state for the normal poly render mode for the world
			D3DBlendEnable(TRUE);
			D3DBlendFunc (D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA);
			
			// Get the first poly in the sorted list
			SortWorldPolysByHandle();
			
			for (i=0; i< WorldCache.NumPolys; i++)
			{
				pPoly = WorldCache.SortedPolys[i];

				if (pPoly->Flags & DRV_RENDER_CLAMP_UV)
					D3DTexWrap(0, FALSE);
				else
					D3DTexWrap(0, TRUE);

				if (!SetupTexture(0, pPoly->THandle, pPoly->MipLevel))
					return GE_FALSE;

				World_PolyPrepVerts(pPoly, PREP_WORLD_VERTS_NORMAL, 0, 0);

				D3DTexturedPoly(&WorldCache.Verts[pPoly->FirstVert], pPoly->NumVerts);
			}
			
			break;
		}
		
		case RENDER_WORLD_POLYS_LMAP:
		{

			AppInfo.lpD3DDevice->SetTextureStageState( 0, D3DTSS_TEXCOORDINDEX, 0);

			D3DTexWrap(0, FALSE);

			D3DBlendEnable(TRUE);
			D3DBlendFunc (D3DBLEND_DESTCOLOR, D3DBLEND_ZERO);

			pPoly = WorldCache.Polys;

			for (i=0; i< WorldCache.NumPolys; i++, pPoly++)
			{
				BOOL	Dynamic = 0;

				if (!pPoly->LInfo)
					continue;

				// Call the engine to set this sucker up, because it's visible...
				D3DDRV.SetupLightmap(pPoly->LInfo, &Dynamic);

				if (!SetupLMap(0, pPoly->LInfo, 0, Dynamic))
					return GE_FALSE;

				World_PolyPrepVerts(pPoly, PREP_WORLD_VERTS_LMAP, 0, 0);

				D3DTexturedPoly(&WorldCache.Verts[pPoly->FirstVert], pPoly->NumVerts);
				
				if (pPoly->LInfo->RGBLight[1])
				{

/*  01/18/2003 Wendell Buckner                                                         
     Don't call fog api if it's no set in the global fog flag...*
/*	01/13/2003 Wendell Buckner
    Optimization from GeForce_Optimization2.doc
    9.	Do not duplicate render state commands.  Worse is useless renderstates.  Do not set a renderstate unless it is needed. 
					AppInfo.lpD3DDevice->SetRenderState(D3DRENDERSTATE_FOGENABLE, FALSE); *
					D3DFogEnable ( FALSE, 0 );                                            */
  					if (AppInfo.FogEnable)
                       D3DFogEnable ( FALSE, 0 );  


					D3DBlendFunc (D3DBLEND_ONE, D3DBLEND_ONE);				// Change to a fog state

					// For some reason, some cards can't upload data to the same texture twice, and have it take.
					// So we force Fog maps to use a different slot than the lightmap was using...
					pPoly->LInfo->THandle->MipData[0].Slot = NULL;

					if (!SetupLMap(0, pPoly->LInfo, 1, 1))	// Dynamic is 1, because fog is always dynamic
						return GE_FALSE;

					D3DTexturedPoly(&WorldCache.Verts[pPoly->FirstVert], pPoly->NumVerts);
		
					D3DBlendFunc (D3DBLEND_DESTCOLOR, D3DBLEND_ZERO);		// Restore state

					if (AppInfo.FogEnable)
/*	01/13/2003 Wendell Buckner
    Optimization from GeForce_Optimization2.doc
    9.	Do not duplicate render state commands.  Worse is useless renderstates.  Do not set a renderstate unless it is needed. 
						AppInfo.lpD3DDevice->SetRenderState(D3DRENDERSTATE_FOGENABLE , TRUE); */
						D3DFogEnable ( TRUE, ((DWORD)AppInfo.FogR<<16)|((DWORD)AppInfo.FogG<<8)|(DWORD)AppInfo.FogB );
				}
			}
			break;
		}

		case RENDER_WORLD_POLYS_SINGLE_PASS:
		{
			// Setup texture stage states

/* 01/18/2003 Wendell Buckner
    Optimization from GeForce_Optimization2.doc  
9.	Do not duplicate render state commands.  Worse is useless renderstates.  Do not set a renderstate unless it is needed. */
            D3DSetTextureStageState0(D3DTA_TEXTURE,D3DTA_DIFFUSE,D3DTOP_MODULATE,D3DTA_TEXTURE,D3DTA_DIFFUSE,D3DTOP_MODULATE,0); 

/* 01/18/2003 Wendell Buckner
    Optimization from GeForce_Optimization2.doc  
9.	Do not duplicate render state commands.  Worse is useless renderstates.  Do not set a renderstate unless it is needed. */
            D3DSetTextureStageState1(D3DTA_TEXTURE,D3DTA_CURRENT,D3DTOP_MODULATE,D3DTA_DIFFUSE,D3DTA_CURRENT,D3DTOP_MODULATE,1);

			// Setup frame buffer blend modes
			D3DBlendEnable(TRUE);
			D3DBlendFunc (D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA);

			// Set the default state for the normal poly render mode for the world
			D3DTexWrap(TSTAGE_0, TRUE);
			D3DTexWrap(TSTAGE_1, FALSE);

			// Sort the list for front back operation to get the least number of world texture misses
			SortWorldPolysByHandle();
			
			// Reset non lightmaps faces to 0
			WorldCache.NumPolys2 = 0;

/* 04/21/2003 Wendell Buckner
	Send World Poly's as triangle lists instead of fans... */
            WorldCache.NumPolys3 = 0;

			for (i=0; i< WorldCache.NumPolys; i++)
			{
				pPoly = WorldCache.SortedPolys[i];

				if (!pPoly->LInfo)
				{
					// Put gouraud only polys in a seperate list, and render last
					WorldCache.SortedPolys2[WorldCache.NumPolys2++] = pPoly;
				}
				else
				{
					// Put lightmap only polys in a seperate list, and render last
				    WorldCache.SortedPolys3[WorldCache.NumPolys3++] = pPoly;
				}
            }
			
/* 04/21/2003 Wendell Buckner
	Send World Poly's as triangle lists instead of fans... */
			for (i=0; i< WorldCache.NumPolys3; i++)
			{
				BOOL	Dynamic = 0;

/* 04/21/2003 Wendell Buckner
	Send World Poly's as triangle lists instead of fans... */
				pPoly = WorldCache.SortedPolys3[i];

				if (pPoly->Flags & DRV_RENDER_CLAMP_UV)
					D3DTexWrap(TSTAGE_0, FALSE);
				else
					D3DTexWrap(TSTAGE_0, TRUE);

				if (!SetupTexture(TSTAGE_0, pPoly->THandle, pPoly->MipLevel))
					return GE_FALSE;				

				// Call the engine to set this sucker up, because it's visible...
				D3DDRV.SetupLightmap(pPoly->LInfo, &Dynamic);

/* 10/15/2003 Wendell Buckner
    Bumpmapping for the World */
		        if(pPoly->Flags & DRV_RENDER_BUMPMAP)
				{
	             geRDriver_THandle	*pTHandle;                 
//ts1 = bumpmap
				 if (pPoly->Flags & DRV_RENDER_CLAMP_UV)
				  D3DTexWrap(TSTAGE_1, FALSE);
				 else
			      D3DTexWrap(TSTAGE_1, TRUE);

                 pTHandle = pPoly->THandle->NextTHandle;

//TODO: Allow mip map levels greater than 0 */
                 if (!SetupTexture(TSTAGE_1, pTHandle, 0)) 
		          return GE_FALSE;

//ts2 = lightmap
                 D3DTexWrap(TSTAGE_2, FALSE);

				 if (!SetupLMap(TSTAGE_2, pPoly->LInfo, 0, Dynamic))
					return GE_FALSE;
				}
		        else
				{
 				 D3DTexWrap(TSTAGE_1, FALSE);

				 if (!SetupLMap(TSTAGE_1, pPoly->LInfo, 0, Dynamic))
					return GE_FALSE;

/* 08/24/2004 Wendell Buckner 
    No reason to clear the textures... just disable the texture stages your not using (D3DTOP_DISABLE)...*
//make sure texture stage 2 is clear for regular lightmaps...
 				 D3DSetTexture(2, NULL);		// Reset texture stage 2 */
				}
    
// Prep the verts for a lightmap and texture map
				World_PolyPrepVerts(pPoly, PREP_WORLD_VERTS_SINGLE_PASS, TSTAGE_0, TSTAGE_1);

/*  01/18/2003 Wendell Buckner                                                         
     To continually turn this off and on really is ineffecient... turn it on when you  
     need it turn if off when you don't *
/*	01/13/2003 Wendell Buckner
    Optimization from GeForce_Optimization2.doc
    9.	Do not duplicate render state commands.  Worse is useless renderstates.  Do not set a renderstate unless it is needed. */
  				if ( AppInfo.FogEnable ) // poly fog
				{
				 if ( pPoly->Flags & DRV_RENDER_POLY_NO_FOG )
				  D3DFogEnable ( FALSE, 0 );
				 else
				  D3DFogEnable ( TRUE, ((DWORD)AppInfo.FogR<<16)|((DWORD)AppInfo.FogG<<8)|(DWORD)AppInfo.FogB );
				}    

/* 10/15/2003 Wendell Buckner
    Bumpmapping for the World */
				if (pPoly->Flags & DRV_RENDER_BUMPMAP)
				{
// Set the color operations and arguments to prepare for bump mapping.

// Stage 0: the base texture
				 D3DSetTextureStageState0(D3DTA_TEXTURE, D3DTA_DIFFUSE, D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_CURRENT, D3DTOP_SELECTARG1, 0);

// Stage 1: the bump map 
				 D3DSetTextureStageState1(D3DTA_TEXTURE, D3DTA_CURRENT, D3DTOP_BUMPENVMAPLUMINANCE, D3DTA_TEXTURE, D3DTA_CURRENT, D3DTOP_DISABLE, 0);

// Use lightmap/luminance for this example.
				 D3DSetTextureStageState2(D3DTA_TEXTURE, D3DTA_CURRENT, D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_CURRENT, D3DTOP_DISABLE , 1);

		         D3DSetTextureStageState1BM (1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f );		
				} 
                else
				{
                 D3DSetTextureStageState0(D3DTA_TEXTURE,D3DTA_DIFFUSE,D3DTOP_MODULATE,D3DTA_TEXTURE,D3DTA_DIFFUSE,D3DTOP_MODULATE,0); 
                 D3DSetTextureStageState1(D3DTA_TEXTURE,D3DTA_CURRENT,D3DTOP_MODULATE,D3DTA_DIFFUSE,D3DTA_CURRENT,D3DTOP_MODULATE,1);

/* 08/24/2004 Wendell Buckner 
    No reason to clear the textures... just disable the texture stages your not using (D3DTOP_DISABLE)...*/
                 D3DSetTextureStageState2(D3DTA_TEXTURE,D3DTA_CURRENT,D3DTOP_DISABLE,D3DTA_DIFFUSE,D3DTA_CURRENT,D3DTOP_DISABLE,0);

				}

				// Draw the texture

//				D3DMain_Log("BEGIN - Draw Lightmap Polys\n");

/*	01/28/2003 Wendell Buckner
	Send World Poly's as triangle lists instead of fans... */

/*  BEGIN**********************************************************************************************/
				
				LastPoly  = ( (i + 1) == WorldCache.NumPolys3 );
                OnlyPoly  = ( WorldCache.NumPolys3 == 1 );

				if ( !LastPoly )
				  pPoly2 = WorldCache.SortedPolys3[i+1];
				else 
				{
				 if( !OnlyPoly )
				  pPoly2 = WorldCache.SortedPolys3[i-1];				 
				 else
				  pPoly2 = WorldCache.SortedPolys3[i];				 
				}

				MatchPoly = ( (pPoly->THandle == pPoly2->THandle) && (pPoly->Flags == pPoly2->Flags) && ( pPoly->LInfo->THandle  == pPoly2->LInfo->THandle) && (pPoly->MipLevel == pPoly2->MipLevel) && (pPoly->LInfo->RGBLight[1] == pPoly2->LInfo->RGBLight[1]) && (!OnlyPoly) );
				PendPoly  = ( VertCount > 0 );
				AddPoly   = ( MatchPoly || PendPoly );

				while ( AddPoly )
				{
				 memcpy( &Verts[VertCount], &WorldCache.Verts[pPoly->FirstVert], sizeof(PCache_Vert) * 3 );
				 VertCount += 3;

				 if ( pPoly->NumVerts == 3 ) break;

				 MaxVerts = pPoly->NumVerts - 1; 

                 for ( vi = 2; vi < MaxVerts; vi++, VertCount +=3 )
				 {	
				  memcpy( &Verts[VertCount], &WorldCache.Verts[pPoly->FirstVert], sizeof(PCache_Vert) );
                  memcpy( &Verts[VertCount+1], &WorldCache.Verts[pPoly->FirstVert+vi], sizeof(PCache_Vert) * 2 );
				 }

				 break;
				}

				if ( MatchPoly )
				{
				 MatchPolyCount = pPoly2->NumVerts - 2;
                 MatchVertCount =  MatchPolyCount * 3;				
				 PolyCount = VertCount/3;
                 FullPoly = ( ((VertCount + MatchVertCount) > MAX_WORLD_POLY_VERTS - 1) || ((PolyCount + MatchPolyCount) > MAX_WORLD_POLYS) );
				}
                else FullPoly = FALSE;

				FlushPoly = ( (!MatchPoly) || LastPoly || FullPoly);

				if ( FlushPoly )
				{
				 if ( PendPoly )
				 {					 
				  D3DTexturedPoly3( Verts, VertCount );
				  fVertCount = VertCount;
	              VertCount = 0;
				 }
				 else
				 {
				  D3DTexturedPoly(&WorldCache.Verts[pPoly->FirstVert], pPoly->NumVerts);  
				 }
				}
                else continue;

/* END **********************************************************************************************/

/*  01/18/2003 Wendell Buckner                                                         
     To continually turn this off and on really is ineffecient... turn it on when you  
     need it turn if off when you don't *
/*	01/13/2003 Wendell Buckner
    Optimization from GeForce_Optimization2.doc
    9.	Do not duplicate render state commands.  Worse is useless renderstates.  Do not set a renderstate unless it is needed. */

				// Render any fog maps
				if (pPoly->LInfo->RGBLight[1])
				{

/*  01/18/2003 Wendell Buckner                                                         
     Basically when you leave the above loop, make sure fog is on  */
				    if ( AppInfo.FogEnable ) D3DFogEnable ( TRUE, ((DWORD)AppInfo.FogR<<16)|((DWORD)AppInfo.FogG<<8)|(DWORD)AppInfo.FogB );

					D3DBlendFunc (D3DBLEND_ONE, D3DBLEND_ONE);				// Change to a fog state

				#if (TSTAGE_0 == 0)

/* 01/18/2003 Wendell Buckner
    Optimization from GeForce_Optimization2.doc  
9.	Do not duplicate render state commands.  Worse is useless renderstates.  Do not set a renderstate unless it is needed. */
                    D3DSetTextureStageState0(D3DTA_TEXTURE,D3DTA_DIFFUSE,D3DTOP_SELECTARG2,D3DTA_TEXTURE,D3DTA_DIFFUSE,D3DTOP_DISABLE,0); 

/* 01/18/2003 Wendell Buckner
    Optimization from GeForce_Optimization2.doc  
9.	Do not duplicate render state commands.  Worse is useless renderstates.  Do not set a renderstate unless it is needed. */
  					D3DSetTextureStageState1(D3DTA_TEXTURE,D3DTA_CURRENT,D3DTOP_SELECTARG1,D3DTA_DIFFUSE,D3DTA_CURRENT,D3DTOP_MODULATE,1); 

				#else
					AppInfo.lpD3DDevice->SetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );
					AppInfo.lpD3DDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
				#endif

					// For some reason, some cards can't upload data to the same texture twice, and have it take.
					// So we force Fog maps to use a different slot other than what the lightmap was using...
					pPoly->LInfo->THandle->MipData[0].Slot = NULL;

					if (!SetupLMap(TSTAGE_1, pPoly->LInfo, 1, 1))	// Dynamic is 1, because fog is always dynamic
						return GE_FALSE;

/*	01/28/2003 Wendell Buckner
	Send World Poly's as triangle lists instead of fans... */
				    if ( FlushPoly )
					{
					 if ( PendPoly )
					 {					 
					  D3DTexturedPoly3( Verts, fVertCount );
					  fVertCount = 0;
					 }
					 else
					  D3DTexturedPoly(&WorldCache.Verts[pPoly->FirstVert], pPoly->NumVerts);  
					}
					
					// Restore states to the last state before fag map
				#if (TSTAGE_0 == 0)

/* 01/18/2003 Wendell Buckner
    Optimization from GeForce_Optimization2.doc  
9.	Do not duplicate render state commands.  Worse is useless renderstates.  Do not set a renderstate unless it is needed. */
   	                D3DSetTextureStageState0(D3DTA_TEXTURE,D3DTA_DIFFUSE,D3DTOP_MODULATE,D3DTA_TEXTURE,D3DTA_DIFFUSE,D3DTOP_MODULATE,0); 

/* 01/18/2003 Wendell Buckner
    Optimization from GeForce_Optimization2.doc  
9.	Do not duplicate render state commands.  Worse is useless renderstates.  Do not set a renderstate unless it is needed. */
  					D3DSetTextureStageState1(D3DTA_TEXTURE,D3DTA_CURRENT,D3DTOP_MODULATE,D3DTA_DIFFUSE,D3DTA_CURRENT,D3DTOP_MODULATE,1); 

				#else
					AppInfo.lpD3DDevice->SetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_MODULATE );
					AppInfo.lpD3DDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2 );
				#endif

					D3DBlendFunc (D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA);
				}
				
				
			}
			
			// Setup for any non-lightmaped faces faces, turn tmu1 off
		#if (TSTAGE_0 == 0)

/* 01/18/2003 Wendell Buckner
    Optimization from GeForce_Optimization2.doc  
9.	Do not duplicate render state commands.  Worse is useless renderstates.  Do not set a renderstate unless it is needed. */
            D3DSetTextureStageState1(D3DTA_TEXTURE,D3DTA_CURRENT,D3DTOP_DISABLE,D3DTA_DIFFUSE,D3DTA_CURRENT,D3DTOP_DISABLE,1); 

		#else
			AppInfo.lpD3DDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
			AppInfo.lpD3DDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
		#endif

/*	01/28/2003 Wendell Buckner
	Send World Poly's as triangle lists instead of fans...*/
			pPoly2    = NULL;
    		vi        = 0;
			VertCount = 0;
			MaxVerts  = 0; 
			MatchPolyCount = 0;
			MatchVertCount =  0;
			PolyCount = 0;
			LastPoly  = FALSE;
			MatchPoly = FALSE;
			PendPoly  = FALSE;
			AddPoly   = FALSE;
    		FlushPoly = FALSE;
    		OnlyPoly = FALSE;
			FullPoly = FALSE;

			// Render all the faces without lightmaps
			for (i=0; i< WorldCache.NumPolys2; i++)
			{
				BOOL	Dynamic = 0;

				pPoly = WorldCache.SortedPolys2[i];

				if (pPoly->Flags & DRV_RENDER_CLAMP_UV)
					D3DTexWrap(TSTAGE_0, FALSE);
				else
					D3DTexWrap(TSTAGE_0, TRUE);

				if (!SetupTexture(TSTAGE_0, pPoly->THandle, pPoly->MipLevel))
					return GE_FALSE;				

/* 01/11/2004 Wendell Buckner
    FULLBRIGHT BUG - Fix lightmap being displayed when rendering non-lightmap poly's like full bright polys */
				if( pPoly->Flags & DRV_RENDER_BUMPMAP ) 
				{

/* 11/11/2003 Wendell Buckner
    Bumpmapping for the World */
		        do
				{
	             geRDriver_THandle	*pTHandle;                 

/* 01/11/2004 Wendell Buckner
    FULLBRIGHT BUG - Fix lightmap being displayed when rendering non-lightmap poly's like full bright polys 
				 if(!(pPoly->Flags & DRV_RENDER_BUMPMAP)) break; */


//ts1 = bumpmap
				 if (pPoly->Flags & DRV_RENDER_CLAMP_UV)
				  D3DTexWrap(TSTAGE_1, FALSE);
				 else
			      D3DTexWrap(TSTAGE_1, TRUE);

                 pTHandle = pPoly->THandle->NextTHandle;

				 if( !pTHandle ) break;

//TODO: Allow mip map levels greater than 0 */
                 if (!SetupTexture(TSTAGE_1, pTHandle, 0)) 
		          return GE_FALSE;

//ts2 = specular
				 if (pPoly->Flags & DRV_RENDER_CLAMP_UV)
				  D3DTexWrap(TSTAGE_2, FALSE);
				 else
			      D3DTexWrap(TSTAGE_2, TRUE);

                 pTHandle = pTHandle->NextTHandle;

				 if( !pTHandle ) break;

				 if (!SetupTexture(TSTAGE_2, pTHandle, 0))
					return GE_FALSE;
				}
				while (FALSE);

/* 01/11/2004 Wendell Buckner
    FULLBRIGHT BUG - Fix lightmap being displayed when rendering non-lightmap poly's like full bright polys */
				}

/* 08/24/2004 Wendell Buckner 
    No reason to clear the textures... just disable the texture stages your not using (D3DTOP_DISABLE)...*
				else
				{
				 D3DSetTexture(TSTAGE_1,NULL);
				 D3DSetTexture(TSTAGE_2,NULL);
				} */

				// Prep verts as if there was no lightmap
				World_PolyPrepVerts(pPoly, PREP_WORLD_VERTS_NORMAL, TSTAGE_0, TSTAGE_1);


/*  01/18/2003 Wendell Buckner                                                         
     To continually turn this off and on really is ineffecient... turn it on when you  
     need it turn if off when you don't *
/*	01/13/2003 Wendell Buckner
    Optimization from GeForce_Optimization2.doc
    9.	Do not duplicate render state commands.  Worse is useless renderstates.  Do not set a renderstate unless it is needed. */ 
				if ( AppInfo.FogEnable ) // poly fog
				{
				  if ( pPoly->Flags & DRV_RENDER_POLY_NO_FOG )
				   D3DFogEnable ( FALSE, 0 );
				  else
                   D3DFogEnable ( TRUE, ((DWORD)AppInfo.FogR<<16)|((DWORD)AppInfo.FogG<<8)|(DWORD)AppInfo.FogB );
				} 

/* 11/11/2003 Wendell Buckner
    Bumpmapping for the World */
				if (pPoly->Flags & DRV_RENDER_BUMPMAP)
				{
// Set the color operations and arguments to prepare for bump mapping.

// Stage 0: the base texture
				 D3DSetTextureStageState0(D3DTA_TEXTURE, D3DTA_DIFFUSE, D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_CURRENT, D3DTOP_SELECTARG1, 0);

// Stage 1: the bump map 
				 D3DSetTextureStageState1(D3DTA_TEXTURE, D3DTA_CURRENT, D3DTOP_BUMPENVMAPLUMINANCE, D3DTA_TEXTURE, D3DTA_CURRENT, D3DTOP_DISABLE, 0);

// Use specular/luminance for this example.
                 D3DSetTextureStageState2(D3DTA_TEXTURE, D3DTA_CURRENT, D3DTOP_ADD, D3DTA_TEXTURE, D3DTA_CURRENT, D3DTOP_DISABLE , 0);

		         D3DSetTextureStageState1BM (1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f );		
				} 
                else
				{
                 D3DSetTextureStageState0(D3DTA_TEXTURE,D3DTA_DIFFUSE,D3DTOP_MODULATE,D3DTA_TEXTURE,D3DTA_DIFFUSE,D3DTOP_MODULATE,0); 

/* 08/24/2004 Wendell Buckner 
    No reason to clear the textures... just disable the texture stages your not using (D3DTOP_DISABLE)...*
                 D3DSetTextureStageState1(D3DTA_TEXTURE,D3DTA_CURRENT,D3DTOP_MODULATE,D3DTA_DIFFUSE,D3DTA_CURRENT,D3DTOP_MODULATE,1); */
                 D3DSetTextureStageState1(D3DTA_TEXTURE,D3DTA_CURRENT,D3DTOP_DISABLE,D3DTA_DIFFUSE,D3DTA_CURRENT,D3DTOP_DISABLE,1);
				}

				// Draw the texture

/*	01/28/2003 Wendell Buckner
	Send World Poly's as triangle lists instead of fans...*/

//* BEGIN**********************************************************************************************
				
				LastPoly  = ( (i + 1) == WorldCache.NumPolys2 );
                OnlyPoly  = ( WorldCache.NumPolys2 == 1 );

				if ( !LastPoly )
				  pPoly2 = WorldCache.SortedPolys2[i+1];
				else 
				{
				 if( !OnlyPoly )
				  pPoly2 = WorldCache.SortedPolys2[i-1];				 
				 else
				  pPoly2 = WorldCache.SortedPolys2[i];				 
				}

				MatchPoly = ( ( pPoly->THandle == pPoly2->THandle ) && (pPoly->Flags == pPoly2->Flags ) && (pPoly->MipLevel == pPoly2->MipLevel) && (!OnlyPoly) );
				PendPoly  = ( VertCount > 0 );
				AddPoly   = ( MatchPoly || PendPoly );

				while ( AddPoly )
				{
				 memcpy( &Verts[VertCount], &WorldCache.Verts[pPoly->FirstVert], sizeof(PCache_Vert) * 3 );
				 VertCount += 3;

				 if ( pPoly->NumVerts == 3 ) break;

				 MaxVerts = pPoly->NumVerts - 1; 

                 for ( vi = 2; vi < MaxVerts; vi++, VertCount +=3 )
				 {	
				  memcpy( &Verts[VertCount], &WorldCache.Verts[pPoly->FirstVert], sizeof(PCache_Vert) );
                  memcpy( &Verts[VertCount+1], &WorldCache.Verts[pPoly->FirstVert+vi], sizeof(PCache_Vert) * 2 );
				 }

				 break;
				}

				if ( MatchPoly )
				{
				 MatchPolyCount = pPoly2->NumVerts - 2;
                 MatchVertCount =  MatchPolyCount * 3;				
				 PolyCount = VertCount/3;
                 FullPoly = ( ((VertCount + MatchVertCount) > MAX_WORLD_POLY_VERTS - 1) || ((PolyCount + MatchPolyCount) > MAX_WORLD_POLYS) );
				}
                else FullPoly = FALSE;


				FlushPoly = ( (!MatchPoly) || LastPoly || FullPoly);

				if ( FlushPoly )
				{
				 if ( PendPoly )
				 {					 
				  D3DTexturedPoly3( Verts, VertCount );
	              VertCount = 0;
				 }
				 else
				 {
				  D3DTexturedPoly(&WorldCache.Verts[pPoly->FirstVert], pPoly->NumVerts);  
				 }
				}

//* END **********************************************************************************************/

/*  01/18/2003 Wendell Buckner                                                         
     To continually turn this off and on really is ineffecient... turn it on when you  
     need it turn if off when you don't*
/*	01/13/2003 Wendell Buckner
    Optimization from GeForce_Optimization2.doc
    9.	Do not duplicate render state commands.  Worse is useless renderstates.  Do not set a renderstate unless it is needed. */
			}

/*  01/18/2003 Wendell Buckner                                                         
     Basically when you leave the above loop, make sure fog is on  */
            if ( AppInfo.FogEnable ) D3DFogEnable ( TRUE, ((DWORD)AppInfo.FogR<<16)|((DWORD)AppInfo.FogG<<8)|(DWORD)AppInfo.FogB );
			break;						 
		}

		default:
			return FALSE;
	}


	return TRUE;
}

//====================================================================================
//	ClearWorldCache
//====================================================================================
static BOOL ClearWorldCache(void)
{
	WorldCache.NumPolys = 0;
	WorldCache.NumVerts = 0;

	return TRUE;
}

//====================================================================================
//====================================================================================
BOOL PCache_Reset(void)
{
	WorldCache.NumPolys = 0;
	WorldCache.NumVerts = 0;

	MiscCache.NumPolys = 0;
	MiscCache.NumVerts = 0;

/*  01/28/2003 Wendell Buckner                                                          */
/*   Cache decals so that they can be drawn after all the 3d stuff...                   */
	DecalCache.NumDecals = 0;
	return TRUE;
}

//====================================================================================
//	GetMipLevel
//====================================================================================
static int32 GetMipLevel(DRV_TLVertex *Verts, int32 NumVerts, float ScaleU, float ScaleV, int32 MaxMipLevel)
{
	int32		Mip;

	if (MaxMipLevel == 0)
		return 0;

	//
	//	Get the MipLevel
	//
	{
		float		du, dv, dx, dy, MipScale;

	#if 1		// WAY slower, but more accurate
		int32		i;

		MipScale = 999999.0f;

		for (i=0; i< NumVerts; i++)
		{
			float			MipScaleT;
			DRV_TLVertex	*pVert0, *pVert1;
			int32			i2;

			i2 = i+1;

			if (i2 >= NumVerts)
				i2=0;

			pVert0 = &Verts[i];
			pVert1 = &Verts[i2];

			du = pVert1->u - pVert0->u;
			dv = pVert1->v - pVert0->v;
			dx = pVert1->x - pVert0->x;
			dy = pVert1->y - pVert0->y;
			
			du *= ScaleU;
			dv *= ScaleV;

			MipScaleT = ((du*du)+(dv*dv)) / ((dx*dx)+(dy*dy));

			if (MipScaleT < MipScale)
				MipScale = MipScaleT;		// Record the best MipScale (the one closest to the the eye)
		}
	#else		// Faster, less accurate
		du = Verts[1].u - Verts[0].u;
		dv = Verts[1].v - Verts[0].v;
		dx = Verts[1].x - Verts[0].x;
		dy = Verts[1].y - Verts[0].y;

		du *= ScaleU;
		dv *= ScaleV;

		MipScale = ((du*du)+(dv*dv)) / ((dx*dx)+(dy*dy));
	#endif

	#if 0
		if (MipScale <= 5)		// 2, 6, 12
			Mip = 0;
		else if (MipScale <= 20)
			Mip = 1;
		else if (MipScale <= 45)
			Mip = 2;
		else
			Mip = 3;
	#else
		if (MipScale <= 4)		// 2, 6, 12
			Mip = 0;
		else if (MipScale <= 15)
			Mip = 1;
		else if (MipScale <= 40)
			Mip = 2;
		else
			Mip = 3;
	#endif
	}

	if (Mip > MaxMipLevel)
		Mip = MaxMipLevel;

	return Mip;
}