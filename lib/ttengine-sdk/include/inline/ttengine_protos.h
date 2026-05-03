#ifndef _VBCCINLINE_TTENGINE_H
#define _VBCCINLINE_TTENGINE_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

APTR __TT_OpenFontA(__reg("a6") struct Library *, __reg("a0") struct TagItem * taglist)="\tjsr\t-30(a6)";
#define TT_OpenFontA(taglist) __TT_OpenFontA(TTEngineBase, (taglist))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
APTR __TT_OpenFont(__reg("a6") struct Library *, Tag taglist, ...)="\tmove.l\ta0,-(a7)\n\tlea\t4(a7),a0\n\tjsr\t-30(a6)\n\tmovea.l\t(a7)+,a0";
#define TT_OpenFont(...) __TT_OpenFont(TTEngineBase, __VA_ARGS__)
#endif

BOOL __TT_SetFont(__reg("a6") struct Library *, __reg("a1") struct RastPort * rp, __reg("a0") APTR font)="\tjsr\t-36(a6)";
#define TT_SetFont(rp, font) __TT_SetFont(TTEngineBase, (rp), (font))

VOID __TT_CloseFont(__reg("a6") struct Library *, __reg("a0") APTR font)="\tjsr\t-42(a6)";
#define TT_CloseFont(font) __TT_CloseFont(TTEngineBase, (font))

VOID __TT_Text(__reg("a6") struct Library *, __reg("a1") struct RastPort * rp, __reg("a0") APTR string, __reg("d0") ULONG count)="\tjsr\t-48(a6)";
#define TT_Text(rp, string, count) __TT_Text(TTEngineBase, (rp), (string), (count))

ULONG __TT_SetAttrsA(__reg("a6") struct Library *, __reg("a1") struct RastPort * rp, __reg("a0") struct TagItem * taglist)="\tjsr\t-54(a6)";
#define TT_SetAttrsA(rp, taglist) __TT_SetAttrsA(TTEngineBase, (rp), (taglist))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
ULONG __TT_SetAttrs(__reg("a6") struct Library *, __reg("a1") struct RastPort * rp, Tag taglist, ...)="\tmove.l\ta0,-(a7)\n\tlea\t4(a7),a0\n\tjsr\t-54(a6)\n\tmovea.l\t(a7)+,a0";
#define TT_SetAttrs(rp, ...) __TT_SetAttrs(TTEngineBase, (rp), __VA_ARGS__)
#endif

ULONG __TT_GetAttrsA(__reg("a6") struct Library *, __reg("a1") struct RastPort * rp, __reg("a0") struct TagItem * taglist)="\tjsr\t-60(a6)";
#define TT_GetAttrsA(rp, taglist) __TT_GetAttrsA(TTEngineBase, (rp), (taglist))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
ULONG __TT_GetAttrs(__reg("a6") struct Library *, __reg("a1") struct RastPort * rp, Tag taglist, ...)="\tmove.l\ta0,-(a7)\n\tlea\t4(a7),a0\n\tjsr\t-60(a6)\n\tmovea.l\t(a7)+,a0";
#define TT_GetAttrs(rp, ...) __TT_GetAttrs(TTEngineBase, (rp), __VA_ARGS__)
#endif

ULONG __TT_TextLength(__reg("a6") struct Library *, __reg("a1") struct RastPort * rp, __reg("a0") APTR string, __reg("d0") ULONG count)="\tjsr\t-66(a6)";
#define TT_TextLength(rp, string, count) __TT_TextLength(TTEngineBase, (rp), (string), (count))

VOID __TT_TextExtent(__reg("a6") struct Library *, __reg("a1") struct RastPort * rp, __reg("a0") APTR string, __reg("d0") LONG count, __reg("a2") struct TextExtent * te)="\tjsr\t-72(a6)";
#define TT_TextExtent(rp, string, count, te) __TT_TextExtent(TTEngineBase, (rp), (string), (count), (te))

