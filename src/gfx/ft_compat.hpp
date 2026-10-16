// ============================================================================
//  RAW UI - FreeType compatibility layer.
//
//  The FreeType headers shipped with the PS4 SDK keep all of the documentation
//  but have many of the FT_EXPORT prototypes stripped out (only 29 of them
//  survive in freetype.h).  The types, structs and enums are intact, so the
//  handful of functions RAW UI calls are declared here instead.
//
//  The signatures match FreeType 2.5.1 - the version libSceFreeType embeds -
//  and every symbol below is exported by that module.
// ============================================================================
#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_MULTIPLE_MASTERS_H

// Missing from the SDK headers.
#ifndef FT_MAKE_TAG
#define FT_MAKE_TAG(a, b, c, d)                                        \
    ((FT_ULong)(((FT_ULong)(a) << 24) | ((FT_ULong)(b) << 16) |        \
                ((FT_ULong)(c) << 8) | (FT_ULong)(d)))
#endif

extern "C" {

FT_Error FT_Init_FreeType(FT_Library* alibrary);
FT_Error FT_Done_FreeType(FT_Library library);

FT_Error FT_New_Memory_Face(FT_Library library, const FT_Byte* file_base,
                            FT_Long file_size, FT_Long face_index, FT_Face* aface);
FT_Error FT_New_Face(FT_Library library, const char* filepathname, FT_Long face_index,
                     FT_Face* aface);
FT_Error FT_Done_Face(FT_Face face);

FT_Error FT_Select_Charmap(FT_Face face, FT_Encoding encoding);

FT_Error FT_Load_Char(FT_Face face, FT_ULong char_code, FT_Int32 load_flags);
FT_Error FT_Load_Glyph(FT_Face face, FT_UInt glyph_index, FT_Int32 load_flags);
FT_Error FT_Render_Glyph(FT_GlyphSlot slot, FT_Render_Mode render_mode);
FT_Error FT_Get_Kerning(FT_Face face, FT_UInt left_glyph, FT_UInt right_glyph,
                        FT_UInt kern_mode, FT_Vector* akerning);

FT_UInt FT_Get_Char_Index(FT_Face face, FT_ULong charcode);

}  // extern "C"
