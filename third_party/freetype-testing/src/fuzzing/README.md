[![License: GPL v2](https://img.shields.io/badge/License-GPL%20v2-blue.svg)](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html)
[![Build Status](https://travis-ci.com/freetype/freetype2-testing.svg?branch=master)](https://travis-ci.com/freetype/freetype2-testing)

# FreeType

https://www.freetype.org

FreeType is a freely available software library to render fonts.

# Fuzzing

The **Fuzzing subproject** has two main purposes:

- **[OSS-Fuzz](https://github.com/google/oss-fuzz)**:
  Provide the source code and build scripts for OSS Fuzz's fuzzers.
- **[Travis CI](https://travis-ci.com/freetype/freetype2-testing)**:
  Provide settings, source code, and build scripts for the regression test suite.

## Structure

The general structure of this subproject is as follows:

- [**corpora**](corpora):    The initial corpora for OSS-Fuzz and the
                             regression tests.
- [**scripts**](scripts):    Scripts that build the fuzz targets and prepare
                             files (corpora and settings) for OSS-Fuzz.
- [**settings**](settings):  Various settings for the OSS-Fuzz targets as well
                             as for used submodules.
- [**src**](src):            Source code of the fuzz targets.

## Fuzzed Coverage

The fuzzers reach large parts of FreeType (1 September 2018):

|                   | Total       | Percent |
| ----------------- | ----------: | ------: |
| **Functions**     | 1462/1616   | 90.41%  |
| **Lines of Code** | 73866/83156 | 88.90%  |

## Fuzzed API

The following tables provide an overview over currently fuzzed and unfuzzed
parts of
[FreeType's API](https://www.freetype.org/freetype2/docs/reference/ft2-toc.html).
Columns have the following meaning:

- **Function**:   The name of an API function.
- **Module**:     The name of a module when set via
                  [`FT_Property_Set`](https://www.freetype.org/freetype2/docs/reference/ft2-module_management.html#FT_Property_Set).
- **Property**:   The name of a property when set via
                  [`FT_Property_Set`](https://www.freetype.org/freetype2/docs/reference/ft2-module_management.html#FT_Property_Set).
- **Fuzzed**:
    - (:wavy_dash:)         A function is used at least once, by one fuzz
                            target.
    - (:heavy_check_mark:)  A function is used in (almost) every possible way
                            and it is up the fuzzer to find inputs that fully
                            test it.
- **Resources**:  Some important resources that a function uses.  This list is
                  by no means complete.  It merely highlights important shared
                  resources that carry over from one function to another.
- **Alias**:      Other functions that do exactly the same.
- **Calls**:      Other API functions that a function calls.  Used to track
                  down implicitely used API functions.
- **Called by**:  Other API functions that call a function.  Used to track
                  down implicitely used API functions.

### FreeType Version

https://www.freetype.org/freetype2/docs/reference/ft2-version.html

|       | Function                       | Fuzzed             | Resources |
| ----- | ------------------------------ | :----------------: | --------- |
| 1.1.1 | `FT_Library_Version`           | :heavy_check_mark: | Library   |

### Base Interface

https://www.freetype.org/freetype2/docs/reference/ft2-base_interface.html

|       | Function           | Fuzzed             | Resources |
| ----- | ------------------ | :----------------: | --------- |
| 2.1.1 | `FT_Init_FreeType` | :heavy_check_mark: |           |
| 2.1.2 | `FT_Done_FreeType` | :heavy_check_mark: | Library   |

|       | Function             | Fuzzed             | Resources                | Calls | Called by        |
| ----- | -------------------- | :----------------: | ------------------------ | ----- | ---------------- |
| 2.2.1 | `FT_New_Face`        |                    | Library <br> File Path   |       |                  |
| 2.2.2 | `FT_Done_Face`       | :heavy_check_mark: | Face                     | 6.1.1 |                  |
| 2.2.3 | `FT_Reference_Face`  |                    | Face                     |       |                  |
| 2.2.4 | `FT_New_Memory_Face` | :heavy_check_mark: | Library <br> Bytes       | 2.2.6 |                  |
| 2.2.5 | `FT_Face_Properties` |                    | Face                     |       |                  |
| 2.2.6 | `FT_Open_Face`       | :heavy_check_mark: | Library                  | 6.1.2 | 2.2.4 <br> 2.2.8 |
| 2.2.7 | `FT_Attach_File`     |                    | Face <br> File Path      |       |                  |
| 2.2.8 | `FT_Attach_Stream`   | :heavy_check_mark: | Face <br> Bytes          | 2.2.6 |                  |

|       | Function             | Fuzzed             | Resources | Calls | Called by |
| ----- | -------------------- | :----------------: | --------- | ----- | --------- |
| 2.3.1 | `FT_Set_Char_Size`   | :heavy_check_mark: | Face      | 2.3.3 |           |
| 2.3.2 | `FT_Set_Pixel_Sizes` | :heavy_check_mark: | Face      |       |           |
| 2.3.3 | `FT_Request_Size`    | :heavy_check_mark: | Face      |       | 2.3.1     |
| 2.3.4 | `FT_Select_Size`     | :heavy_check_mark: | Face      |       |           |
| 2.3.5 | `FT_Set_Transform`   | :heavy_check_mark: | Face      |       |           |

|       | Function            | Fuzzed             | Resources                             | Calls            | Called by |
| ----- | ------------------- | :----------------: | ------------------------------------- | ---------------- | --------- |
| 2.4.1 | `FT_Load_Glyph`     | :heavy_check_mark: | Face <br> Glyph Index <br> Load Flags | 2.5.1            | 2.4.6     |
| 2.4.2 | `FT_Get_Char_Index` | :heavy_check_mark: | Face <br> Char Code                   |                  | 2.4.6     |
| 2.4.3 | `FT_Get_First_Char` | :heavy_check_mark: | Face                                  |                  |           |
| 2.4.4 | `FT_Get_Next_Char`  | :heavy_check_mark: | Face <br> Char Code                   |                  |           |
| 2.4.5 | `FT_Get_Name_Index` | :heavy_check_mark: | Face                                  |                  |           |
| 2.4.6 | `FT_Load_Char`      | :heavy_check_mark: | Face <br> Char Code <br> Load Flags   | 2.4.1 <br> 2.4.2 |           |

|       | Function            | Fuzzed             | Resources  | Called by |
| ----- | ------------------- | :----------------: | ---------- | --------- |
| 2.5.1 | `FT_Render_Glyph`   | :heavy_check_mark: | Glyph Slot | 2.4.1     |

|       | Function               | Fuzzed             | Resources                |
| ----- | ---------------------- | :----------------: | ------------------------ |
| 2.6.1 | `FT_Get_Kerning`       | :heavy_check_mark: | Face <br> 2x Glyph Index |
| 2.6.2 | `FT_Get_Track_Kerning` | :heavy_check_mark: | Face                     |

|       | Function                 | Fuzzed             | Resources             |
| ----- | ------------------------ | :----------------: | --------------------- |
| 2.7.1 | `FT_Get_Glyph_Name`      | :heavy_check_mark: | Face <br> Glyph Index |
| 2.7.2 | `FT_Get_Postscript_Name` | :heavy_check_mark: | Face                  |

|       | Function               | Fuzzed             | Resources          |
| ----- | ---------------------- | :----------------: | ------------------ |
| 2.8.1 | `FT_Select_Charmap`    | :heavy_check_mark: | Face               |
| 2.8.2 | `FT_Set_Charmap`       | :heavy_check_mark: | Face <br> Char Map |
| 2.8.3 | `FT_Get_Charmap_Index` | :heavy_check_mark: | Face <br> Char Map |

|       | Function               | Fuzzed             | Resources                      |
| ----- | ---------------------- | :----------------: | ------------------------------ |
| 2.9.1 | `FT_Get_FSType_Flags`  | :heavy_check_mark: | Face                           |
| 2.9.2 | `FT_Get_SubGlyph_Info` | :heavy_check_mark: | Glyph Slot <br> Subglyph Index |

### Unicode Variation Sequences

https://www.freetype.org/freetype2/docs/reference/ft2-glyph_variants.html

|       | Function                          | Fuzzed             | Resources                             |
| ----- | --------------------------------- | :----------------: | :-----------------------------------: |
| 3.1.1 | `FT_Face_GetCharVariantIndex`     | :heavy_check_mark: | Face <br> Char Code <br> Var Selector |
| 3.1.2 | `FT_Face_GetCharVariantIsDefault` | :heavy_check_mark: | Face <br> Char Code <br> Var Selector |
| 3.1.3 | `FT_Face_GetVariantSelectors`     | :heavy_check_mark: | Face                                  |
| 3.1.4 | `FT_Face_GetVariantsOfChar`       | :heavy_check_mark: | Face <br> Char Code                   |
| 3.1.5 | `FT_Face_GetCharsOfVariant`       | :heavy_check_mark: | Face <br> Var Selector                |

### Glyph Management

https://www.freetype.org/freetype2/docs/reference/ft2-glyph_management.html

|       | Function             | Fuzzed             | Resources  |
| ----- | -------------------- | :----------------: | ---------- |
| 4.1.1 | `FT_Get_Glyph`       | :heavy_check_mark: | Glyph Slot |
| 4.1.2 | `FT_Glyph_Copy`      | :heavy_check_mark: | Glyph      |
| 4.1.3 | `FT_Glyph_Transform` | :heavy_check_mark: | Glyph      |
| 4.1.4 | `FT_Glyph_Get_CBox`  | :heavy_check_mark: | Glyph      |
| 4.1.5 | `FT_Glyph_To_Bitmap` | :heavy_check_mark: | Glyph      |
| 4.1.6 | `FT_Done_Glyph`      | :heavy_check_mark: | Glyph      |

### Mac Specific Interface

https://www.freetype.org/freetype2/docs/reference/ft2-mac_specific.html

|       | Function                           | Fuzzed |
| ----- | ---------------------------------- | :----: |
| 5.1.1 | `FT_New_Face_From_FOND`            |        |
| 5.1.2 | `FT_GetFile_From_Mac_Name`         |        |
| 5.1.3 | `FT_GetFile_From_Mac_ATS_Name`     |        |
| 5.1.4 | `FT_GetFilePath_From_Mac_ATS_Name` |        |
| 5.1.5 | `FT_New_Face_From_FSSpec`          |        |
| 5.1.6 | `FT_New_Face_From_FSRef`           |        |

### Size Management

https://www.freetype.org/freetype2/docs/reference/ft2-sizes_management.html

|       | Function           | Fuzzed             | Resources | Called by |
| ----- | ------------------ | :----------------: | --------- | --------- |
| 6.1.1 | `FT_New_Size`      | :heavy_check_mark: | Face      | 2.2.6     |
| 6.1.2 | `FT_Done_Size`     | :heavy_check_mark: | Size      | 2.2.2     |
| 6.1.3 | `FT_Activate_Size` |                    | Size      |           |

### Multiple Masters

https://www.freetype.org/freetype2/docs/reference/ft2-multiple_masters.html

|        | Function                        | Fuzzed             | Resources           | Alias  |
| ------ | ------------------------------- | :----------------: | :-----------------: | :----: |
| 7.1.1  | `FT_Get_Multi_Master`           | :heavy_check_mark: | Face                |        |
| 7.1.2  | `FT_Get_MM_Var`                 | :heavy_check_mark: | Face                |        |
| 7.1.3  | `FT_Done_MM_Var`                | :heavy_check_mark: | Library <br> MM Var |        |
| 7.1.4  | `FT_Set_MM_Design_Coordinates`  | :heavy_check_mark: | Face                |        |
| 7.1.5  | `FT_Set_Var_Design_Coordinates` | :heavy_check_mark: | Face                |        |
| 7.1.6  | `FT_Get_Var_Design_Coordinates` | :heavy_check_mark: | Face                |        |
| 7.1.7  | `FT_Set_MM_Blend_Coordinates`   | :wavy_dash:        | Face                | 7.1.9  |
| 7.1.8  | `FT_Get_MM_Blend_Coordinates`   | :heavy_check_mark: | Face                | 7.1.10 |
| 7.1.9  | `FT_Set_Var_Blend_Coordinates`  | :wavy_dash:        | Face                | 7.1.7  |
| 7.1.10 | `FT_Get_Var_Blend_Coordinates`  | :heavy_check_mark: | Face                | 7.1.8  |
| 7.1.11 | `FT_Get_Var_Axis_Flags`         | :heavy_check_mark: | MM Var              |        |
| 7.1.12 | `FT_Set_Named_Instance`         | :wavy_dash:        | Face                |        |

### TrueType Tables

https://www.freetype.org/freetype2/docs/reference/ft2-truetype_tables.html

|       | Function                  | Fuzzed             | Resources |
| ----- | ------------------------- | :----------------: | --------- |
| 8.1.1 | `FT_Get_Sfnt_Table`       | :heavy_check_mark: | Face      |
| 8.1.2 | `FT_Load_Sfnt_Table`      | :wavy_dash:        | Face      |
| 8.1.3 | `FT_Sfnt_Table_Info`      | :heavy_check_mark: | Face      |
| 8.1.4 | `FT_Get_CMap_Language_ID` | :heavy_check_mark: | Char Map  |
| 8.1.5 | `FT_Get_CMap_Format`      | :heavy_check_mark: | Char Map  |

### Type 1 Tables

https://www.freetype.org/freetype2/docs/reference/ft2-type1_tables.html

|       | Function                 | Fuzzed             | Resources  |
| ----- | ------------------------ | :----------------: | :--------: |
| 9.1.1 | `FT_Has_PS_Glyph_Names`  | :heavy_check_mark: | Face       |
| 9.1.2 | `FT_Get_PS_Font_Info`    | :heavy_check_mark: | Face       |
| 9.1.3 | `FT_Get_PS_Font_Private` | :heavy_check_mark: | Face       |
| 9.1.4 | `FT_Get_PS_Font_Value`   | :heavy_check_mark: | Face       |

### SFNT Names

https://www.freetype.org/freetype2/docs/reference/ft2-sfnt_names.html

|        | Function                 | Fuzzed             | Resources |
| ------ | ------------------------ | :----------------: | :-------: |
| 10.1.1 | `FT_Get_Sfnt_Name_Count` | :heavy_check_mark: | Face      |
| 10.1.2 | `FT_Get_Sfnt_Name`       | :heavy_check_mark: | Face      |
| 10.1.3 | `FT_Get_Sfnt_LangTag`    | :heavy_check_mark: | Face      |

### BDF and PCF Files

https://www.freetype.org/freetype2/docs/reference/ft2-bdf_fonts.html

|        | Function                | Fuzzed             |
| ------ | ----------------------- | :----------------: |
| 11.1.1 | `FT_Get_BDF_Charset_ID` | :heavy_check_mark: |
| 11.1.2 | `FT_Get_BDF_Property`   |                    |

### CID Fonts

https://www.freetype.org/freetype2/docs/reference/ft2-cid_fonts.html

|        | Function                                  | Fuzzed             | Resources             |
| ------ | ----------------------------------------- | :----------------: | --------------------- |
| 12.1.1 | `FT_Get_CID_Registry_Ordering_Supplement` | :heavy_check_mark: | Face                  |
| 12.1.2 | `FT_Get_CID_Is_Internally_CID_Keyed`      | :heavy_check_mark: | Face                  |
| 12.1.3 | `FT_Get_CID_From_Glyph_Index`             | :heavy_check_mark: | Face <br> Glyph Index |

### PFR Fonts

https://www.freetype.org/freetype2/docs/reference/ft2-pfr_fonts.html

|        | Function             | Fuzzed |
| ------ | -------------------- | :----: |
| 13.1.1 | `FT_Get_PFR_Metrics` |        |
| 13.1.2 | `FT_Get_PFR_Kerning` |        |
| 13.1.3 | `FT_Get_PFR_Advance` |        |

### Window FNT Files

https://www.freetype.org/freetype2/docs/reference/ft2-winfnt_fonts.html

|        | Function               | Fuzzed             | Resources |
| ------ | ---------------------- | :----------------: | --------- |
| 14.1.1 | `FT_Get_WinFNT_Header` | :heavy_check_mark: | Face      |

### Font Formats

https://www.freetype.org/freetype2/docs/reference/ft2-font_formats.html

|        | Function             | Fuzzed             | Resources |
| ------ | -------------------- | :----------------: | --------- |
| 15.1.1 | `FT_Get_Font_Format` | :heavy_check_mark: | Face      |

### Gasp Table

https://www.freetype.org/freetype2/docs/reference/ft2-gasp_table.html

|        | Function      | Fuzzed             | Resources |
| ------ | ------------- | :----------------: | --------- |
| 16.1.1 | `FT_Get_Gasp` | :heavy_check_mark: | Face      |

### Driver Properties

https://www.freetype.org/freetype2/docs/reference/ft2-properties.html

|         | Module       | Property               | Fuzzed             |
| ------- | ------------ | ---------------------- | :----------------: |
| 17.1.1  | `autofitter` | `darkening-parameters` |                    |
| 17.1.2  | `autofitter` | `default-script`       |                    |
| 17.1.3  | `autofitter` | `fallback-script`      |                    |
| 17.1.4  | `autofitter` | `increase-x-height`    |                    |
| 17.1.5  | `autofitter` | `no-stem-darkening`    |                    |
| 17.1.6  | `autofitter` | `warping `             | :heavy_check_mark: |
| 17.1.7  | `cff`        | `darkening-parameters` |                    |
| 17.1.8  | `cff`        | `hinting-engine`       | :heavy_check_mark: |
| 17.1.9  | `cff`        | `no-stem-darkening`    |                    |
| 17.1.10 | `cff`        | `random-seed`          |                    |
| 17.1.11 | `pcf`        | `no-long-family-names` |                    |
| 17.1.12 | `t1cid`      | `darkening-parameters` |                    |
| 17.1.13 | `t1cid`      | `hinting-engine`       | :heavy_check_mark: |
| 17.1.14 | `t1cid`      | `no-stem-darkening`    |                    |
| 17.1.15 | `t1cid`      | `random-seed`          |                    |
| 17.1.16 | `truetype`   | `interpreter-version ` | :heavy_check_mark: |
| 17.1.17 | `type1`      | `darkening-parameters` |                    |
| 17.1.18 | `type1`      | `hinting-engine`       | :heavy_check_mark: |
| 17.1.19 | `type1`      | `no-stem-darkening`    |                    |
| 17.1.20 | `type1`      | `random-seed`          |                    |

### Cache Sub-System

https://www.freetype.org/freetype2/docs/reference/ft2-cache_subsystem.html

|         | Function                      | Fuzzed |
| ------- | ----------------------------- | :----: |
| 18.1.1  | `FTC_Manager_New`             |        |
| 18.1.2  | `FTC_Manager_Reset`           |        |
| 18.1.3  | `FTC_Manager_Done`            |        |
| 18.1.4  | `FTC_Manager_LookupFace`      |        |
| 18.1.5  | `FTC_Manager_LookupSize`      |        |
| 18.1.6  | `FTC_Manager_RemoveFaceID`    |        |
| 18.1.7  | `FTC_Node_Unref`              |        |
| 18.1.8  | `FTC_ImageCache_New`          |        |
| 18.1.9  | `FTC_ImageCache_Lookup`       |        |
| 18.1.10 | `FTC_SBitCache_New`           |        |
| 18.1.11 | `FTC_SBitCache_Lookup`        |        |
| 18.1.12 | `FTC_CMapCache_New`           |        |
| 18.1.13 | `FTC_CMapCache_Lookup`        |        |
| 18.1.14 | `FTC_ImageCache_LookupScaler` |        |
| 18.1.15 | `FTC_SBitCache_LookupScaler`  |        |

### Outline Processing

https://www.freetype.org/freetype2/docs/reference/ft2-outline_processing.html

|         | Function                     | Fuzzed             | Resources            |
| ------- | ---------------------------- | :----------------: | -------------------- |
| 21.1.1  | `FT_Outline_New`             | :heavy_check_mark: | Library              |
| 21.1.2  | `FT_Outline_Done`            | :heavy_check_mark: | Library <br> Outline |
| 21.1.3  | `FT_Outline_Copy`            | :heavy_check_mark: | Outline              |
| 21.1.4  | `FT_Outline_Translate`       | :heavy_check_mark: | Outline              |
| 21.1.5  | `FT_Outline_Transform`       | :heavy_check_mark: | Outline              |
| 21.1.6  | `FT_Outline_Embolden`        | :heavy_check_mark: | Outline              |
| 21.1.7  | `FT_Outline_EmboldenXY`      | :heavy_check_mark: | Outline              |
| 21.1.8  | `FT_Outline_Reverse`         | :heavy_check_mark: | Outline              |
| 21.1.9  | `FT_Outline_Check`           | :heavy_check_mark: | Outline              |
| 21.1.10 | `FT_Outline_Get_CBox`        | :heavy_check_mark: | Outline              |
| 21.1.11 | `FT_Outline_Get_BBox`        | :heavy_check_mark: | Outline              |
| 21.1.12 | `FT_Outline_Get_Bitmap`      |                    | Library <br> Outline |
| 21.1.13 | `FT_Outline_Render`          |                    | Library <br> Outline |
| 21.1.14 | `FT_Outline_Decompose`       | :heavy_check_mark: | Outline              |
| 21.1.15 | `FT_Outline_Get_Orientation` | :heavy_check_mark: | Outline              |

### Bitmap Handling

https://www.freetype.org/freetype2/docs/reference/ft2-bitmap_handling.html

|        | Function                  | Fuzzed             | Resources           |
| ------ | ------------------------- | :----------------: | ------------------- |
| 23.1.1 | `FT_Bitmap_Init`          | :heavy_check_mark: |                     |
| 23.1.2 | `FT_Bitmap_Copy`          | :heavy_check_mark: | Library <br> Bitmap |
| 23.1.3 | `FT_Bitmap_Embolden`      | :heavy_check_mark: | Library <br> Bitmap |
| 23.1.4 | `FT_Bitmap_Convert`       | :heavy_check_mark: | Library <br> Bitmap |
| 23.1.5 | `FT_Bitmap_Blend`         | :wavy_dash:        | Library <br> Bitmap |
| 23.1.6 | `FT_GlyphSlot_Own_Bitmap` |                    | Glyph Slot          |
| 23.1.7 | `FT_Bitmap_Done`          | :heavy_check_mark: | Library <br> Bitmap |

### COLRv1 Methods

Not yet released, see documentation in [freetype.h](https://gitlab.freedesktop.org/freetype/freetype/-/blob/master/include/freetype/freetype.h#L4137)

|    | Function                   | Fuzzed             | Resouces                        |
| -- | -------------------------- | :----------------: | ------------------------------- |
|    | `FT_Get_Color_Glyph_Paint` | :heavy_check_mark: | Face <br> Glyph Index           |
|    | `FT_Get_Paint_Layers`      | :heavy_check_mark: | Face <br> COLRv1 Layer Iterator |
|    | `FT_Get_Colorline_Stops`   | :heavy_check_mark: | Face <br> COLRv1 Color Iterator |
|    | `FT_Get_Paint`             | :heavy_check_mark: | Face <br> COLRv1 Opaque Paint   |

### GZIP Streams

https://www.freetype.org/freetype2/docs/reference/ft2-gzip.html

|        | Function             | Fuzzed             |
| ------ | -------------------- | :----------------: |
| 28.1.1 | `FT_Stream_OpenGzip` | :heavy_check_mark: |
| 28.1.2 | `FT_Gzip_Uncompress` |                    |

### LZW Streams

https://www.freetype.org/freetype2/docs/reference/ft2-lzw.html

|        | Function            | Fuzzed             |
| ------ | ------------------- | :----------------: |
| 29.1.1 | `FT_Stream_OpenLZW` | :heavy_check_mark: |

### BZIP2 Streams

https://www.freetype.org/freetype2/docs/reference/ft2-bzip2.html

|        | Function              | Fuzzed             |
| ------ | --------------------- | :----------------: |
| 30.1.1 | `FT_Stream_OpenBzip2` | :heavy_check_mark: |