ULONG __TT_TextFit(__reg("a6") struct Library *, __reg("a1") struct RastPort * rp, __reg("a0") APTR string, __reg("d0") ULONG count, __reg("a2") struct TextExtent * te, __reg("a3") struct TextExtent * tec, __reg("d1") LONG dir, __reg("d2") ULONG cwidth, __reg("d3") ULONG cheight)="\tjsr\t-78(a6)";
#define TT_TextFit(rp, string, count, te, tec, dir, cwidth, cheight) __TT_TextFit(TTEngineBase, (rp), (string), (count), (te), (tec), (dir), (cwidth), (cheight))

struct TT_Pixmap * __TT_GetPixmapA(__reg("a6") struct Library *, __reg("a1") APTR font, __reg("a2") APTR string, __reg("d0") ULONG count, __reg("a0") struct TagItem * taglist)="\tjsr\t-84(a6)";
#define TT_GetPixmapA(font, string, count, taglist) __TT_GetPixmapA(TTEngineBase, (font), (string), (count), (taglist))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
struct TT_Pixmap * __TT_GetPixmap(__reg("a6") struct Library *, __reg("a1") APTR font, __reg("a2") APTR string, __reg("d0") ULONG count, Tag taglist, ...)="\tmove.l\ta0,-(a7)\n\tlea\t4(a7),a0\n\tjsr\t-84(a6)\n\tmovea.l\t(a7)+,a0";
#define TT_GetPixmap(font, string, count, ...) __TT_GetPixmap(TTEngineBase, (font), (string), (count), __VA_ARGS__)
#endif

VOID __TT_FreePixmap(__reg("a6") struct Library *, __reg("a0") struct TT_Pixmap * pixmap)="\tjsr\t-90(a6)";
#define TT_FreePixmap(pixmap) __TT_FreePixmap(TTEngineBase, (pixmap))

VOID __TT_DoneRastPort(__reg("a6") struct Library *, __reg("a1") struct RastPort * rp)="\tjsr\t-96(a6)";
#define TT_DoneRastPort(rp) __TT_DoneRastPort(TTEngineBase, (rp))

APTR __TT_AllocRequest(__reg("a6") struct Library *)="\tjsr\t-102(a6)";
#define TT_AllocRequest() __TT_AllocRequest(TTEngineBase)

struct TagItem* __TT_RequestA(__reg("a6") struct Library *, __reg("a0") APTR request, __reg("a1") struct TagItem * taglist)="\tjsr\t-108(a6)";
#define TT_RequestA(request, taglist) __TT_RequestA(TTEngineBase, (request), (taglist))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
struct TagItem* __TT_Request(__reg("a6") struct Library *, __reg("a0") APTR request, Tag taglist, ...)="\tmove.l\ta1,-(a7)\n\tlea\t4(a7),a1\n\tjsr\t-108(a6)\n\tmovea.l\t(a7)+,a1";
#define TT_Request(request, ...) __TT_Request(TTEngineBase, (request), __VA_ARGS__)
#endif

VOID __TT_FreeRequest(__reg("a6") struct Library *, __reg("a0") APTR request)="\tjsr\t-114(a6)";
#define TT_FreeRequest(request) __TT_FreeRequest(TTEngineBase, (request))

STRPTR * __TT_ObtainFamilyListA(__reg("a6") struct Library *, __reg("a0") struct TagItem * taglist)="\tjsr\t-120(a6)";
#define TT_ObtainFamilyListA(taglist) __TT_ObtainFamilyListA(TTEngineBase, (taglist))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
STRPTR * __TT_ObtainFamilyList(__reg("a6") struct Library *, Tag taglist, ...)="\tmove.l\ta0,-(a7)\n\tlea\t4(a7),a0\n\tjsr\t-120(a6)\n\tmovea.l\t(a7)+,a0";
#define TT_ObtainFamilyList(...) __TT_ObtainFamilyList(TTEngineBase, __VA_ARGS__)
#endif

VOID __TT_FreeFamilyList(__reg("a6") struct Library *, __reg("a0") STRPTR * list)="\tjsr\t-126(a6)";
#define TT_FreeFamilyList(list) __TT_FreeFamilyList(TTEngineBase, (list))

#endif /*  _VBCCINLINE_TTENGINE_H  */
