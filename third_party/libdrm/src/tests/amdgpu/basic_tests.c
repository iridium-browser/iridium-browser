/*
 * Copyright 2014 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#ifdef MAJOR_IN_SYSMACROS
#include <sys/sysmacros.h>
#endif
#include <sys/stat.h>
#include <fcntl.h>
#ifdef HAVE_ALLOCA_H
# include <alloca.h>
#endif
#include <sys/wait.h>

#include "CUnit/Basic.h"

#include "amdgpu_test.h"
#include "amdgpu_drm.h"
#include "util_math.h"

static  amdgpu_device_handle device_handle;
static  uint32_t  major_version;
static  uint32_t  minor_version;
static  uint32_t  family_id;

static void amdgpu_query_info_test(void);
static void amdgpu_command_submission_gfx(void);
static void amdgpu_command_submission_compute(void);
static void amdgpu_command_submission_multi_fence(void);
static void amdgpu_command_submission_sdma(void);
static void amdgpu_userptr_test(void);
static void amdgpu_semaphore_test(void);
static void amdgpu_sync_dependency_test(void);
static void amdgpu_bo_eviction_test(void);
static void amdgpu_compute_dispatch_test(void);
static void amdgpu_gfx_dispatch_test(void);
static void amdgpu_draw_test(void);
static void amdgpu_gpu_reset_test(void);

static void amdgpu_command_submission_write_linear_helper(unsigned ip_type);
static void amdgpu_command_submission_const_fill_helper(unsigned ip_type);
static void amdgpu_command_submission_copy_linear_helper(unsigned ip_type);
static void amdgpu_test_exec_cs_helper(amdgpu_context_handle context_handle,
				       unsigned ip_type,
				       int instance, int pm4_dw, uint32_t *pm4_src,
				       int res_cnt, amdgpu_bo_handle *resources,
				       struct amdgpu_cs_ib_info *ib_info,
				       struct amdgpu_cs_request *ibs_request);
 
CU_TestInfo basic_tests[] = {
	{ "Query Info Test",  amdgpu_query_info_test },
	{ "Userptr Test",  amdgpu_userptr_test },
	{ "bo eviction Test",  amdgpu_bo_eviction_test },
	{ "Command submission Test (GFX)",  amdgpu_command_submission_gfx },
	{ "Command submission Test (Compute)", amdgpu_command_submission_compute },
	{ "Command submission Test (Multi-Fence)", amdgpu_command_submission_multi_fence },
	{ "Command submission Test (SDMA)", amdgpu_command_submission_sdma },
	{ "SW semaphore Test",  amdgpu_semaphore_test },
	{ "Sync dependency Test",  amdgpu_sync_dependency_test },
	{ "Dispatch Test (Compute)",  amdgpu_compute_dispatch_test },
	{ "Dispatch Test (GFX)",  amdgpu_gfx_dispatch_test },
	{ "Draw Test",  amdgpu_draw_test },
	{ "GPU reset Test", amdgpu_gpu_reset_test },
	CU_TEST_INFO_NULL,
};
#define BUFFER_SIZE (8 * 1024)
#define SDMA_PKT_HEADER_op_offset 0
#define SDMA_PKT_HEADER_op_mask   0x000000FF
#define SDMA_PKT_HEADER_op_shift  0
#define SDMA_PKT_HEADER_OP(x) (((x) & SDMA_PKT_HEADER_op_mask) << SDMA_PKT_HEADER_op_shift)
#define SDMA_OPCODE_CONSTANT_FILL  11
#       define SDMA_CONSTANT_FILL_EXTRA_SIZE(x)           ((x) << 14)
	/* 0 = byte fill
	 * 2 = DW fill
	 */
#define SDMA_PACKET(op, sub_op, e)	((((e) & 0xFFFF) << 16) |	\
					(((sub_op) & 0xFF) << 8) |	\
					(((op) & 0xFF) << 0))
#define	SDMA_OPCODE_WRITE				  2
#       define SDMA_WRITE_SUB_OPCODE_LINEAR               0
#       define SDMA_WRTIE_SUB_OPCODE_TILED                1

#define	SDMA_OPCODE_COPY				  1
#       define SDMA_COPY_SUB_OPCODE_LINEAR                0

#define GFX_COMPUTE_NOP  0xffff1000
#define SDMA_NOP  0x0

/* PM4 */
#define	PACKET_TYPE0	0
#define	PACKET_TYPE1	1
#define	PACKET_TYPE2	2
#define	PACKET_TYPE3	3

#define CP_PACKET_GET_TYPE(h) (((h) >> 30) & 3)
#define CP_PACKET_GET_COUNT(h) (((h) >> 16) & 0x3FFF)
#define CP_PACKET0_GET_REG(h) ((h) & 0xFFFF)
#define CP_PACKET3_GET_OPCODE(h) (((h) >> 8) & 0xFF)
#define PACKET0(reg, n)	((PACKET_TYPE0 << 30) |				\
			 ((reg) & 0xFFFF) |			\
			 ((n) & 0x3FFF) << 16)
#define CP_PACKET2			0x80000000
#define		PACKET2_PAD_SHIFT		0
#define		PACKET2_PAD_MASK		(0x3fffffff << 0)

#define PACKET2(v)	(CP_PACKET2 | REG_SET(PACKET2_PAD, (v)))

#define PACKET3(op, n)	((PACKET_TYPE3 << 30) |				\
			 (((op) & 0xFF) << 8) |				\
			 ((n) & 0x3FFF) << 16)
#define PACKET3_COMPUTE(op, n) PACKET3(op, n) | (1 << 1)

/* Packet 3 types */
#define	PACKET3_NOP					0x10

#define	PACKET3_WRITE_DATA				0x37
#define		WRITE_DATA_DST_SEL(x)                   ((x) << 8)
		/* 0 - register
		 * 1 - memory (sync - via GRBM)
		 * 2 - gl2
		 * 3 - gds
		 * 4 - reserved
		 * 5 - memory (async - direct)
		 */
#define		WR_ONE_ADDR                             (1 << 16)
#define		WR_CONFIRM                              (1 << 20)
#define		WRITE_DATA_CACHE_POLICY(x)              ((x) << 25)
		/* 0 - LRU
		 * 1 - Stream
		 */
#define		WRITE_DATA_ENGINE_SEL(x)                ((x) << 30)
		/* 0 - me
		 * 1 - pfp
		 * 2 - ce
		 */

#define	PACKET3_DMA_DATA				0x50
/* 1. header
 * 2. CONTROL
 * 3. SRC_ADDR_LO or DATA [31:0]
 * 4. SRC_ADDR_HI [31:0]
 * 5. DST_ADDR_LO [31:0]
 * 6. DST_ADDR_HI [7:0]
 * 7. COMMAND [30:21] | BYTE_COUNT [20:0]
 */
/* CONTROL */
#              define PACKET3_DMA_DATA_ENGINE(x)     ((x) << 0)
		/* 0 - ME
		 * 1 - PFP
		 */
#              define PACKET3_DMA_DATA_SRC_CACHE_POLICY(x) ((x) << 13)
		/* 0 - LRU
		 * 1 - Stream
		 * 2 - Bypass
		 */
#              define PACKET3_DMA_DATA_SRC_VOLATILE (1 << 15)
#              define PACKET3_DMA_DATA_DST_SEL(x)  ((x) << 20)
		/* 0 - DST_ADDR using DAS
		 * 1 - GDS
		 * 3 - DST_ADDR using L2
		 */
#              define PACKET3_DMA_DATA_DST_CACHE_POLICY(x) ((x) << 25)
		/* 0 - LRU
		 * 1 - Stream
		 * 2 - Bypass
		 */
#              define PACKET3_DMA_DATA_DST_VOLATILE (1 << 27)
#              define PACKET3_DMA_DATA_SRC_SEL(x)  ((x) << 29)
		/* 0 - SRC_ADDR using SAS
		 * 1 - GDS
		 * 2 - DATA
		 * 3 - SRC_ADDR using L2
		 */
#              define PACKET3_DMA_DATA_CP_SYNC     (1 << 31)
/* COMMAND */
#              define PACKET3_DMA_DATA_DIS_WC      (1 << 21)
#              define PACKET3_DMA_DATA_CMD_SRC_SWAP(x) ((x) << 22)
		/* 0 - none
		 * 1 - 8 in 16
		 * 2 - 8 in 32
		 * 3 - 8 in 64
		 */
#              define PACKET3_DMA_DATA_CMD_DST_SWAP(x) ((x) << 24)
		/* 0 - none
		 * 1 - 8 in 16
		 * 2 - 8 in 32
		 * 3 - 8 in 64
		 */
#              define PACKET3_DMA_DATA_CMD_SAS     (1 << 26)
		/* 0 - memory
		 * 1 - register
		 */
#              define PACKET3_DMA_DATA_CMD_DAS     (1 << 27)
		/* 0 - memory
		 * 1 - register
		 */
#              define PACKET3_DMA_DATA_CMD_SAIC    (1 << 28)
#              define PACKET3_DMA_DATA_CMD_DAIC    (1 << 29)
#              define PACKET3_DMA_DATA_CMD_RAW_WAIT  (1 << 30)

#define SDMA_PACKET_SI(op, b, t, s, cnt)	((((op) & 0xF) << 28) |	\
						(((b) & 0x1) << 26) |		\
						(((t) & 0x1) << 23) |		\
						(((s) & 0x1) << 22) |		\
						(((cnt) & 0xFFFFF) << 0))
#define	SDMA_OPCODE_COPY_SI	3
#define SDMA_OPCODE_CONSTANT_FILL_SI	13
#define SDMA_NOP_SI  0xf
#define GFX_COMPUTE_NOP_SI 0x80000000
#define	PACKET3_DMA_DATA_SI	0x41
#              define PACKET3_DMA_DATA_SI_ENGINE(x)     ((x) << 27)
		/* 0 - ME
		 * 1 - PFP
		 */
#              define PACKET3_DMA_DATA_SI_DST_SEL(x)  ((x) << 20)
		/* 0 - DST_ADDR using DAS
		 * 1 - GDS
		 * 3 - DST_ADDR using L2
		 */
#              define PACKET3_DMA_DATA_SI_SRC_SEL(x)  ((x) << 29)
		/* 0 - SRC_ADDR using SAS
		 * 1 - GDS
		 * 2 - DATA
		 * 3 - SRC_ADDR using L2
		 */
#              define PACKET3_DMA_DATA_SI_CP_SYNC     (1 << 31)


#define PKT3_CONTEXT_CONTROL                   0x28
#define     CONTEXT_CONTROL_LOAD_ENABLE(x)     (((unsigned)(x) & 0x1) << 31)
#define     CONTEXT_CONTROL_LOAD_CE_RAM(x)     (((unsigned)(x) & 0x1) << 28)
#define     CONTEXT_CONTROL_SHADOW_ENABLE(x)   (((unsigned)(x) & 0x1) << 31)

#define PKT3_CLEAR_STATE                       0x12

#define PKT3_SET_SH_REG                        0x76
#define		PACKET3_SET_SH_REG_START			0x00002c00

#define	PACKET3_DISPATCH_DIRECT				0x15
#define PACKET3_EVENT_WRITE				0x46
#define PACKET3_ACQUIRE_MEM				0x58
#define PACKET3_SET_CONTEXT_REG				0x69
#define PACKET3_SET_UCONFIG_REG				0x79
#define PACKET3_DRAW_INDEX_AUTO				0x2D
/* gfx 8 */
#define mmCOMPUTE_PGM_LO                                                        0x2e0c
#define mmCOMPUTE_PGM_RSRC1                                                     0x2e12
#define mmCOMPUTE_TMPRING_SIZE                                                  0x2e18
#define mmCOMPUTE_USER_DATA_0                                                   0x2e40
#define mmCOMPUTE_USER_DATA_1                                                   0x2e41
#define mmCOMPUTE_RESOURCE_LIMITS                                               0x2e15
#define mmCOMPUTE_NUM_THREAD_X                                                  0x2e07



#define SWAP_32(num) (((num & 0xff000000) >> 24) | \
		      ((num & 0x0000ff00) << 8) | \
		      ((num & 0x00ff0000) >> 8) | \
		      ((num & 0x000000ff) << 24))


/* Shader code
 * void main()
{

	float x = some_input;
		for (unsigned i = 0; i < 1000000; i++)
  	x = sin(x);

	u[0] = 42u;
}
*/

static  uint32_t shader_bin[] = {
	SWAP_32(0x800082be), SWAP_32(0x02ff08bf), SWAP_32(0x7f969800), SWAP_32(0x040085bf),
	SWAP_32(0x02810281), SWAP_32(0x02ff08bf), SWAP_32(0x7f969800), SWAP_32(0xfcff84bf),
	SWAP_32(0xff0083be), SWAP_32(0x00f00000), SWAP_32(0xc10082be), SWAP_32(0xaa02007e),
	SWAP_32(0x000070e0), SWAP_32(0x00000080), SWAP_32(0x000081bf)
};

#define CODE_OFFSET 512
#define DATA_OFFSET 1024

enum cs_type {
	CS_BUFFERCLEAR,
	CS_BUFFERCOPY
};

static const uint32_t bufferclear_cs_shader_gfx9[] = {
    0xD1FD0000, 0x04010C08, 0x7E020204, 0x7E040205,
    0x7E060206, 0x7E080207, 0xE01C2000, 0x80000100,
    0xBF810000
};

static const uint32_t bufferclear_cs_shader_registers_gfx9[][2] = {
	{0x2e12, 0x000C0041},	//{ mmCOMPUTE_PGM_RSRC1,	  0x000C0041 },
	{0x2e13, 0x00000090},	//{ mmCOMPUTE_PGM_RSRC2,	  0x00000090 },
	{0x2e07, 0x00000040},	//{ mmCOMPUTE_NUM_THREAD_X, 0x00000040 },
	{0x2e08, 0x00000001},	//{ mmCOMPUTE_NUM_THREAD_Y, 0x00000001 },
	{0x2e09, 0x00000001},	//{ mmCOMPUTE_NUM_THREAD_Z, 0x00000001 }
};

static const uint32_t bufferclear_cs_shader_registers_num_gfx9 = 5;

static const uint32_t buffercopy_cs_shader_gfx9[] = {
    0xD1FD0000, 0x04010C08, 0xE00C2000, 0x80000100,
    0xBF8C0F70, 0xE01C2000, 0x80010100, 0xBF810000
};

static const uint32_t preamblecache_gfx9[] = {
	0xc0026900, 0x81, 0x80000000, 0x40004000, 0xc0026900, 0x8c, 0xaa99aaaa, 0x0,
	0xc0026900, 0x90, 0x80000000, 0x40004000, 0xc0026900, 0x94, 0x80000000, 0x40004000,
	0xc0026900, 0xb4,  0x0, 0x3f800000, 0xc0016900, 0x103, 0x0,
	0xc0016900, 0x208, 0x0, 0xc0016900, 0x290, 0x0,
	0xc0016900, 0x2a1, 0x0, 0xc0026900, 0x2ad, 0x0, 0x0,
	0xc0016900, 0x2d5, 0x10000, 0xc0016900,  0x2dc, 0x0,
	0xc0066900, 0x2de, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xc0026900, 0x2e5, 0x0, 0x0,
	0xc0056900, 0x2f9, 0x5, 0x3f800000, 0x3f800000, 0x3f800000, 0x3f800000,
	0xc0036900, 0x311, 0x3, 0, 0x100000, 0xc0026900, 0x316, 0x1e, 0x20,
	0xc0016900, 0x349, 0x0, 0xc0016900, 0x358, 0x0, 0xc0016900, 0x367, 0x0,
	0xc0016900, 0x376, 0x0, 0xc0016900, 0x385, 0x0, 0xc0016900, 0x19, 0x0,
	0xc0056900, 0xe8, 0x0, 0x0, 0x0, 0x0, 0x0,
	0xc0076900, 0x1e1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0xc0026900, 0x204, 0x90000, 0x4, 0xc0046900, 0x20c, 0x0, 0x0, 0x0, 0x0,
	0xc0016900, 0x2b2, 0x0, 0xc0026900, 0x30e, 0xffffffff, 0xffffffff,
	0xc0016900, 0x314, 0x0, 0xc0016900, 0x2a6, 0, 0xc0016900, 0x210, 0,
	0xc0002f00, 0x1, 0xc0016900, 0x1, 0x1,
	0xc0016900, 0x18, 0x2, 0xc0016900, 0x206, 0x300, 0xc0017900, 0x20000243, 0x0,
	0xc0017900, 0x248, 0xffffffff, 0xc0017900, 0x249, 0x0, 0xc0017900, 0x24a, 0x0,
	0xc0017900, 0x24b, 0x0
};

enum ps_type {
	PS_CONST,
	PS_TEX
};

static const uint32_t ps_const_shader_gfx9[] = {
    0x7E000200, 0x7E020201, 0x7E040202, 0x7E060203,
    0xD2960000, 0x00020300, 0xD2960001, 0x00020702,
    0xC4001C0F, 0x00000100, 0xBF810000
};

static const uint32_t ps_const_shader_patchinfo_code_size_gfx9 = 6;

static const uint32_t ps_const_shader_patchinfo_code_gfx9[][10][6] = {
    {{ 0xBF800000, 0xBF800000, 0xBF800000, 0xBF800000, 0xC4001890, 0x00000000 },
     { 0xBF800000, 0xBF800000, 0xBF800000, 0xBF800000, 0xC4001801, 0x00000000 },
     { 0xBF800000, 0xBF800000, 0xBF800000, 0xBF800000, 0xC4001803, 0x00000100 },
     { 0xBF800000, 0xBF800000, 0xBF800000, 0xBF800000, 0xC4001803, 0x00000300 },
     { 0xD2960000, 0x00020300, 0xD2960001, 0x00020702, 0xC4001C0F, 0x00000100 },
     { 0xD2950000, 0x00020300, 0xD2950001, 0x00020702, 0xC4001C0F, 0x00000100 },
     { 0xD2940000, 0x00020300, 0xD2940001, 0x00020702, 0xC4001C0F, 0x00000100 },
     { 0xD2970000, 0x00020300, 0xD2970001, 0x00020702, 0xC4001C0F, 0x00000100 },
     { 0xD2980000, 0x00020300, 0xD2980001, 0x00020702, 0xC4001C0F, 0x00000100 },
     { 0xBF800000, 0xBF800000, 0xBF800000, 0xBF800000, 0xC400180F, 0x03020100 }
    }
};

static const uint32_t ps_const_shader_patchinfo_offset_gfx9[] = {
    0x00000004
};

static const uint32_t ps_num_sh_registers_gfx9 = 2;

static const uint32_t ps_const_sh_registers_gfx9[][2] = {
    {0x2C0A, 0x000C0040},//{ mmSPI_SHADER_PGM_RSRC1_PS, 0x000C0040 },
    {0x2C0B, 0x00000008}, //{ mmSPI_SHADER_PGM_RSRC2_PS, 0x00000008 }
};

static const uint32_t ps_num_context_registers_gfx9 = 7;

static const uint32_t ps_const_context_reg_gfx9[][2] = {
    {0xA1B4, 0x00000002}, //{ mmSPI_PS_INPUT_ADDR,       0x00000002 },
    {0xA1B6, 0x00000000}, //{ mmSPI_PS_IN_CONTROL,       0x00000000 },
    {0xA08F, 0x0000000F}, //{ mmCB_SHADER_MASK,          0x0000000F },
    {0xA203, 0x00000010}, //{ mmDB_SHADER_CONTROL,       0x00000010 },
    {0xA1C4, 0x00000000}, //{ mmSPI_SHADER_Z_FORMAT,     0x00000000 },
    {0xA1B8, 0x00000000}, //{ mmSPI_BARYC_CNTL,          0x00000000 /* Always 0 for now */},
    {0xA1C5, 0x00000004}, //{ mmSPI_SHADER_COL_FORMAT,   0x00000004 }
};

static const uint32_t ps_tex_shader_gfx9[] = {
    0xBEFC000C, 0xBE8E017E, 0xBEFE077E, 0xD4180000,
    0xD4190001, 0xD41C0100, 0xD41D0101, 0xF0800F00,
    0x00400206, 0xBEFE010E, 0xBF8C0F70, 0xD2960000,
    0x00020702, 0xD2960001, 0x00020B04, 0xC4001C0F,
    0x00000100, 0xBF810000
};

static const uint32_t ps_tex_shader_patchinfo_offset_gfx9[] = {
    0x0000000B
};

static const uint32_t ps_tex_shader_patchinfo_code_size_gfx9 = 6;

static const uint32_t ps_tex_shader_patchinfo_code_gfx9[][10][6] = {
    {{ 0xBF800000, 0xBF800000, 0xBF800000, 0xBF800000, 0xC4001890, 0x00000000 },
     { 0xBF800000, 0xBF800000, 0xBF800000, 0xBF800000, 0xC4001801, 0x00000002 },
     { 0xBF800000, 0xBF800000, 0xBF800000, 0xBF800000, 0xC4001803, 0x00000302 },
     { 0xBF800000, 0xBF800000, 0xBF800000, 0xBF800000, 0xC4001803, 0x00000502 },
     { 0xD2960000, 0x00020702, 0xD2960001, 0x00020B04, 0xC4001C0F, 0x00000100 },
     { 0xD2950000, 0x00020702, 0xD2950001, 0x00020B04, 0xC4001C0F, 0x00000100 },
     { 0xD2940000, 0x00020702, 0xD2940001, 0x00020B04, 0xC4001C0F, 0x00000100 },
     { 0xD2970000, 0x00020702, 0xD2970001, 0x00020B04, 0xC4001C0F, 0x00000100 },
     { 0xD2980000, 0x00020702, 0xD2980001, 0x00020B04, 0xC4001C0F, 0x00000100 },
     { 0xBF800000, 0xBF800000, 0xBF800000, 0xBF800000, 0xC400180F, 0x05040302 }
    }
};

static const uint32_t ps_tex_sh_registers_gfx9[][2] = {
    {0x2C0A, 0x000C0081},//{ mmSPI_SHADER_PGM_RSRC1_PS, 0x000C0081 },
    {0x2C0B, 0x00000018}, //{ mmSPI_SHADER_PGM_RSRC2_PS, 0x00000018 }
};

static const uint32_t ps_tex_context_reg_gfx9[][2] = {
    {0xA1B4, 0x00000002}, //{ mmSPI_PS_INPUT_ADDR,       0x00000002 },
    {0xA1B6, 0x00000001}, //{ mmSPI_PS_IN_CONTROL,       0x00000001 },
    {0xA08F, 0x0000000F}, //{ mmCB_SHADER_MASK,          0x0000000F },
    {0xA203, 0x00000010}, //{ mmDB_SHADER_CONTROL,       0x00000010 },
    {0xA1C4, 0x00000000}, //{ mmSPI_SHADER_Z_FORMAT,     0x00000000 },
    {0xA1B8, 0x00000000}, //{ mmSPI_BARYC_CNTL,          0x00000000 /* Always 0 for now */},
    {0xA1C5, 0x00000004}, //{ mmSPI_SHADER_COL_FORMAT,   0x00000004  }
};

static const uint32_t vs_RectPosTexFast_shader_gfx9[] = {
    0x7E000B00, 0x020000F3, 0xD042000A, 0x00010100,
    0x7E020202, 0x7E040200, 0x020000F3, 0x7E060206,
    0x7E080204, 0xD1000001, 0x002A0302, 0x7C840080,
    0x7E000200, 0x7E040203, 0x7E0A0201, 0xD1000003,
    0x002A0704, 0x7E0C0207, 0x7E0E0205, 0x00000101,
    0x00020505, 0x7E040208, 0x7E0A02F2, 0x00060903,
    0x00080D07, 0x7E0C0209, 0xC40008CF, 0x05020100,
    0xC400020F, 0x05060403, 0xBF810000
};

static const uint32_t cached_cmd_gfx9[] = {
	0xc0016900, 0x0, 0x0, 0xc0026900, 0x3, 0x2a, 0x0,
	0xc0046900, 0xa, 0x0, 0x0, 0x0, 0x200020,
	0xc0016900, 0x83, 0xffff, 0xc0026900, 0x8e, 0xf, 0xf,
	0xc0056900, 0x105, 0x0, 0x0,  0x0, 0x0, 0x1a,
	0xc0026900, 0x10b, 0x0, 0x0, 0xc0016900, 0x1e0, 0x0,
	0xc0036900, 0x200, 0x0, 0x10000, 0xcc0011,
	0xc0026900, 0x292, 0x20, 0x60201b8,
	0xc0026900, 0x2b0, 0x0, 0x0, 0xc0016900, 0x2f8, 0x0
};

int amdgpu_bo_alloc_and_map_raw(amdgpu_device_handle dev, unsigned size,
			unsigned alignment, unsigned heap, uint64_t alloc_flags,
			uint64_t mapping_flags, amdgpu_bo_handle *bo, void **cpu,
			uint64_t *mc_address,
			amdgpu_va_handle *va_handle)
{
	struct amdgpu_bo_alloc_request request = {};
	amdgpu_bo_handle buf_handle;
	amdgpu_va_handle handle;
	uint64_t vmc_addr;
	int r;

	request.alloc_size = size;
	request.phys_alignment = alignment;
	request.preferred_heap = heap;
	request.flags = alloc_flags;

	r = amdgpu_bo_alloc(dev, &request, &buf_handle);
	if (r)
		return r;

	r = amdgpu_va_range_alloc(dev,
				  amdgpu_gpu_va_range_general,
				  size, alignment, 0, &vmc_addr,
				  &handle, 0);
	if (r)
		goto error_va_alloc;

	r = amdgpu_bo_va_op_raw(dev, buf_handle, 0,  ALIGN(size, getpagesize()), vmc_addr,
				   AMDGPU_VM_PAGE_READABLE |
				   AMDGPU_VM_PAGE_WRITEABLE |
				   AMDGPU_VM_PAGE_EXECUTABLE |
				   mapping_flags,
				   AMDGPU_VA_OP_MAP);
	if (r)
		goto error_va_map;

	r = amdgpu_bo_cpu_map(buf_handle, cpu);
	if (r)
		goto error_cpu_map;

	*bo = buf_handle;
	*mc_address = vmc_addr;
	*va_handle = handle;

	return 0;

 error_cpu_map:
	amdgpu_bo_cpu_unmap(buf_handle);

 error_va_map:
	amdgpu_bo_va_op(buf_handle, 0, size, vmc_addr, 0, AMDGPU_VA_OP_UNMAP);

 error_va_alloc:
	amdgpu_bo_free(buf_handle);
	return r;
}



int suite_basic_tests_init(void)
{
	struct amdgpu_gpu_info gpu_info = {0};
	int r;

	r = amdgpu_device_initialize(drm_amdgpu[0], &major_version,
				   &minor_version, &device_handle);

	if (r) {
		if ((r == -EACCES) && (errno == EACCES))
			printf("\n\nError:%s. "
				"Hint:Try to run this test program as root.",
				strerror(errno));
		return CUE_SINIT_FAILED;
	}

	r = amdgpu_query_gpu_info(device_handle, &gpu_info);
	if (r)
		return CUE_SINIT_FAILED;

	family_id = gpu_info.family_id;

	return CUE_SUCCESS;
}

int suite_basic_tests_clean(void)
{
	int r = amdgpu_device_deinitialize(device_handle);

	if (r == 0)
		return CUE_SUCCESS;
	else
		return CUE_SCLEAN_FAILED;
}

static void amdgpu_query_info_test(void)
{
	struct amdgpu_gpu_info gpu_info = {0};
	uint32_t version, feature;
	int r;

	r = amdgpu_query_gpu_info(device_handle, &gpu_info);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_query_firmware_version(device_handle, AMDGPU_INFO_FW_VCE, 0,
					  0, &version, &feature);
	CU_ASSERT_EQUAL(r, 0);
}

static void amdgpu_command_submission_gfx_separate_ibs(void)
{
	amdgpu_context_handle context_handle;
	amdgpu_bo_handle ib_result_handle, ib_result_ce_handle;
	void *ib_result_cpu, *ib_result_ce_cpu;
	uint64_t ib_result_mc_address, ib_result_ce_mc_address;
	struct amdgpu_cs_request ibs_request = {0};
	struct amdgpu_cs_ib_info ib_info[2];
	struct amdgpu_cs_fence fence_status = {0};
	uint32_t *ptr;
	uint32_t expired;
	amdgpu_bo_list_handle bo_list;
	amdgpu_va_handle va_handle, va_handle_ce;
	int r, i = 0;

	r = amdgpu_cs_ctx_create(device_handle, &context_handle);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_alloc_and_map(device_handle, 4096, 4096,
				    AMDGPU_GEM_DOMAIN_GTT, 0,
				    &ib_result_handle, &ib_result_cpu,
				    &ib_result_mc_address, &va_handle);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_alloc_and_map(device_handle, 4096, 4096,
				    AMDGPU_GEM_DOMAIN_GTT, 0,
				    &ib_result_ce_handle, &ib_result_ce_cpu,
				    &ib_result_ce_mc_address, &va_handle_ce);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_get_bo_list(device_handle, ib_result_handle,
			       ib_result_ce_handle, &bo_list);
	CU_ASSERT_EQUAL(r, 0);

	memset(ib_info, 0, 2 * sizeof(struct amdgpu_cs_ib_info));

	/* IT_SET_CE_DE_COUNTERS */
	ptr = ib_result_ce_cpu;
	if (family_id != AMDGPU_FAMILY_SI) {
		ptr[i++] = 0xc0008900;
		ptr[i++] = 0;
	}
	ptr[i++] = 0xc0008400;
	ptr[i++] = 1;
	ib_info[0].ib_mc_address = ib_result_ce_mc_address;
	ib_info[0].size = i;
	ib_info[0].flags = AMDGPU_IB_FLAG_CE;

	/* IT_WAIT_ON_CE_COUNTER */
	ptr = ib_result_cpu;
	ptr[0] = 0xc0008600;
	ptr[1] = 0x00000001;
	ib_info[1].ib_mc_address = ib_result_mc_address;
	ib_info[1].size = 2;

	ibs_request.ip_type = AMDGPU_HW_IP_GFX;
	ibs_request.number_of_ibs = 2;
	ibs_request.ibs = ib_info;
	ibs_request.resources = bo_list;
	ibs_request.fence_info.handle = NULL;

	r = amdgpu_cs_submit(context_handle, 0,&ibs_request, 1);

	CU_ASSERT_EQUAL(r, 0);

	fence_status.context = context_handle;
	fence_status.ip_type = AMDGPU_HW_IP_GFX;
	fence_status.ip_instance = 0;
	fence_status.fence = ibs_request.seq_no;

	r = amdgpu_cs_query_fence_status(&fence_status,
					 AMDGPU_TIMEOUT_INFINITE,
					 0, &expired);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_unmap_and_free(ib_result_handle, va_handle,
				     ib_result_mc_address, 4096);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_unmap_and_free(ib_result_ce_handle, va_handle_ce,
				     ib_result_ce_mc_address, 4096);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_list_destroy(bo_list);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_cs_ctx_free(context_handle);
	CU_ASSERT_EQUAL(r, 0);

}

static void amdgpu_command_submission_gfx_shared_ib(void)
{
	amdgpu_context_handle context_handle;
	amdgpu_bo_handle ib_result_handle;
	void *ib_result_cpu;
	uint64_t ib_result_mc_address;
	struct amdgpu_cs_request ibs_request = {0};
	struct amdgpu_cs_ib_info ib_info[2];
	struct amdgpu_cs_fence fence_status = {0};
	uint32_t *ptr;
	uint32_t expired;
	amdgpu_bo_list_handle bo_list;
	amdgpu_va_handle va_handle;
	int r, i = 0;

	r = amdgpu_cs_ctx_create(device_handle, &context_handle);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_alloc_and_map(device_handle, 4096, 4096,
				    AMDGPU_GEM_DOMAIN_GTT, 0,
				    &ib_result_handle, &ib_result_cpu,
				    &ib_result_mc_address, &va_handle);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_get_bo_list(device_handle, ib_result_handle, NULL,
			       &bo_list);
	CU_ASSERT_EQUAL(r, 0);

	memset(ib_info, 0, 2 * sizeof(struct amdgpu_cs_ib_info));

	/* IT_SET_CE_DE_COUNTERS */
	ptr = ib_result_cpu;
	if (family_id != AMDGPU_FAMILY_SI) {
		ptr[i++] = 0xc0008900;
		ptr[i++] = 0;
	}
	ptr[i++] = 0xc0008400;
	ptr[i++] = 1;
	ib_info[0].ib_mc_address = ib_result_mc_address;
	ib_info[0].size = i;
	ib_info[0].flags = AMDGPU_IB_FLAG_CE;

	ptr = (uint32_t *)ib_result_cpu + 4;
	ptr[0] = 0xc0008600;
	ptr[1] = 0x00000001;
	ib_info[1].ib_mc_address = ib_result_mc_address + 16;
	ib_info[1].size = 2;

	ibs_request.ip_type = AMDGPU_HW_IP_GFX;
	ibs_request.number_of_ibs = 2;
	ibs_request.ibs = ib_info;
	ibs_request.resources = bo_list;
	ibs_request.fence_info.handle = NULL;

	r = amdgpu_cs_submit(context_handle, 0, &ibs_request, 1);

	CU_ASSERT_EQUAL(r, 0);

	fence_status.context = context_handle;
	fence_status.ip_type = AMDGPU_HW_IP_GFX;
	fence_status.ip_instance = 0;
	fence_status.fence = ibs_request.seq_no;

	r = amdgpu_cs_query_fence_status(&fence_status,
					 AMDGPU_TIMEOUT_INFINITE,
					 0, &expired);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_unmap_and_free(ib_result_handle, va_handle,
				     ib_result_mc_address, 4096);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_list_destroy(bo_list);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_cs_ctx_free(context_handle);
	CU_ASSERT_EQUAL(r, 0);
}

static void amdgpu_command_submission_gfx_cp_write_data(void)
{
	amdgpu_command_submission_write_linear_helper(AMDGPU_HW_IP_GFX);
}

static void amdgpu_command_submission_gfx_cp_const_fill(void)
{
	amdgpu_command_submission_const_fill_helper(AMDGPU_HW_IP_GFX);
}

static void amdgpu_command_submission_gfx_cp_copy_data(void)
{
	amdgpu_command_submission_copy_linear_helper(AMDGPU_HW_IP_GFX);
}

static void amdgpu_bo_eviction_test(void)
{
	const int sdma_write_length = 1024;
	const int pm4_dw = 256;
	amdgpu_context_handle context_handle;
	amdgpu_bo_handle bo1, bo2, vram_max[2], gtt_max[2];
	amdgpu_bo_handle *resources;
	uint32_t *pm4;
	struct amdgpu_cs_ib_info *ib_info;
	struct amdgpu_cs_request *ibs_request;
	uint64_t bo1_mc, bo2_mc;
	volatile unsigned char *bo1_cpu, *bo2_cpu;
	int i, j, r, loop1, loop2;
	uint64_t gtt_flags[2] = {0, AMDGPU_GEM_CREATE_CPU_GTT_USWC};
	amdgpu_va_handle bo1_va_handle, bo2_va_handle;
	struct amdgpu_heap_info vram_info, gtt_info;

	pm4 = calloc(pm4_dw, sizeof(*pm4));
	CU_ASSERT_NOT_EQUAL(pm4, NULL);

	ib_info = calloc(1, sizeof(*ib_info));
	CU_ASSERT_NOT_EQUAL(ib_info, NULL);

	ibs_request = calloc(1, sizeof(*ibs_request));
	CU_ASSERT_NOT_EQUAL(ibs_request, NULL);

	r = amdgpu_cs_ctx_create(device_handle, &context_handle);
	CU_ASSERT_EQUAL(r, 0);

	/* prepare resource */
	resources = calloc(4, sizeof(amdgpu_bo_handle));
	CU_ASSERT_NOT_EQUAL(resources, NULL);

	r = amdgpu_query_heap_info(device_handle, AMDGPU_GEM_DOMAIN_VRAM,
				   0, &vram_info);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_alloc_wrap(device_handle, vram_info.max_allocation, 4096,
				 AMDGPU_GEM_DOMAIN_VRAM, 0, &vram_max[0]);
	CU_ASSERT_EQUAL(r, 0);
	r = amdgpu_bo_alloc_wrap(device_handle, vram_info.max_allocation, 4096,
				 AMDGPU_GEM_DOMAIN_VRAM, 0, &vram_max[1]);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_query_heap_info(device_handle, AMDGPU_GEM_DOMAIN_GTT,
				   0, &gtt_info);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_alloc_wrap(device_handle, gtt_info.max_allocation, 4096,
				 AMDGPU_GEM_DOMAIN_GTT, 0, &gtt_max[0]);
	CU_ASSERT_EQUAL(r, 0);
	r = amdgpu_bo_alloc_wrap(device_handle, gtt_info.max_allocation, 4096,
				 AMDGPU_GEM_DOMAIN_GTT, 0, &gtt_max[1]);
	CU_ASSERT_EQUAL(r, 0);



	loop1 = loop2 = 0;
	/* run 9 circle to test all mapping combination */
	while(loop1 < 2) {
		while(loop2 < 2) {
			/* allocate UC bo1for sDMA use */
			r = amdgpu_bo_alloc_and_map(device_handle,
						    sdma_write_length, 4096,
						    AMDGPU_GEM_DOMAIN_GTT,
						    gtt_flags[loop1], &bo1,
						    (void**)&bo1_cpu, &bo1_mc,
						    &bo1_va_handle);
			CU_ASSERT_EQUAL(r, 0);

			/* set bo1 */
			memset((void*)bo1_cpu, 0xaa, sdma_write_length);

			/* allocate UC bo2 for sDMA use */
			r = amdgpu_bo_alloc_and_map(device_handle,
						    sdma_write_length, 4096,
						    AMDGPU_GEM_DOMAIN_GTT,
						    gtt_flags[loop2], &bo2,
						    (void**)&bo2_cpu, &bo2_mc,
						    &bo2_va_handle);
			CU_ASSERT_EQUAL(r, 0);

			/* clear bo2 */
			memset((void*)bo2_cpu, 0, sdma_write_length);

			resources[0] = bo1;
			resources[1] = bo2;
			resources[2] = vram_max[loop2];
			resources[3] = gtt_max[loop2];

			/* fulfill PM4: test DMA copy linear */
			i = j = 0;
			if (family_id == AMDGPU_FAMILY_SI) {
				pm4[i++] = SDMA_PACKET_SI(SDMA_OPCODE_COPY_SI, 0, 0, 0,
							  sdma_write_length);
				pm4[i++] = 0xffffffff & bo2_mc;
				pm4[i++] = 0xffffffff & bo1_mc;
				pm4[i++] = (0xffffffff00000000 & bo2_mc) >> 32;
				pm4[i++] = (0xffffffff00000000 & bo1_mc) >> 32;
			} else {
				pm4[i++] = SDMA_PACKET(SDMA_OPCODE_COPY, SDMA_COPY_SUB_OPCODE_LINEAR, 0);
				if (family_id >= AMDGPU_FAMILY_AI)
					pm4[i++] = sdma_write_length - 1;
				else
					pm4[i++] = sdma_write_length;
				pm4[i++] = 0;
				pm4[i++] = 0xffffffff & bo1_mc;
				pm4[i++] = (0xffffffff00000000 & bo1_mc) >> 32;
				pm4[i++] = 0xffffffff & bo2_mc;
				pm4[i++] = (0xffffffff00000000 & bo2_mc) >> 32;
			}

			amdgpu_test_exec_cs_helper(context_handle,
						   AMDGPU_HW_IP_DMA, 0,
						   i, pm4,
						   4, resources,
						   ib_info, ibs_request);

			/* verify if SDMA test result meets with expected */
			i = 0;
			while(i < sdma_write_length) {
				CU_ASSERT_EQUAL(bo2_cpu[i++], 0xaa);
			}
			r = amdgpu_bo_unmap_and_free(bo1, bo1_va_handle, bo1_mc,
						     sdma_write_length);
			CU_ASSERT_EQUAL(r, 0);
			r = amdgpu_bo_unmap_and_free(bo2, bo2_va_handle, bo2_mc,
						     sdma_write_length);
			CU_ASSERT_EQUAL(r, 0);
			loop2++;
		}
		loop2 = 0;
		loop1++;
	}
	amdgpu_bo_free(vram_max[0]);
	amdgpu_bo_free(vram_max[1]);
	amdgpu_bo_free(gtt_max[0]);
	amdgpu_bo_free(gtt_max[1]);
	/* clean resources */
	free(resources);
	free(ibs_request);
	free(ib_info);
	free(pm4);

	/* end of test */
	r = amdgpu_cs_ctx_free(context_handle);
	CU_ASSERT_EQUAL(r, 0);
}


static void amdgpu_command_submission_gfx(void)
{
	/* write data using the CP */
	amdgpu_command_submission_gfx_cp_write_data();
	/* const fill using the CP */
	amdgpu_command_submission_gfx_cp_const_fill();
	/* copy data using the CP */
	amdgpu_command_submission_gfx_cp_copy_data();
	/* separate IB buffers for multi-IB submission */
	amdgpu_command_submission_gfx_separate_ibs();
	/* shared IB buffer for multi-IB submission */
	amdgpu_command_submission_gfx_shared_ib();
}

static void amdgpu_semaphore_test(void)
{
	amdgpu_context_handle context_handle[2];
	amdgpu_semaphore_handle sem;
	amdgpu_bo_handle ib_result_handle[2];
	void *ib_result_cpu[2];
	uint64_t ib_result_mc_address[2];
	struct amdgpu_cs_request ibs_request[2] = {0};
	struct amdgpu_cs_ib_info ib_info[2] = {0};
	struct amdgpu_cs_fence fence_status = {0};
	uint32_t *ptr;
	uint32_t expired;
	uint32_t sdma_nop, gfx_nop;
	amdgpu_bo_list_handle bo_list[2];
	amdgpu_va_handle va_handle[2];
	int r, i;

	if (family_id == AMDGPU_FAMILY_SI) {
		sdma_nop = SDMA_PACKET_SI(SDMA_NOP_SI, 0, 0, 0, 0);
		gfx_nop = GFX_COMPUTE_NOP_SI;
	} else {
		sdma_nop = SDMA_PKT_HEADER_OP(SDMA_NOP);
		gfx_nop = GFX_COMPUTE_NOP;
	}

	r = amdgpu_cs_create_semaphore(&sem);
	CU_ASSERT_EQUAL(r, 0);
	for (i = 0; i < 2; i++) {
		r = amdgpu_cs_ctx_create(device_handle, &context_handle[i]);
		CU_ASSERT_EQUAL(r, 0);

		r = amdgpu_bo_alloc_and_map(device_handle, 4096, 4096,
					    AMDGPU_GEM_DOMAIN_GTT, 0,
					    &ib_result_handle[i], &ib_result_cpu[i],
					    &ib_result_mc_address[i], &va_handle[i]);
		CU_ASSERT_EQUAL(r, 0);

		r = amdgpu_get_bo_list(device_handle, ib_result_handle[i],
				       NULL, &bo_list[i]);
		CU_ASSERT_EQUAL(r, 0);
	}

	/* 1. same context different engine */
	ptr = ib_result_cpu[0];
	ptr[0] = sdma_nop;
	ib_info[0].ib_mc_address = ib_result_mc_address[0];
	ib_info[0].size = 1;

	ibs_request[0].ip_type = AMDGPU_HW_IP_DMA;
	ibs_request[0].number_of_ibs = 1;
	ibs_request[0].ibs = &ib_info[0];
	ibs_request[0].resources = bo_list[0];
	ibs_request[0].fence_info.handle = NULL;
	r = amdgpu_cs_submit(context_handle[0], 0,&ibs_request[0], 1);
	CU_ASSERT_EQUAL(r, 0);
	r = amdgpu_cs_signal_semaphore(context_handle[0], AMDGPU_HW_IP_DMA, 0, 0, sem);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_cs_wait_semaphore(context_handle[0], AMDGPU_HW_IP_GFX, 0, 0, sem);
	CU_ASSERT_EQUAL(r, 0);
	ptr = ib_result_cpu[1];
	ptr[0] = gfx_nop;
	ib_info[1].ib_mc_address = ib_result_mc_address[1];
	ib_info[1].size = 1;

	ibs_request[1].ip_type = AMDGPU_HW_IP_GFX;
	ibs_request[1].number_of_ibs = 1;
	ibs_request[1].ibs = &ib_info[1];
	ibs_request[1].resources = bo_list[1];
	ibs_request[1].fence_info.handle = NULL;

	r = amdgpu_cs_submit(context_handle[0], 0,&ibs_request[1], 1);
	CU_ASSERT_EQUAL(r, 0);

	fence_status.context = context_handle[0];
	fence_status.ip_type = AMDGPU_HW_IP_GFX;
	fence_status.ip_instance = 0;
	fence_status.fence = ibs_request[1].seq_no;
	r = amdgpu_cs_query_fence_status(&fence_status,
					 500000000, 0, &expired);
	CU_ASSERT_EQUAL(r, 0);
	CU_ASSERT_EQUAL(expired, true);

	/* 2. same engine different context */
	ptr = ib_result_cpu[0];
	ptr[0] = gfx_nop;
	ib_info[0].ib_mc_address = ib_result_mc_address[0];
	ib_info[0].size = 1;

	ibs_request[0].ip_type = AMDGPU_HW_IP_GFX;
	ibs_request[0].number_of_ibs = 1;
	ibs_request[0].ibs = &ib_info[0];
	ibs_request[0].resources = bo_list[0];
	ibs_request[0].fence_info.handle = NULL;
	r = amdgpu_cs_submit(context_handle[0], 0,&ibs_request[0], 1);
	CU_ASSERT_EQUAL(r, 0);
	r = amdgpu_cs_signal_semaphore(context_handle[0], AMDGPU_HW_IP_GFX, 0, 0, sem);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_cs_wait_semaphore(context_handle[1], AMDGPU_HW_IP_GFX, 0, 0, sem);
	CU_ASSERT_EQUAL(r, 0);
	ptr = ib_result_cpu[1];
	ptr[0] = gfx_nop;
	ib_info[1].ib_mc_address = ib_result_mc_address[1];
	ib_info[1].size = 1;

	ibs_request[1].ip_type = AMDGPU_HW_IP_GFX;
	ibs_request[1].number_of_ibs = 1;
	ibs_request[1].ibs = &ib_info[1];
	ibs_request[1].resources = bo_list[1];
	ibs_request[1].fence_info.handle = NULL;
	r = amdgpu_cs_submit(context_handle[1], 0,&ibs_request[1], 1);

	CU_ASSERT_EQUAL(r, 0);

	fence_status.context = context_handle[1];
	fence_status.ip_type = AMDGPU_HW_IP_GFX;
	fence_status.ip_instance = 0;
	fence_status.fence = ibs_request[1].seq_no;
	r = amdgpu_cs_query_fence_status(&fence_status,
					 500000000, 0, &expired);
	CU_ASSERT_EQUAL(r, 0);
	CU_ASSERT_EQUAL(expired, true);

	for (i = 0; i < 2; i++) {
		r = amdgpu_bo_unmap_and_free(ib_result_handle[i], va_handle[i],
					     ib_result_mc_address[i], 4096);
		CU_ASSERT_EQUAL(r, 0);

		r = amdgpu_bo_list_destroy(bo_list[i]);
		CU_ASSERT_EQUAL(r, 0);

		r = amdgpu_cs_ctx_free(context_handle[i]);
		CU_ASSERT_EQUAL(r, 0);
	}

	r = amdgpu_cs_destroy_semaphore(sem);
	CU_ASSERT_EQUAL(r, 0);
}

static void amdgpu_command_submission_compute_nop(void)
{
	amdgpu_context_handle context_handle;
	amdgpu_bo_handle ib_result_handle;
	void *ib_result_cpu;
	uint64_t ib_result_mc_address;
	struct amdgpu_cs_request ibs_request;
	struct amdgpu_cs_ib_info ib_info;
	struct amdgpu_cs_fence fence_status;
	uint32_t *ptr;
	uint32_t expired;
	int r, instance;
	amdgpu_bo_list_handle bo_list;
	amdgpu_va_handle va_handle;
	struct drm_amdgpu_info_hw_ip info;

	r = amdgpu_query_hw_ip_info(device_handle, AMDGPU_HW_IP_COMPUTE, 0, &info);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_cs_ctx_create(device_handle, &context_handle);
	CU_ASSERT_EQUAL(r, 0);

	for (instance = 0; (1 << instance) & info.available_rings; instance++) {
		r = amdgpu_bo_alloc_and_map(device_handle, 4096, 4096,
					    AMDGPU_GEM_DOMAIN_GTT, 0,
					    &ib_result_handle, &ib_result_cpu,
					    &ib_result_mc_address, &va_handle);
		CU_ASSERT_EQUAL(r, 0);

		r = amdgpu_get_bo_list(device_handle, ib_result_handle, NULL,
				       &bo_list);
		CU_ASSERT_EQUAL(r, 0);

		ptr = ib_result_cpu;
		memset(ptr, 0, 16);
		ptr[0]=PACKET3(PACKET3_NOP, 14);

		memset(&ib_info, 0, sizeof(struct amdgpu_cs_ib_info));
		ib_info.ib_mc_address = ib_result_mc_address;
		ib_info.size = 16;

		memset(&ibs_request, 0, sizeof(struct amdgpu_cs_request));
		ibs_request.ip_type = AMDGPU_HW_IP_COMPUTE;
		ibs_request.ring = instance;
		ibs_request.number_of_ibs = 1;
		ibs_request.ibs = &ib_info;
		ibs_request.resources = bo_list;
		ibs_request.fence_info.handle = NULL;

		memset(&fence_status, 0, sizeof(struct amdgpu_cs_fence));
		r = amdgpu_cs_submit(context_handle, 0,&ibs_request, 1);
		CU_ASSERT_EQUAL(r, 0);

		fence_status.context = context_handle;
		fence_status.ip_type = AMDGPU_HW_IP_COMPUTE;
		fence_status.ip_instance = 0;
		fence_status.ring = instance;
		fence_status.fence = ibs_request.seq_no;

		r = amdgpu_cs_query_fence_status(&fence_status,
						 AMDGPU_TIMEOUT_INFINITE,
						 0, &expired);
		CU_ASSERT_EQUAL(r, 0);

		r = amdgpu_bo_list_destroy(bo_list);
		CU_ASSERT_EQUAL(r, 0);

		r = amdgpu_bo_unmap_and_free(ib_result_handle, va_handle,
					     ib_result_mc_address, 4096);
		CU_ASSERT_EQUAL(r, 0);
	}

	r = amdgpu_cs_ctx_free(context_handle);
	CU_ASSERT_EQUAL(r, 0);
}

static void amdgpu_command_submission_compute_cp_write_data(void)
{
	amdgpu_command_submission_write_linear_helper(AMDGPU_HW_IP_COMPUTE);
}

static void amdgpu_command_submission_compute_cp_const_fill(void)
{
	amdgpu_command_submission_const_fill_helper(AMDGPU_HW_IP_COMPUTE);
}

static void amdgpu_command_submission_compute_cp_copy_data(void)
{
	amdgpu_command_submission_copy_linear_helper(AMDGPU_HW_IP_COMPUTE);
}

static void amdgpu_command_submission_compute(void)
{
	/* write data using the CP */
	amdgpu_command_submission_compute_cp_write_data();
	/* const fill using the CP */
	amdgpu_command_submission_compute_cp_const_fill();
	/* copy data using the CP */
	amdgpu_command_submission_compute_cp_copy_data();
	/* nop test */
	amdgpu_command_submission_compute_nop();
}

/*
 * caller need create/release:
 * pm4_src, resources, ib_info, and ibs_request
 * submit command stream described in ibs_request and wait for this IB accomplished
 */
static void amdgpu_test_exec_cs_helper(amdgpu_context_handle context_handle,
				       unsigned ip_type,
				       int instance, int pm4_dw, uint32_t *pm4_src,
				       int res_cnt, amdgpu_bo_handle *resources,
				       struct amdgpu_cs_ib_info *ib_info,
				       struct amdgpu_cs_request *ibs_request)
{
	int r;
	uint32_t expired;
	uint32_t *ring_ptr;
	amdgpu_bo_handle ib_result_handle;
	void *ib_result_cpu;
	uint64_t ib_result_mc_address;
	struct amdgpu_cs_fence fence_status = {0};
	amdgpu_bo_handle *all_res = alloca(sizeof(resources[0]) * (res_cnt + 1));
	amdgpu_va_handle va_handle;

	/* prepare CS */
	CU_ASSERT_NOT_EQUAL(pm4_src, NULL);
	CU_ASSERT_NOT_EQUAL(resources, NULL);
	CU_ASSERT_NOT_EQUAL(ib_info, NULL);
	CU_ASSERT_NOT_EQUAL(ibs_request, NULL);
	CU_ASSERT_TRUE(pm4_dw <= 1024);

	/* allocate IB */
	r = amdgpu_bo_alloc_and_map(device_handle, 4096, 4096,
				    AMDGPU_GEM_DOMAIN_GTT, 0,
				    &ib_result_handle, &ib_result_cpu,
				    &ib_result_mc_address, &va_handle);
	CU_ASSERT_EQUAL(r, 0);

	/* copy PM4 packet to ring from caller */
	ring_ptr = ib_result_cpu;
	memcpy(ring_ptr, pm4_src, pm4_dw * sizeof(*pm4_src));

	ib_info->ib_mc_address = ib_result_mc_address;
	ib_info->size = pm4_dw;

	ibs_request->ip_type = ip_type;
	ibs_request->ring = instance;
	ibs_request->number_of_ibs = 1;
	ibs_request->ibs = ib_info;
	ibs_request->fence_info.handle = NULL;

	memcpy(all_res, resources, sizeof(resources[0]) * res_cnt);
	all_res[res_cnt] = ib_result_handle;

	r = amdgpu_bo_list_create(device_handle, res_cnt+1, all_res,
				  NULL, &ibs_request->resources);
	CU_ASSERT_EQUAL(r, 0);

	CU_ASSERT_NOT_EQUAL(ibs_request, NULL);

	/* submit CS */
	r = amdgpu_cs_submit(context_handle, 0, ibs_request, 1);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_list_destroy(ibs_request->resources);
	CU_ASSERT_EQUAL(r, 0);

	fence_status.ip_type = ip_type;
	fence_status.ip_instance = 0;
	fence_status.ring = ibs_request->ring;
	fence_status.context = context_handle;
	fence_status.fence = ibs_request->seq_no;

	/* wait for IB accomplished */
	r = amdgpu_cs_query_fence_status(&fence_status,
					 AMDGPU_TIMEOUT_INFINITE,
					 0, &expired);
	CU_ASSERT_EQUAL(r, 0);
	CU_ASSERT_EQUAL(expired, true);

	r = amdgpu_bo_unmap_and_free(ib_result_handle, va_handle,
				     ib_result_mc_address, 4096);
	CU_ASSERT_EQUAL(r, 0);
}

static void amdgpu_command_submission_write_linear_helper(unsigned ip_type)
{
	const int sdma_write_length = 128;
	const int pm4_dw = 256;
	amdgpu_context_handle context_handle;
	amdgpu_bo_handle bo;
	amdgpu_bo_handle *resources;
	uint32_t *pm4;
	struct amdgpu_cs_ib_info *ib_info;
	struct amdgpu_cs_request *ibs_request;
	uint64_t bo_mc;
	volatile uint32_t *bo_cpu;
	int i, j, r, loop, ring_id;
	uint64_t gtt_flags[2] = {0, AMDGPU_GEM_CREATE_CPU_GTT_USWC};
	amdgpu_va_handle va_handle;
	struct drm_amdgpu_info_hw_ip hw_ip_info;

	pm4 = calloc(pm4_dw, sizeof(*pm4));
	CU_ASSERT_NOT_EQUAL(pm4, NULL);

	ib_info = calloc(1, sizeof(*ib_info));
	CU_ASSERT_NOT_EQUAL(ib_info, NULL);

	ibs_request = calloc(1, sizeof(*ibs_request));
	CU_ASSERT_NOT_EQUAL(ibs_request, NULL);

	r = amdgpu_query_hw_ip_info(device_handle, ip_type, 0, &hw_ip_info);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_cs_ctx_create(device_handle, &context_handle);
	CU_ASSERT_EQUAL(r, 0);

	/* prepare resource */
	resources = calloc(1, sizeof(amdgpu_bo_handle));
	CU_ASSERT_NOT_EQUAL(resources, NULL);

	for (ring_id = 0; (1 << ring_id) & hw_ip_info.available_rings; ring_id++) {
		loop = 0;
		while(loop < 2) {
			/* allocate UC bo for sDMA use */
			r = amdgpu_bo_alloc_and_map(device_handle,
						    sdma_write_length * sizeof(uint32_t),
						    4096, AMDGPU_GEM_DOMAIN_GTT,
						    gtt_flags[loop], &bo, (void**)&bo_cpu,
						    &bo_mc, &va_handle);
			CU_ASSERT_EQUAL(r, 0);

			/* clear bo */
			memset((void*)bo_cpu, 0, sdma_write_length * sizeof(uint32_t));

			resources[0] = bo;

			/* fulfill PM4: test DMA write-linear */
			i = j = 0;
			if (ip_type == AMDGPU_HW_IP_DMA) {
				if (family_id == AMDGPU_FAMILY_SI)
					pm4[i++] = SDMA_PACKET_SI(SDMA_OPCODE_WRITE, 0, 0, 0,
								  sdma_write_length);
				else
					pm4[i++] = SDMA_PACKET(SDMA_OPCODE_WRITE,
							       SDMA_WRITE_SUB_OPCODE_LINEAR, 0);
				pm4[i++] = 0xffffffff & bo_mc;
				pm4[i++] = (0xffffffff00000000 & bo_mc) >> 32;
				if (family_id >= AMDGPU_FAMILY_AI)
					pm4[i++] = sdma_write_length - 1;
				else if (family_id != AMDGPU_FAMILY_SI)
					pm4[i++] = sdma_write_length;
				while(j++ < sdma_write_length)
					pm4[i++] = 0xdeadbeaf;
			} else if ((ip_type == AMDGPU_HW_IP_GFX) ||
				    (ip_type == AMDGPU_HW_IP_COMPUTE)) {
				pm4[i++] = PACKET3(PACKET3_WRITE_DATA, 2 + sdma_write_length);
				pm4[i++] = WRITE_DATA_DST_SEL(5) | WR_CONFIRM;
				pm4[i++] = 0xfffffffc & bo_mc;
				pm4[i++] = (0xffffffff00000000 & bo_mc) >> 32;
				while(j++ < sdma_write_length)
					pm4[i++] = 0xdeadbeaf;
			}

			amdgpu_test_exec_cs_helper(context_handle,
						   ip_type, ring_id,
						   i, pm4,
						   1, resources,
						   ib_info, ibs_request);

			/* verify if SDMA test result meets with expected */
			i = 0;
			while(i < sdma_write_length) {
				CU_ASSERT_EQUAL(bo_cpu[i++], 0xdeadbeaf);
			}

			r = amdgpu_bo_unmap_and_free(bo, va_handle, bo_mc,
						     sdma_write_length * sizeof(uint32_t));
			CU_ASSERT_EQUAL(r, 0);
			loop++;
		}
	}
	/* clean resources */
	free(resources);
	free(ibs_request);
	free(ib_info);
	free(pm4);

	/* end of test */
	r = amdgpu_cs_ctx_free(context_handle);
	CU_ASSERT_EQUAL(r, 0);
}

static void amdgpu_command_submission_sdma_write_linear(void)
{
	amdgpu_command_submission_write_linear_helper(AMDGPU_HW_IP_DMA);
}

static void amdgpu_command_submission_const_fill_helper(unsigned ip_type)
{
	const int sdma_write_length = 1024 * 1024;
	const int pm4_dw = 256;
	amdgpu_context_handle context_handle;
	amdgpu_bo_handle bo;
	amdgpu_bo_handle *resources;
	uint32_t *pm4;
	struct amdgpu_cs_ib_info *ib_info;
	struct amdgpu_cs_request *ibs_request;
	uint64_t bo_mc;
	volatile uint32_t *bo_cpu;
	int i, j, r, loop, ring_id;
	uint64_t gtt_flags[2] = {0, AMDGPU_GEM_CREATE_CPU_GTT_USWC};
	amdgpu_va_handle va_handle;
	struct drm_amdgpu_info_hw_ip hw_ip_info;

	pm4 = calloc(pm4_dw, sizeof(*pm4));
	CU_ASSERT_NOT_EQUAL(pm4, NULL);

	ib_info = calloc(1, sizeof(*ib_info));
	CU_ASSERT_NOT_EQUAL(ib_info, NULL);

	ibs_request = calloc(1, sizeof(*ibs_request));
	CU_ASSERT_NOT_EQUAL(ibs_request, NULL);

	r = amdgpu_query_hw_ip_info(device_handle, ip_type, 0, &hw_ip_info);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_cs_ctx_create(device_handle, &context_handle);
	CU_ASSERT_EQUAL(r, 0);

	/* prepare resource */
	resources = calloc(1, sizeof(amdgpu_bo_handle));
	CU_ASSERT_NOT_EQUAL(resources, NULL);

	for (ring_id = 0; (1 << ring_id) & hw_ip_info.available_rings; ring_id++) {
		loop = 0;
		while(loop < 2) {
			/* allocate UC bo for sDMA use */
			r = amdgpu_bo_alloc_and_map(device_handle,
						    sdma_write_length, 4096,
						    AMDGPU_GEM_DOMAIN_GTT,
						    gtt_flags[loop], &bo, (void**)&bo_cpu,
						    &bo_mc, &va_handle);
			CU_ASSERT_EQUAL(r, 0);

			/* clear bo */
			memset((void*)bo_cpu, 0, sdma_write_length);

			resources[0] = bo;

			/* fulfill PM4: test DMA const fill */
			i = j = 0;
			if (ip_type == AMDGPU_HW_IP_DMA) {
				if (family_id == AMDGPU_FAMILY_SI) {
					pm4[i++] = SDMA_PACKET_SI(SDMA_OPCODE_CONSTANT_FILL_SI,
								  0, 0, 0,
								  sdma_write_length / 4);
					pm4[i++] = 0xfffffffc & bo_mc;
					pm4[i++] = 0xdeadbeaf;
					pm4[i++] = (0xffffffff00000000 & bo_mc) >> 16;
				} else {
					pm4[i++] = SDMA_PACKET(SDMA_OPCODE_CONSTANT_FILL, 0,
							       SDMA_CONSTANT_FILL_EXTRA_SIZE(2));
					pm4[i++] = 0xffffffff & bo_mc;
					pm4[i++] = (0xffffffff00000000 & bo_mc) >> 32;
					pm4[i++] = 0xdeadbeaf;
					if (family_id >= AMDGPU_FAMILY_AI)
						pm4[i++] = sdma_write_length - 1;
					else
						pm4[i++] = sdma_write_length;
				}
			} else if ((ip_type == AMDGPU_HW_IP_GFX) ||
				   (ip_type == AMDGPU_HW_IP_COMPUTE)) {
				if (family_id == AMDGPU_FAMILY_SI) {
					pm4[i++] = PACKET3(PACKET3_DMA_DATA_SI, 4);
					pm4[i++] = 0xdeadbeaf;
					pm4[i++] = PACKET3_DMA_DATA_SI_ENGINE(0) |
						   PACKET3_DMA_DATA_SI_DST_SEL(0) |
						   PACKET3_DMA_DATA_SI_SRC_SEL(2) |
						   PACKET3_DMA_DATA_SI_CP_SYNC;
					pm4[i++] = 0xffffffff & bo_mc;
					pm4[i++] = (0xffffffff00000000 & bo_mc) >> 32;
					pm4[i++] = sdma_write_length;
				} else {
					pm4[i++] = PACKET3(PACKET3_DMA_DATA, 5);
					pm4[i++] = PACKET3_DMA_DATA_ENGINE(0) |
						   PACKET3_DMA_DATA_DST_SEL(0) |
						   PACKET3_DMA_DATA_SRC_SEL(2) |
						   PACKET3_DMA_DATA_CP_SYNC;
					pm4[i++] = 0xdeadbeaf;
					pm4[i++] = 0;
					pm4[i++] = 0xfffffffc & bo_mc;
					pm4[i++] = (0xffffffff00000000 & bo_mc) >> 32;
					pm4[i++] = sdma_write_length;
				}
			}

			amdgpu_test_exec_cs_helper(context_handle,
						   ip_type, ring_id,
						   i, pm4,
						   1, resources,
						   ib_info, ibs_request);

			/* verify if SDMA test result meets with expected */
			i = 0;
			while(i < (sdma_write_length / 4)) {
				CU_ASSERT_EQUAL(bo_cpu[i++], 0xdeadbeaf);
			}

			r = amdgpu_bo_unmap_and_free(bo, va_handle, bo_mc,
						     sdma_write_length);
			CU_ASSERT_EQUAL(r, 0);
			loop++;
		}
	}
	/* clean resources */
	free(resources);
	free(ibs_request);
	free(ib_info);
	free(pm4);

	/* end of test */
	r = amdgpu_cs_ctx_free(context_handle);
	CU_ASSERT_EQUAL(r, 0);
}

static void amdgpu_command_submission_sdma_const_fill(void)
{
	amdgpu_command_submission_const_fill_helper(AMDGPU_HW_IP_DMA);
}

static void amdgpu_command_submission_copy_linear_helper(unsigned ip_type)
{
	const int sdma_write_length = 1024;
	const int pm4_dw = 256;
	amdgpu_context_handle context_handle;
	amdgpu_bo_handle bo1, bo2;
	amdgpu_bo_handle *resources;
	uint32_t *pm4;
	struct amdgpu_cs_ib_info *ib_info;
	struct amdgpu_cs_request *ibs_request;
	uint64_t bo1_mc, bo2_mc;
	volatile unsigned char *bo1_cpu, *bo2_cpu;
	int i, j, r, loop1, loop2, ring_id;
	uint64_t gtt_flags[2] = {0, AMDGPU_GEM_CREATE_CPU_GTT_USWC};
	amdgpu_va_handle bo1_va_handle, bo2_va_handle;
	struct drm_amdgpu_info_hw_ip hw_ip_info;

	pm4 = calloc(pm4_dw, sizeof(*pm4));
	CU_ASSERT_NOT_EQUAL(pm4, NULL);

	ib_info = calloc(1, sizeof(*ib_info));
	CU_ASSERT_NOT_EQUAL(ib_info, NULL);

	ibs_request = calloc(1, sizeof(*ibs_request));
	CU_ASSERT_NOT_EQUAL(ibs_request, NULL);

	r = amdgpu_query_hw_ip_info(device_handle, ip_type, 0, &hw_ip_info);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_cs_ctx_create(device_handle, &context_handle);
	CU_ASSERT_EQUAL(r, 0);

	/* prepare resource */
	resources = calloc(2, sizeof(amdgpu_bo_handle));
	CU_ASSERT_NOT_EQUAL(resources, NULL);

	for (ring_id = 0; (1 << ring_id) & hw_ip_info.available_rings; ring_id++) {
		loop1 = loop2 = 0;
		/* run 9 circle to test all mapping combination */
		while(loop1 < 2) {
			while(loop2 < 2) {
				/* allocate UC bo1for sDMA use */
				r = amdgpu_bo_alloc_and_map(device_handle,
							    sdma_write_length, 4096,
							    AMDGPU_GEM_DOMAIN_GTT,
							    gtt_flags[loop1], &bo1,
							    (void**)&bo1_cpu, &bo1_mc,
							    &bo1_va_handle);
				CU_ASSERT_EQUAL(r, 0);

				/* set bo1 */
				memset((void*)bo1_cpu, 0xaa, sdma_write_length);

				/* allocate UC bo2 for sDMA use */
				r = amdgpu_bo_alloc_and_map(device_handle,
							    sdma_write_length, 4096,
							    AMDGPU_GEM_DOMAIN_GTT,
							    gtt_flags[loop2], &bo2,
							    (void**)&bo2_cpu, &bo2_mc,
							    &bo2_va_handle);
				CU_ASSERT_EQUAL(r, 0);

				/* clear bo2 */
				memset((void*)bo2_cpu, 0, sdma_write_length);

				resources[0] = bo1;
				resources[1] = bo2;

				/* fulfill PM4: test DMA copy linear */
				i = j = 0;
				if (ip_type == AMDGPU_HW_IP_DMA) {
					if (family_id == AMDGPU_FAMILY_SI) {
						pm4[i++] = SDMA_PACKET_SI(SDMA_OPCODE_COPY_SI,
									  0, 0, 0,
									  sdma_write_length);
						pm4[i++] = 0xffffffff & bo2_mc;
						pm4[i++] = 0xffffffff & bo1_mc;
						pm4[i++] = (0xffffffff00000000 & bo2_mc) >> 32;
						pm4[i++] = (0xffffffff00000000 & bo1_mc) >> 32;
					} else {
						pm4[i++] = SDMA_PACKET(SDMA_OPCODE_COPY,
								       SDMA_COPY_SUB_OPCODE_LINEAR,
								       0);
						if (family_id >= AMDGPU_FAMILY_AI)
							pm4[i++] = sdma_write_length - 1;
						else
							pm4[i++] = sdma_write_length;
						pm4[i++] = 0;
						pm4[i++] = 0xffffffff & bo1_mc;
						pm4[i++] = (0xffffffff00000000 & bo1_mc) >> 32;
						pm4[i++] = 0xffffffff & bo2_mc;
						pm4[i++] = (0xffffffff00000000 & bo2_mc) >> 32;
					}
				} else if ((ip_type == AMDGPU_HW_IP_GFX) ||
					   (ip_type == AMDGPU_HW_IP_COMPUTE)) {
					if (family_id == AMDGPU_FAMILY_SI) {
						pm4[i++] = PACKET3(PACKET3_DMA_DATA_SI, 4);
						pm4[i++] = 0xfffffffc & bo1_mc;
						pm4[i++] = PACKET3_DMA_DATA_SI_ENGINE(0) |
							   PACKET3_DMA_DATA_SI_DST_SEL(0) |
							   PACKET3_DMA_DATA_SI_SRC_SEL(0) |
							   PACKET3_DMA_DATA_SI_CP_SYNC |
							   (0xffff00000000 & bo1_mc) >> 32;
						pm4[i++] = 0xfffffffc & bo2_mc;
						pm4[i++] = (0xffffffff00000000 & bo2_mc) >> 32;
						pm4[i++] = sdma_write_length;
					} else {
						pm4[i++] = PACKET3(PACKET3_DMA_DATA, 5);
						pm4[i++] = PACKET3_DMA_DATA_ENGINE(0) |
							   PACKET3_DMA_DATA_DST_SEL(0) |
							   PACKET3_DMA_DATA_SRC_SEL(0) |
							   PACKET3_DMA_DATA_CP_SYNC;
						pm4[i++] = 0xfffffffc & bo1_mc;
						pm4[i++] = (0xffffffff00000000 & bo1_mc) >> 32;
						pm4[i++] = 0xfffffffc & bo2_mc;
						pm4[i++] = (0xffffffff00000000 & bo2_mc) >> 32;
						pm4[i++] = sdma_write_length;
					}
				}

				amdgpu_test_exec_cs_helper(context_handle,
							   ip_type, ring_id,
							   i, pm4,
							   2, resources,
							   ib_info, ibs_request);

				/* verify if SDMA test result meets with expected */
				i = 0;
				while(i < sdma_write_length) {
					CU_ASSERT_EQUAL(bo2_cpu[i++], 0xaa);
				}
				r = amdgpu_bo_unmap_and_free(bo1, bo1_va_handle, bo1_mc,
							     sdma_write_length);
				CU_ASSERT_EQUAL(r, 0);
				r = amdgpu_bo_unmap_and_free(bo2, bo2_va_handle, bo2_mc,
							     sdma_write_length);
				CU_ASSERT_EQUAL(r, 0);
				loop2++;
			}
			loop1++;
		}
	}
	/* clean resources */
	free(resources);
	free(ibs_request);
	free(ib_info);
	free(pm4);

	/* end of test */
	r = amdgpu_cs_ctx_free(context_handle);
	CU_ASSERT_EQUAL(r, 0);
}

static void amdgpu_command_submission_sdma_copy_linear(void)
{
	amdgpu_command_submission_copy_linear_helper(AMDGPU_HW_IP_DMA);
}

static void amdgpu_command_submission_sdma(void)
{
	amdgpu_command_submission_sdma_write_linear();
	amdgpu_command_submission_sdma_const_fill();
	amdgpu_command_submission_sdma_copy_linear();
}

static void amdgpu_command_submission_multi_fence_wait_all(bool wait_all)
{
	amdgpu_context_handle context_handle;
	amdgpu_bo_handle ib_result_handle, ib_result_ce_handle;
	void *ib_result_cpu, *ib_result_ce_cpu;
	uint64_t ib_result_mc_address, ib_result_ce_mc_address;
	struct amdgpu_cs_request ibs_request[2] = {0};
	struct amdgpu_cs_ib_info ib_info[2];
	struct amdgpu_cs_fence fence_status[2] = {0};
	uint32_t *ptr;
	uint32_t expired;
	amdgpu_bo_list_handle bo_list;
	amdgpu_va_handle va_handle, va_handle_ce;
	int r;
	int i = 0, ib_cs_num = 2;

	r = amdgpu_cs_ctx_create(device_handle, &context_handle);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_alloc_and_map(device_handle, 4096, 4096,
				    AMDGPU_GEM_DOMAIN_GTT, 0,
				    &ib_result_handle, &ib_result_cpu,
				    &ib_result_mc_address, &va_handle);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_alloc_and_map(device_handle, 4096, 4096,
				    AMDGPU_GEM_DOMAIN_GTT, 0,
				    &ib_result_ce_handle, &ib_result_ce_cpu,
				    &ib_result_ce_mc_address, &va_handle_ce);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_get_bo_list(device_handle, ib_result_handle,
			       ib_result_ce_handle, &bo_list);
	CU_ASSERT_EQUAL(r, 0);

	memset(ib_info, 0, 2 * sizeof(struct amdgpu_cs_ib_info));

	/* IT_SET_CE_DE_COUNTERS */
	ptr = ib_result_ce_cpu;
	if (family_id != AMDGPU_FAMILY_SI) {
		ptr[i++] = 0xc0008900;
		ptr[i++] = 0;
	}
	ptr[i++] = 0xc0008400;
	ptr[i++] = 1;
	ib_info[0].ib_mc_address = ib_result_ce_mc_address;
	ib_info[0].size = i;
	ib_info[0].flags = AMDGPU_IB_FLAG_CE;

	/* IT_WAIT_ON_CE_COUNTER */
	ptr = ib_result_cpu;
	ptr[0] = 0xc0008600;
	ptr[1] = 0x00000001;
	ib_info[1].ib_mc_address = ib_result_mc_address;
	ib_info[1].size = 2;

	for (i = 0; i < ib_cs_num; i++) {
		ibs_request[i].ip_type = AMDGPU_HW_IP_GFX;
		ibs_request[i].number_of_ibs = 2;
		ibs_request[i].ibs = ib_info;
		ibs_request[i].resources = bo_list;
		ibs_request[i].fence_info.handle = NULL;
	}

	r = amdgpu_cs_submit(context_handle, 0,ibs_request, ib_cs_num);

	CU_ASSERT_EQUAL(r, 0);

	for (i = 0; i < ib_cs_num; i++) {
		fence_status[i].context = context_handle;
		fence_status[i].ip_type = AMDGPU_HW_IP_GFX;
		fence_status[i].fence = ibs_request[i].seq_no;
	}

	r = amdgpu_cs_wait_fences(fence_status, ib_cs_num, wait_all,
				AMDGPU_TIMEOUT_INFINITE,
				&expired, NULL);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_unmap_and_free(ib_result_handle, va_handle,
				     ib_result_mc_address, 4096);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_unmap_and_free(ib_result_ce_handle, va_handle_ce,
				     ib_result_ce_mc_address, 4096);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_list_destroy(bo_list);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_cs_ctx_free(context_handle);
	CU_ASSERT_EQUAL(r, 0);
}

static void amdgpu_command_submission_multi_fence(void)
{
	amdgpu_command_submission_multi_fence_wait_all(true);
	amdgpu_command_submission_multi_fence_wait_all(false);
}

static void amdgpu_userptr_test(void)
{
	int i, r, j;
	uint32_t *pm4 = NULL;
	uint64_t bo_mc;
	void *ptr = NULL;
	int pm4_dw = 256;
	int sdma_write_length = 4;
	amdgpu_bo_handle handle;
	amdgpu_context_handle context_handle;
	struct amdgpu_cs_ib_info *ib_info;
	struct amdgpu_cs_request *ibs_request;
	amdgpu_bo_handle buf_handle;
	amdgpu_va_handle va_handle;

	pm4 = calloc(pm4_dw, sizeof(*pm4));
	CU_ASSERT_NOT_EQUAL(pm4, NULL);

	ib_info = calloc(1, sizeof(*ib_info));
	CU_ASSERT_NOT_EQUAL(ib_info, NULL);

	ibs_request = calloc(1, sizeof(*ibs_request));
	CU_ASSERT_NOT_EQUAL(ibs_request, NULL);

	r = amdgpu_cs_ctx_create(device_handle, &context_handle);
	CU_ASSERT_EQUAL(r, 0);

	posix_memalign(&ptr, sysconf(_SC_PAGE_SIZE), BUFFER_SIZE);
	CU_ASSERT_NOT_EQUAL(ptr, NULL);
	memset(ptr, 0, BUFFER_SIZE);

	r = amdgpu_create_bo_from_user_mem(device_handle,
					   ptr, BUFFER_SIZE, &buf_handle);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_va_range_alloc(device_handle,
				  amdgpu_gpu_va_range_general,
				  BUFFER_SIZE, 1, 0, &bo_mc,
				  &va_handle, 0);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_va_op(buf_handle, 0, BUFFER_SIZE, bo_mc, 0, AMDGPU_VA_OP_MAP);
	CU_ASSERT_EQUAL(r, 0);

	handle = buf_handle;

	j = i = 0;

	if (family_id == AMDGPU_FAMILY_SI)
		pm4[i++] = SDMA_PACKET_SI(SDMA_OPCODE_WRITE, 0, 0, 0,
				sdma_write_length);
	else
		pm4[i++] = SDMA_PACKET(SDMA_OPCODE_WRITE,
				SDMA_WRITE_SUB_OPCODE_LINEAR, 0);
	pm4[i++] = 0xffffffff & bo_mc;
	pm4[i++] = (0xffffffff00000000 & bo_mc) >> 32;
	if (family_id >= AMDGPU_FAMILY_AI)
		pm4[i++] = sdma_write_length - 1;
	else if (family_id != AMDGPU_FAMILY_SI)
		pm4[i++] = sdma_write_length;

	while (j++ < sdma_write_length)
		pm4[i++] = 0xdeadbeaf;

	if (!fork()) {
		pm4[0] = 0x0;
		exit(0);
	}

	amdgpu_test_exec_cs_helper(context_handle,
				   AMDGPU_HW_IP_DMA, 0,
				   i, pm4,
				   1, &handle,
				   ib_info, ibs_request);
	i = 0;
	while (i < sdma_write_length) {
		CU_ASSERT_EQUAL(((int*)ptr)[i++], 0xdeadbeaf);
	}
	free(ibs_request);
	free(ib_info);
	free(pm4);

	r = amdgpu_bo_va_op(buf_handle, 0, BUFFER_SIZE, bo_mc, 0, AMDGPU_VA_OP_UNMAP);
	CU_ASSERT_EQUAL(r, 0);
	r = amdgpu_va_range_free(va_handle);
	CU_ASSERT_EQUAL(r, 0);
	r = amdgpu_bo_free(buf_handle);
	CU_ASSERT_EQUAL(r, 0);
	free(ptr);

	r = amdgpu_cs_ctx_free(context_handle);
	CU_ASSERT_EQUAL(r, 0);

	wait(NULL);
}

static void amdgpu_sync_dependency_test(void)
{
	amdgpu_context_handle context_handle[2];
	amdgpu_bo_handle ib_result_handle;
	void *ib_result_cpu;
	uint64_t ib_result_mc_address;
	struct amdgpu_cs_request ibs_request;
	struct amdgpu_cs_ib_info ib_info;
	struct amdgpu_cs_fence fence_status;
	uint32_t expired;
	int i, j, r;
	amdgpu_bo_list_handle bo_list;
	amdgpu_va_handle va_handle;
	static uint32_t *ptr;
	uint64_t seq_no;

	r = amdgpu_cs_ctx_create(device_handle, &context_handle[0]);
	CU_ASSERT_EQUAL(r, 0);
	r = amdgpu_cs_ctx_create(device_handle, &context_handle[1]);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_alloc_and_map(device_handle, 8192, 4096,
			AMDGPU_GEM_DOMAIN_GTT, 0,
						    &ib_result_handle, &ib_result_cpu,
						    &ib_result_mc_address, &va_handle);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_get_bo_list(device_handle, ib_result_handle, NULL,
			       &bo_list);
	CU_ASSERT_EQUAL(r, 0);

	ptr = ib_result_cpu;
	i = 0;

	memcpy(ptr + CODE_OFFSET , shader_bin, sizeof(shader_bin));

	/* Dispatch minimal init config and verify it's executed */
	ptr[i++] = PACKET3(PKT3_CONTEXT_CONTROL, 1);
	ptr[i++] = 0x80000000;
	ptr[i++] = 0x80000000;

	ptr[i++] = PACKET3(PKT3_CLEAR_STATE, 0);
	ptr[i++] = 0x80000000;


	/* Program compute regs */
	ptr[i++] = PACKET3(PKT3_SET_SH_REG, 2);
	ptr[i++] = mmCOMPUTE_PGM_LO - PACKET3_SET_SH_REG_START;
	ptr[i++] = (ib_result_mc_address + CODE_OFFSET * 4) >> 8;
	ptr[i++] = (ib_result_mc_address + CODE_OFFSET * 4) >> 40;


	ptr[i++] = PACKET3(PKT3_SET_SH_REG, 2);
	ptr[i++] = mmCOMPUTE_PGM_RSRC1 - PACKET3_SET_SH_REG_START;
	/*
	 * 002c0040         COMPUTE_PGM_RSRC1 <- VGPRS = 0
	                                      SGPRS = 1
	                                      PRIORITY = 0
	                                      FLOAT_MODE = 192 (0xc0)
	                                      PRIV = 0
	                                      DX10_CLAMP = 1
	                                      DEBUG_MODE = 0
	                                      IEEE_MODE = 0
	                                      BULKY = 0
	                                      CDBG_USER = 0
	 *
	 */
	ptr[i++] = 0x002c0040;


	/*
	 * 00000010         COMPUTE_PGM_RSRC2 <- SCRATCH_EN = 0
	                                      USER_SGPR = 8
	                                      TRAP_PRESENT = 0
	                                      TGID_X_EN = 0
	                                      TGID_Y_EN = 0
	                                      TGID_Z_EN = 0
	                                      TG_SIZE_EN = 0
	                                      TIDIG_COMP_CNT = 0
	                                      EXCP_EN_MSB = 0
	                                      LDS_SIZE = 0
	                                      EXCP_EN = 0
	 *
	 */
	ptr[i++] = 0x00000010;


/*
 * 00000100         COMPUTE_TMPRING_SIZE <- WAVES = 256 (0x100)
                                         WAVESIZE = 0
 *
 */
	ptr[i++] = PACKET3(PKT3_SET_SH_REG, 1);
	ptr[i++] = mmCOMPUTE_TMPRING_SIZE - PACKET3_SET_SH_REG_START;
	ptr[i++] = 0x00000100;

	ptr[i++] = PACKET3(PKT3_SET_SH_REG, 2);
	ptr[i++] = mmCOMPUTE_USER_DATA_0 - PACKET3_SET_SH_REG_START;
	ptr[i++] = 0xffffffff & (ib_result_mc_address + DATA_OFFSET * 4);
	ptr[i++] = (0xffffffff00000000 & (ib_result_mc_address + DATA_OFFSET * 4)) >> 32;

	ptr[i++] = PACKET3(PKT3_SET_SH_REG, 1);
	ptr[i++] = mmCOMPUTE_RESOURCE_LIMITS - PACKET3_SET_SH_REG_START;
	ptr[i++] = 0;

	ptr[i++] = PACKET3(PKT3_SET_SH_REG, 3);
	ptr[i++] = mmCOMPUTE_NUM_THREAD_X - PACKET3_SET_SH_REG_START;
	ptr[i++] = 1;
	ptr[i++] = 1;
	ptr[i++] = 1;


	/* Dispatch */
	ptr[i++] = PACKET3(PACKET3_DISPATCH_DIRECT, 3);
	ptr[i++] = 1;
	ptr[i++] = 1;
	ptr[i++] = 1;
	ptr[i++] = 0x00000045; /* DISPATCH DIRECT field */


	while (i & 7)
		ptr[i++] =  0xffff1000; /* type3 nop packet */

	memset(&ib_info, 0, sizeof(struct amdgpu_cs_ib_info));
	ib_info.ib_mc_address = ib_result_mc_address;
	ib_info.size = i;

	memset(&ibs_request, 0, sizeof(struct amdgpu_cs_request));
	ibs_request.ip_type = AMDGPU_HW_IP_GFX;
	ibs_request.ring = 0;
	ibs_request.number_of_ibs = 1;
	ibs_request.ibs = &ib_info;
	ibs_request.resources = bo_list;
	ibs_request.fence_info.handle = NULL;

	r = amdgpu_cs_submit(context_handle[1], 0,&ibs_request, 1);
	CU_ASSERT_EQUAL(r, 0);
	seq_no = ibs_request.seq_no;



	/* Prepare second command with dependency on the first */
	j = i;
	ptr[i++] = PACKET3(PACKET3_WRITE_DATA, 3);
	ptr[i++] = WRITE_DATA_DST_SEL(5) | WR_CONFIRM;
	ptr[i++] =          0xfffffffc & (ib_result_mc_address + DATA_OFFSET * 4);
	ptr[i++] = (0xffffffff00000000 & (ib_result_mc_address + DATA_OFFSET * 4)) >> 32;
	ptr[i++] = 99;

	while (i & 7)
		ptr[i++] =  0xffff1000; /* type3 nop packet */

	memset(&ib_info, 0, sizeof(struct amdgpu_cs_ib_info));
	ib_info.ib_mc_address = ib_result_mc_address + j * 4;
	ib_info.size = i - j;

	memset(&ibs_request, 0, sizeof(struct amdgpu_cs_request));
	ibs_request.ip_type = AMDGPU_HW_IP_GFX;
	ibs_request.ring = 0;
	ibs_request.number_of_ibs = 1;
	ibs_request.ibs = &ib_info;
	ibs_request.resources = bo_list;
	ibs_request.fence_info.handle = NULL;

	ibs_request.number_of_dependencies = 1;

	ibs_request.dependencies = calloc(1, sizeof(*ibs_request.dependencies));
	ibs_request.dependencies[0].context = context_handle[1];
	ibs_request.dependencies[0].ip_instance = 0;
	ibs_request.dependencies[0].ring = 0;
	ibs_request.dependencies[0].fence = seq_no;


	r = amdgpu_cs_submit(context_handle[0], 0,&ibs_request, 1);
	CU_ASSERT_EQUAL(r, 0);


	memset(&fence_status, 0, sizeof(struct amdgpu_cs_fence));
	fence_status.context = context_handle[0];
	fence_status.ip_type = AMDGPU_HW_IP_GFX;
	fence_status.ip_instance = 0;
	fence_status.ring = 0;
	fence_status.fence = ibs_request.seq_no;

	r = amdgpu_cs_query_fence_status(&fence_status,
		       AMDGPU_TIMEOUT_INFINITE,0, &expired);
	CU_ASSERT_EQUAL(r, 0);

	/* Expect the second command to wait for shader to complete */
	CU_ASSERT_EQUAL(ptr[DATA_OFFSET], 99);

	r = amdgpu_bo_list_destroy(bo_list);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_unmap_and_free(ib_result_handle, va_handle,
				     ib_result_mc_address, 4096);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_cs_ctx_free(context_handle[0]);
	CU_ASSERT_EQUAL(r, 0);
	r = amdgpu_cs_ctx_free(context_handle[1]);
	CU_ASSERT_EQUAL(r, 0);

	free(ibs_request.dependencies);
}

static int amdgpu_dispatch_load_cs_shader(uint8_t *ptr,
					   int cs_type)
{
	uint32_t shader_size;
	const uint32_t *shader;

	switch (cs_type) {
		case CS_BUFFERCLEAR:
			shader = bufferclear_cs_shader_gfx9;
			shader_size = sizeof(bufferclear_cs_shader_gfx9);
			break;
		case CS_BUFFERCOPY:
			shader = buffercopy_cs_shader_gfx9;
			shader_size = sizeof(buffercopy_cs_shader_gfx9);
			break;
		default:
			return -1;
			break;
	}

	memcpy(ptr, shader, shader_size);
	return 0;
}

static int amdgpu_dispatch_init(uint32_t *ptr, uint32_t ip_type)
{
	int i = 0;

	/* Write context control and load shadowing register if necessary */
	if (ip_type == AMDGPU_HW_IP_GFX) {
		ptr[i++] = PACKET3(PKT3_CONTEXT_CONTROL, 1);
		ptr[i++] = 0x80000000;
		ptr[i++] = 0x80000000;
	}

	/* Issue commands to set default compute state. */
	/* clear mmCOMPUTE_START_Z - mmCOMPUTE_START_X */
	ptr[i++] = PACKET3_COMPUTE(PKT3_SET_SH_REG, 3);
	ptr[i++] = 0x204;
	i += 3;

	/* clear mmCOMPUTE_TMPRING_SIZE */
	ptr[i++] = PACKET3_COMPUTE(PKT3_SET_SH_REG, 1);
	ptr[i++] = 0x218;
	ptr[i++] = 0;

	return i;
}

static int amdgpu_dispatch_write_cumask(uint32_t *ptr)
{
	int i = 0;

	/*  Issue commands to set cu mask used in current dispatch */
	/* set mmCOMPUTE_STATIC_THREAD_MGMT_SE1 - mmCOMPUTE_STATIC_THREAD_MGMT_SE0 */
	ptr[i++] = PACKET3_COMPUTE(PKT3_SET_SH_REG, 2);
	ptr[i++] = 0x216;
	ptr[i++] = 0xffffffff;
	ptr[i++] = 0xffffffff;
	/* set mmCOMPUTE_STATIC_THREAD_MGMT_SE3 - mmCOMPUTE_STATIC_THREAD_MGMT_SE2 */
	ptr[i++] = PACKET3_COMPUTE(PKT3_SET_SH_REG, 2);
	ptr[i++] = 0x219;
	ptr[i++] = 0xffffffff;
	ptr[i++] = 0xffffffff;

	return i;
}

static int amdgpu_dispatch_write2hw(uint32_t *ptr, uint64_t shader_addr)
{
	int i, j;

	i = 0;

	/* Writes shader state to HW */
	/* set mmCOMPUTE_PGM_HI - mmCOMPUTE_PGM_LO */
	ptr[i++] = PACKET3_COMPUTE(PKT3_SET_SH_REG, 2);
	ptr[i++] = 0x20c;
	ptr[i++] = (shader_addr >> 8);
	ptr[i++] = (shader_addr >> 40);
	/* write sh regs*/
	for (j = 0; j < bufferclear_cs_shader_registers_num_gfx9; j++) {
		ptr[i++] = PACKET3_COMPUTE(PKT3_SET_SH_REG, 1);
		/* - Gfx9ShRegBase */
		ptr[i++] = bufferclear_cs_shader_registers_gfx9[j][0] - 0x2c00;
		ptr[i++] = bufferclear_cs_shader_registers_gfx9[j][1];
	}

	return i;
}

static void amdgpu_memset_dispatch_test(amdgpu_device_handle device_handle,
					 uint32_t ip_type,
					 uint32_t ring)
{
	amdgpu_context_handle context_handle;
	amdgpu_bo_handle bo_dst, bo_shader, bo_cmd, resources[3];
	volatile unsigned char *ptr_dst;
	void *ptr_shader;
	uint32_t *ptr_cmd;
	uint64_t mc_address_dst, mc_address_shader, mc_address_cmd;
	amdgpu_va_handle va_dst, va_shader, va_cmd;
	int i, r;
	int bo_dst_size = 16384;
	int bo_shader_size = 4096;
	int bo_cmd_size = 4096;
	struct amdgpu_cs_request ibs_request = {0};
	struct amdgpu_cs_ib_info ib_info= {0};
	amdgpu_bo_list_handle bo_list;
	struct amdgpu_cs_fence fence_status = {0};
	uint32_t expired;

	r = amdgpu_cs_ctx_create(device_handle, &context_handle);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_alloc_and_map(device_handle, bo_cmd_size, 4096,
					AMDGPU_GEM_DOMAIN_GTT, 0,
					&bo_cmd, (void **)&ptr_cmd,
					&mc_address_cmd, &va_cmd);
	CU_ASSERT_EQUAL(r, 0);
	memset(ptr_cmd, 0, bo_cmd_size);

	r = amdgpu_bo_alloc_and_map(device_handle, bo_shader_size, 4096,
					AMDGPU_GEM_DOMAIN_VRAM, 0,
					&bo_shader, &ptr_shader,
					&mc_address_shader, &va_shader);
	CU_ASSERT_EQUAL(r, 0);
	memset(ptr_shader, 0, bo_shader_size);

	r = amdgpu_dispatch_load_cs_shader(ptr_shader, CS_BUFFERCLEAR);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_alloc_and_map(device_handle, bo_dst_size, 4096,
					AMDGPU_GEM_DOMAIN_VRAM, 0,
					&bo_dst, (void **)&ptr_dst,
					&mc_address_dst, &va_dst);
	CU_ASSERT_EQUAL(r, 0);

	i = 0;
	i += amdgpu_dispatch_init(ptr_cmd + i, ip_type);

	/*  Issue commands to set cu mask used in current dispatch */
	i += amdgpu_dispatch_write_cumask(ptr_cmd + i);

	/* Writes shader state to HW */
	i += amdgpu_dispatch_write2hw(ptr_cmd + i, mc_address_shader);

	/* Write constant data */
	/* Writes the UAV constant data to the SGPRs. */
	ptr_cmd[i++] = PACKET3_COMPUTE(PKT3_SET_SH_REG, 4);
	ptr_cmd[i++] = 0x240;
	ptr_cmd[i++] = mc_address_dst;
	ptr_cmd[i++] = (mc_address_dst >> 32) | 0x100000;
	ptr_cmd[i++] = 0x400;
	ptr_cmd[i++] = 0x74fac;

	/* Sets a range of pixel shader constants */
	ptr_cmd[i++] = PACKET3_COMPUTE(PKT3_SET_SH_REG, 4);
	ptr_cmd[i++] = 0x244;
	ptr_cmd[i++] = 0x22222222;
	ptr_cmd[i++] = 0x22222222;
	ptr_cmd[i++] = 0x22222222;
	ptr_cmd[i++] = 0x22222222;

	/* clear mmCOMPUTE_RESOURCE_LIMITS */
	ptr_cmd[i++] = PACKET3_COMPUTE(PKT3_SET_SH_REG, 1);
	ptr_cmd[i++] = 0x215;
	ptr_cmd[i++] = 0;

	/* dispatch direct command */
	ptr_cmd[i++] = PACKET3_COMPUTE(PACKET3_DISPATCH_DIRECT, 3);
	ptr_cmd[i++] = 0x10;
	ptr_cmd[i++] = 1;
	ptr_cmd[i++] = 1;
	ptr_cmd[i++] = 1;

	while (i & 7)
		ptr_cmd[i++] = 0xffff1000; /* type3 nop packet */

	resources[0] = bo_dst;
	resources[1] = bo_shader;
	resources[2] = bo_cmd;
	r = amdgpu_bo_list_create(device_handle, 3, resources, NULL, &bo_list);
	CU_ASSERT_EQUAL(r, 0);

	ib_info.ib_mc_address = mc_address_cmd;
	ib_info.size = i;
	ibs_request.ip_type = ip_type;
	ibs_request.ring = ring;
	ibs_request.resources = bo_list;
	ibs_request.number_of_ibs = 1;
	ibs_request.ibs = &ib_info;
	ibs_request.fence_info.handle = NULL;

	/* submit CS */
	r = amdgpu_cs_submit(context_handle, 0, &ibs_request, 1);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_list_destroy(bo_list);
	CU_ASSERT_EQUAL(r, 0);

	fence_status.ip_type = ip_type;
	fence_status.ip_instance = 0;
	fence_status.ring = ring;
	fence_status.context = context_handle;
	fence_status.fence = ibs_request.seq_no;

	/* wait for IB accomplished */
	r = amdgpu_cs_query_fence_status(&fence_status,
					 AMDGPU_TIMEOUT_INFINITE,
					 0, &expired);
	CU_ASSERT_EQUAL(r, 0);
	CU_ASSERT_EQUAL(expired, true);

	/* verify if memset test result meets with expected */
	i = 0;
	while(i < bo_dst_size) {
		CU_ASSERT_EQUAL(ptr_dst[i++], 0x22);
	}

	r = amdgpu_bo_unmap_and_free(bo_dst, va_dst, mc_address_dst, bo_dst_size);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_unmap_and_free(bo_shader, va_shader, mc_address_shader, bo_shader_size);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_unmap_and_free(bo_cmd, va_cmd, mc_address_cmd, bo_cmd_size);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_cs_ctx_free(context_handle);
	CU_ASSERT_EQUAL(r, 0);
}

static void amdgpu_memcpy_dispatch_test(amdgpu_device_handle device_handle,
					uint32_t ip_type,
					uint32_t ring)
{
	amdgpu_context_handle context_handle;
	amdgpu_bo_handle bo_src, bo_dst, bo_shader, bo_cmd, resources[4];
	volatile unsigned char *ptr_dst;
	void *ptr_shader;
	unsigned char *ptr_src;
	uint32_t *ptr_cmd;
	uint64_t mc_address_src, mc_address_dst, mc_address_shader, mc_address_cmd;
	amdgpu_va_handle va_src, va_dst, va_shader, va_cmd;
	int i, r;
	int bo_dst_size = 16384;
	int bo_shader_size = 4096;
	int bo_cmd_size = 4096;
	struct amdgpu_cs_request ibs_request = {0};
	struct amdgpu_cs_ib_info ib_info= {0};
	uint32_t expired;
	amdgpu_bo_list_handle bo_list;
	struct amdgpu_cs_fence fence_status = {0};

	r = amdgpu_cs_ctx_create(device_handle, &context_handle);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_alloc_and_map(device_handle, bo_cmd_size, 4096,
				    AMDGPU_GEM_DOMAIN_GTT, 0,
				    &bo_cmd, (void **)&ptr_cmd,
				    &mc_address_cmd, &va_cmd);
	CU_ASSERT_EQUAL(r, 0);
	memset(ptr_cmd, 0, bo_cmd_size);

	r = amdgpu_bo_alloc_and_map(device_handle, bo_shader_size, 4096,
					AMDGPU_GEM_DOMAIN_VRAM, 0,
					&bo_shader, &ptr_shader,
					&mc_address_shader, &va_shader);
	CU_ASSERT_EQUAL(r, 0);
	memset(ptr_shader, 0, bo_shader_size);

	r = amdgpu_dispatch_load_cs_shader(ptr_shader, CS_BUFFERCOPY );
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_alloc_and_map(device_handle, bo_dst_size, 4096,
					AMDGPU_GEM_DOMAIN_VRAM, 0,
					&bo_src, (void **)&ptr_src,
					&mc_address_src, &va_src);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_alloc_and_map(device_handle, bo_dst_size, 4096,
					AMDGPU_GEM_DOMAIN_VRAM, 0,
					&bo_dst, (void **)&ptr_dst,
					&mc_address_dst, &va_dst);
	CU_ASSERT_EQUAL(r, 0);

	memset(ptr_src, 0x55, bo_dst_size);

	i = 0;
	i += amdgpu_dispatch_init(ptr_cmd + i, ip_type);

	/*  Issue commands to set cu mask used in current dispatch */
	i += amdgpu_dispatch_write_cumask(ptr_cmd + i);

	/* Writes shader state to HW */
	i += amdgpu_dispatch_write2hw(ptr_cmd + i, mc_address_shader);

	/* Write constant data */
	/* Writes the texture resource constants data to the SGPRs */
	ptr_cmd[i++] = PACKET3_COMPUTE(PKT3_SET_SH_REG, 4);
	ptr_cmd[i++] = 0x240;
	ptr_cmd[i++] = mc_address_src;
	ptr_cmd[i++] = (mc_address_src >> 32) | 0x100000;
	ptr_cmd[i++] = 0x400;
	ptr_cmd[i++] = 0x74fac;

	/* Writes the UAV constant data to the SGPRs. */
	ptr_cmd[i++] = PACKET3_COMPUTE(PKT3_SET_SH_REG, 4);
	ptr_cmd[i++] = 0x244;
	ptr_cmd[i++] = mc_address_dst;
	ptr_cmd[i++] = (mc_address_dst >> 32) | 0x100000;
	ptr_cmd[i++] = 0x400;
	ptr_cmd[i++] = 0x74fac;

	/* clear mmCOMPUTE_RESOURCE_LIMITS */
	ptr_cmd[i++] = PACKET3_COMPUTE(PKT3_SET_SH_REG, 1);
	ptr_cmd[i++] = 0x215;
	ptr_cmd[i++] = 0;

	/* dispatch direct command */
	ptr_cmd[i++] = PACKET3_COMPUTE(PACKET3_DISPATCH_DIRECT, 3);
	ptr_cmd[i++] = 0x10;
	ptr_cmd[i++] = 1;
	ptr_cmd[i++] = 1;
	ptr_cmd[i++] = 1;

	while (i & 7)
		ptr_cmd[i++] = 0xffff1000; /* type3 nop packet */

	resources[0] = bo_shader;
	resources[1] = bo_src;
	resources[2] = bo_dst;
	resources[3] = bo_cmd;
	r = amdgpu_bo_list_create(device_handle, 4, resources, NULL, &bo_list);
	CU_ASSERT_EQUAL(r, 0);

	ib_info.ib_mc_address = mc_address_cmd;
	ib_info.size = i;
	ibs_request.ip_type = ip_type;
	ibs_request.ring = ring;
	ibs_request.resources = bo_list;
	ibs_request.number_of_ibs = 1;
	ibs_request.ibs = &ib_info;
	ibs_request.fence_info.handle = NULL;
	r = amdgpu_cs_submit(context_handle, 0, &ibs_request, 1);
	CU_ASSERT_EQUAL(r, 0);

	fence_status.ip_type = ip_type;
	fence_status.ip_instance = 0;
	fence_status.ring = ring;
	fence_status.context = context_handle;
	fence_status.fence = ibs_request.seq_no;

	/* wait for IB accomplished */
	r = amdgpu_cs_query_fence_status(&fence_status,
					 AMDGPU_TIMEOUT_INFINITE,
					 0, &expired);
	CU_ASSERT_EQUAL(r, 0);
	CU_ASSERT_EQUAL(expired, true);

	/* verify if memcpy test result meets with expected */
	i = 0;
	while(i < bo_dst_size) {
		CU_ASSERT_EQUAL(ptr_dst[i], ptr_src[i]);
		i++;
	}

	r = amdgpu_bo_list_destroy(bo_list);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_unmap_and_free(bo_src, va_src, mc_address_src, bo_dst_size);
	CU_ASSERT_EQUAL(r, 0);
	r = amdgpu_bo_unmap_and_free(bo_dst, va_dst, mc_address_dst, bo_dst_size);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_unmap_and_free(bo_cmd, va_cmd, mc_address_cmd, bo_cmd_size);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_unmap_and_free(bo_shader, va_shader, mc_address_shader, bo_shader_size);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_cs_ctx_free(context_handle);
	CU_ASSERT_EQUAL(r, 0);
}

static void amdgpu_compute_dispatch_test(void)
{
	int r;
	struct drm_amdgpu_info_hw_ip info;
	uint32_t ring_id;

	r = amdgpu_query_hw_ip_info(device_handle, AMDGPU_HW_IP_COMPUTE, 0, &info);
	CU_ASSERT_EQUAL(r, 0);
	if (!info.available_rings)
		printf("SKIP ... as there's no compute ring\n");

	for (ring_id = 0; (1 << ring_id) & info.available_rings; ring_id++) {
		amdgpu_memset_dispatch_test(device_handle, AMDGPU_HW_IP_COMPUTE, ring_id);
		amdgpu_memcpy_dispatch_test(device_handle, AMDGPU_HW_IP_COMPUTE, ring_id);
	}
}

static void amdgpu_gfx_dispatch_test(void)
{
	int r;
	struct drm_amdgpu_info_hw_ip info;
	uint32_t ring_id;

	r = amdgpu_query_hw_ip_info(device_handle, AMDGPU_HW_IP_GFX, 0, &info);
	CU_ASSERT_EQUAL(r, 0);
	if (!info.available_rings)
		printf("SKIP ... as there's no graphics ring\n");

	for (ring_id = 0; (1 << ring_id) & info.available_rings; ring_id++) {
		amdgpu_memset_dispatch_test(device_handle, AMDGPU_HW_IP_GFX, ring_id);
		amdgpu_memcpy_dispatch_test(device_handle, AMDGPU_HW_IP_GFX, ring_id);
	}
}

static int amdgpu_draw_load_ps_shader(uint8_t *ptr, int ps_type)
{
	int i;
	uint32_t shader_offset= 256;
	uint32_t mem_offset, patch_code_offset;
	uint32_t shader_size, patchinfo_code_size;
	const uint32_t *shader;
	const uint32_t *patchinfo_code;
	const uint32_t *patchcode_offset;

	switch (ps_type) {
		case PS_CONST:
			shader = ps_const_shader_gfx9;
			shader_size = sizeof(ps_const_shader_gfx9);
			patchinfo_code = (const uint32_t *)ps_const_shader_patchinfo_code_gfx9;
			patchinfo_code_size = ps_const_shader_patchinfo_code_size_gfx9;
			patchcode_offset = ps_const_shader_patchinfo_offset_gfx9;
			break;
		case PS_TEX:
			shader = ps_tex_shader_gfx9;
			shader_size = sizeof(ps_tex_shader_gfx9);
			patchinfo_code = (const uint32_t *)ps_tex_shader_patchinfo_code_gfx9;
			patchinfo_code_size = ps_tex_shader_patchinfo_code_size_gfx9;
			patchcode_offset = ps_tex_shader_patchinfo_offset_gfx9;
			break;
		default:
			return -1;
			break;
	}

	/* write main shader program */
	for (i = 0 ; i < 10; i++) {
		mem_offset = i * shader_offset;
		memcpy(ptr + mem_offset, shader, shader_size);
	}

	/* overwrite patch codes */
	for (i = 0 ; i < 10; i++) {
		mem_offset = i * shader_offset + patchcode_offset[0] * sizeof(uint32_t);
		patch_code_offset = i * patchinfo_code_size;
		memcpy(ptr + mem_offset,
			patchinfo_code + patch_code_offset,
			patchinfo_code_size * sizeof(uint32_t));
	}

	return 0;
}

/* load RectPosTexFast_VS */
static int amdgpu_draw_load_vs_shader(uint8_t *ptr)
{
	const uint32_t *shader;
	uint32_t shader_size;

	shader = vs_RectPosTexFast_shader_gfx9;
	shader_size = sizeof(vs_RectPosTexFast_shader_gfx9);

	memcpy(ptr, shader, shader_size);

	return 0;
}

static int amdgpu_draw_init(uint32_t *ptr)
{
	int i = 0;
	const uint32_t *preamblecache_ptr;
	uint32_t preamblecache_size;

	/* Write context control and load shadowing register if necessary */
	ptr[i++] = PACKET3(PKT3_CONTEXT_CONTROL, 1);
	ptr[i++] = 0x80000000;
	ptr[i++] = 0x80000000;

	preamblecache_ptr = preamblecache_gfx9;
	preamblecache_size = sizeof(preamblecache_gfx9);

	memcpy(ptr + i, preamblecache_ptr, preamblecache_size);
	return i + preamblecache_size/sizeof(uint32_t);
}

static int amdgpu_draw_setup_and_write_drawblt_surf_info(uint32_t *ptr,
							 uint64_t dst_addr)
{
	int i = 0;

	/* setup color buffer */
	/* offset   reg
	   0xA318   CB_COLOR0_BASE
	   0xA319   CB_COLOR0_BASE_EXT
	   0xA31A   CB_COLOR0_ATTRIB2
	   0xA31B   CB_COLOR0_VIEW
	   0xA31C   CB_COLOR0_INFO
	   0xA31D   CB_COLOR0_ATTRIB
	   0xA31E   CB_COLOR0_DCC_CONTROL
	   0xA31F   CB_COLOR0_CMASK
	   0xA320   CB_COLOR0_CMASK_BASE_EXT
	   0xA321   CB_COLOR0_FMASK
	   0xA322   CB_COLOR0_FMASK_BASE_EXT
	   0xA323   CB_COLOR0_CLEAR_WORD0
	   0xA324   CB_COLOR0_CLEAR_WORD1
	   0xA325   CB_COLOR0_DCC_BASE
	   0xA326   CB_COLOR0_DCC_BASE_EXT */
	ptr[i++] = PACKET3(PACKET3_SET_CONTEXT_REG, 15);
	ptr[i++] = 0x318;
	ptr[i++] = dst_addr >> 8;
	ptr[i++] = dst_addr >> 40;
	ptr[i++] = 0x7c01f;
	ptr[i++] = 0;
	ptr[i++] = 0x50438;
	ptr[i++] = 0x10140000;
	i += 9;

	/* mmCB_MRT0_EPITCH */
	ptr[i++] = PACKET3(PACKET3_SET_CONTEXT_REG, 1);
	ptr[i++] = 0x1e8;
	ptr[i++] = 0x1f;

	/* 0xA32B   CB_COLOR1_BASE */
	ptr[i++] = PACKET3(PACKET3_SET_CONTEXT_REG, 1);
	ptr[i++] = 0x32b;
	ptr[i++] = 0;

	/* 0xA33A   CB_COLOR1_BASE */
	ptr[i++] = PACKET3(PACKET3_SET_CONTEXT_REG, 1);
	ptr[i++] = 0x33a;
	ptr[i++] = 0;

	/* SPI_SHADER_COL_FORMAT */
	ptr[i++] = PACKET3(PACKET3_SET_CONTEXT_REG, 1);
	ptr[i++] = 0x1c5;
	ptr[i++] = 9;

	/* Setup depth buffer */
	/* mmDB_Z_INFO */
	ptr[i++] = PACKET3(PACKET3_SET_CONTEXT_REG, 2);
	ptr[i++] = 0xe;
	i += 2;

	return i;
}

static int amdgpu_draw_setup_and_write_drawblt_state(uint32_t *ptr)
{
	int i = 0;
	const uint32_t *cached_cmd_ptr;
	uint32_t cached_cmd_size;

	/* mmPA_SC_TILE_STEERING_OVERRIDE */
	ptr[i++] = PACKET3(PACKET3_SET_CONTEXT_REG, 1);
	ptr[i++] = 0xd7;
	ptr[i++] = 0;

	ptr[i++] = 0xffff1000;
	ptr[i++] = 0xc0021000;

	ptr[i++] = PACKET3(PACKET3_SET_CONTEXT_REG, 1);
	ptr[i++] = 0xd7;
	ptr[i++] = 1;

	/* mmPA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y0_0 */
	ptr[i++] = PACKET3(PACKET3_SET_CONTEXT_REG, 16);
	ptr[i++] = 0x2fe;
	i += 16;

	/* mmPA_SC_CENTROID_PRIORITY_0 */
	ptr[i++] = PACKET3(PACKET3_SET_CONTEXT_REG, 2);
	ptr[i++] = 0x2f5;
	i += 2;

	cached_cmd_ptr = cached_cmd_gfx9;
	cached_cmd_size = sizeof(cached_cmd_gfx9);

	memcpy(ptr + i, cached_cmd_ptr, cached_cmd_size);
	i += cached_cmd_size/sizeof(uint32_t);

	return i;
}

static int amdgpu_draw_vs_RectPosTexFast_write2hw(uint32_t *ptr,
						  int ps_type,
						  uint64_t shader_addr)
{
	int i = 0;

	/* mmPA_CL_VS_OUT_CNTL */
	ptr[i++] = PACKET3(PACKET3_SET_CONTEXT_REG, 1);
	ptr[i++] = 0x207;
	ptr[i++] = 0;

	/* mmSPI_SHADER_PGM_RSRC3_VS */
	ptr[i++] = PACKET3(PKT3_SET_SH_REG, 1);
	ptr[i++] = 0x46;
	ptr[i++] = 0xffff;

	/* mmSPI_SHADER_PGM_LO_VS...mmSPI_SHADER_PGM_HI_VS */
	ptr[i++] = PACKET3(PKT3_SET_SH_REG, 2);
	ptr[i++] = 0x48;
	ptr[i++] = shader_addr >> 8;
	ptr[i++] = shader_addr >> 40;

	/* mmSPI_SHADER_PGM_RSRC1_VS */
	ptr[i++] = PACKET3(PKT3_SET_SH_REG, 1);
	ptr[i++] = 0x4a;
	ptr[i++] = 0xc0081;
	/* mmSPI_SHADER_PGM_RSRC2_VS */
	ptr[i++] = PACKET3(PKT3_SET_SH_REG, 1);
	ptr[i++] = 0x4b;
	ptr[i++] = 0x18;

	/* mmSPI_VS_OUT_CONFIG */
	ptr[i++] = PACKET3(PACKET3_SET_CONTEXT_REG, 1);
	ptr[i++] = 0x1b1;
	ptr[i++] = 2;

	/* mmSPI_SHADER_POS_FORMAT */
	ptr[i++] = PACKET3(PACKET3_SET_CONTEXT_REG, 1);
	ptr[i++] = 0x1c3;
	ptr[i++] = 4;

	ptr[i++] = PACKET3(PKT3_SET_SH_REG, 4);
	ptr[i++] = 0x4c;
	i += 2;
	ptr[i++] = 0x42000000;
	ptr[i++] = 0x42000000;

	ptr[i++] = PACKET3(PKT3_SET_SH_REG, 4);
	ptr[i++] = 0x50;
	i += 2;
	if (ps_type == PS_CONST) {
		i += 2;
	} else if (ps_type == PS_TEX) {
		ptr[i++] = 0x3f800000;
		ptr[i++] = 0x3f800000;
	}

	ptr[i++] = PACKET3(PKT3_SET_SH_REG, 4);
	ptr[i++] = 0x54;
	i += 4;

	return i;
}

static int amdgpu_draw_ps_write2hw(uint32_t *ptr,
				   int ps_type,
				   uint64_t shader_addr)
{
	int i, j;
	const uint32_t *sh_registers;
	const uint32_t *context_registers;
	uint32_t num_sh_reg, num_context_reg;

	if (ps_type == PS_CONST) {
		sh_registers = (const uint32_t *)ps_const_sh_registers_gfx9;
		context_registers = (const uint32_t *)ps_const_context_reg_gfx9;
		num_sh_reg = ps_num_sh_registers_gfx9;
		num_context_reg = ps_num_context_registers_gfx9;
	} else if (ps_type == PS_TEX) {
		sh_registers = (const uint32_t *)ps_tex_sh_registers_gfx9;
		context_registers = (const uint32_t *)ps_tex_context_reg_gfx9;
		num_sh_reg = ps_num_sh_registers_gfx9;
		num_context_reg = ps_num_context_registers_gfx9;
	}

	i = 0;

	/* 0x2c07   SPI_SHADER_PGM_RSRC3_PS
	   0x2c08   SPI_SHADER_PGM_LO_PS
	   0x2c09   SPI_SHADER_PGM_HI_PS */
	shader_addr += 256 * 9;
	ptr[i++] = PACKET3(PKT3_SET_SH_REG, 3);
	ptr[i++] = 0x7;
	ptr[i++] = 0xffff;
	ptr[i++] = shader_addr >> 8;
	ptr[i++] = shader_addr >> 40;

	for (j = 0; j < num_sh_reg; j++) {
		ptr[i++] = PACKET3(PKT3_SET_SH_REG, 1);
		ptr[i++] = sh_registers[j * 2] - 0x2c00;
		ptr[i++] = sh_registers[j * 2 + 1];
	}

	for (j = 0; j < num_context_reg; j++) {
		if (context_registers[j * 2] != 0xA1C5) {
			ptr[i++] = PACKET3(PACKET3_SET_CONTEXT_REG, 1);
			ptr[i++] = context_registers[j * 2] - 0xa000;
			ptr[i++] = context_registers[j * 2 + 1];
		}

		if (context_registers[j * 2] == 0xA1B4) {
			ptr[i++] = PACKET3(PACKET3_SET_CONTEXT_REG, 1);
			ptr[i++] = 0x1b3;
			ptr[i++] = 2;
		}
	}

	return i;
}

static int amdgpu_draw_draw(uint32_t *ptr)
{
	int i = 0;

	/* mmIA_MULTI_VGT_PARAM */
	ptr[i++] = PACKET3(PACKET3_SET_UCONFIG_REG, 1);
	ptr[i++] = 0x40000258;
	ptr[i++] = 0xd00ff;

	/* mmVGT_PRIMITIVE_TYPE */
	ptr[i++] = PACKET3(PACKET3_SET_UCONFIG_REG, 1);
	ptr[i++] = 0x10000242;
	ptr[i++] = 0x11;

	ptr[i++] = PACKET3(PACKET3_DRAW_INDEX_AUTO, 1);
	ptr[i++] = 3;
	ptr[i++] = 2;

	return i;
}

void amdgpu_memset_draw(amdgpu_device_handle device_handle,
			amdgpu_bo_handle bo_shader_ps,
			amdgpu_bo_handle bo_shader_vs,
			uint64_t mc_address_shader_ps,
			uint64_t mc_address_shader_vs,
			uint32_t ring_id)
{
	amdgpu_context_handle context_handle;
	amdgpu_bo_handle bo_dst, bo_cmd, resources[4];
	volatile unsigned char *ptr_dst;
	uint32_t *ptr_cmd;
	uint64_t mc_address_dst, mc_address_cmd;
	amdgpu_va_handle va_dst, va_cmd;
	int i, r;
	int bo_dst_size = 16384;
	int bo_cmd_size = 4096;
	struct amdgpu_cs_request ibs_request = {0};
	struct amdgpu_cs_ib_info ib_info = {0};
	struct amdgpu_cs_fence fence_status = {0};
	uint32_t expired;
	amdgpu_bo_list_handle bo_list;

	r = amdgpu_cs_ctx_create(device_handle, &context_handle);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_alloc_and_map(device_handle, bo_cmd_size, 4096,
					AMDGPU_GEM_DOMAIN_GTT, 0,
					&bo_cmd, (void **)&ptr_cmd,
					&mc_address_cmd, &va_cmd);
	CU_ASSERT_EQUAL(r, 0);
	memset(ptr_cmd, 0, bo_cmd_size);

	r = amdgpu_bo_alloc_and_map(device_handle, bo_dst_size, 4096,
					AMDGPU_GEM_DOMAIN_VRAM, 0,
					&bo_dst, (void **)&ptr_dst,
					&mc_address_dst, &va_dst);
	CU_ASSERT_EQUAL(r, 0);

	i = 0;
	i += amdgpu_draw_init(ptr_cmd + i);

	i += amdgpu_draw_setup_and_write_drawblt_surf_info(ptr_cmd + i, mc_address_dst);

	i += amdgpu_draw_setup_and_write_drawblt_state(ptr_cmd + i);

	i += amdgpu_draw_vs_RectPosTexFast_write2hw(ptr_cmd + i, PS_CONST, mc_address_shader_vs);

	i += amdgpu_draw_ps_write2hw(ptr_cmd + i, PS_CONST, mc_address_shader_ps);

	ptr_cmd[i++] = PACKET3(PKT3_SET_SH_REG, 4);
	ptr_cmd[i++] = 0xc;
	ptr_cmd[i++] = 0x33333333;
	ptr_cmd[i++] = 0x33333333;
	ptr_cmd[i++] = 0x33333333;
	ptr_cmd[i++] = 0x33333333;

	i += amdgpu_draw_draw(ptr_cmd + i);

	while (i & 7)
		ptr_cmd[i++] = 0xffff1000; /* type3 nop packet */

	resources[0] = bo_dst;
	resources[1] = bo_shader_ps;
	resources[2] = bo_shader_vs;
	resources[3] = bo_cmd;
	r = amdgpu_bo_list_create(device_handle, 3, resources, NULL, &bo_list);
	CU_ASSERT_EQUAL(r, 0);

	ib_info.ib_mc_address = mc_address_cmd;
	ib_info.size = i;
	ibs_request.ip_type = AMDGPU_HW_IP_GFX;
	ibs_request.ring = ring_id;
	ibs_request.resources = bo_list;
	ibs_request.number_of_ibs = 1;
	ibs_request.ibs = &ib_info;
	ibs_request.fence_info.handle = NULL;

	/* submit CS */
	r = amdgpu_cs_submit(context_handle, 0, &ibs_request, 1);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_list_destroy(bo_list);
	CU_ASSERT_EQUAL(r, 0);

	fence_status.ip_type = AMDGPU_HW_IP_GFX;
	fence_status.ip_instance = 0;
	fence_status.ring = ring_id;
	fence_status.context = context_handle;
	fence_status.fence = ibs_request.seq_no;

	/* wait for IB accomplished */
	r = amdgpu_cs_query_fence_status(&fence_status,
					 AMDGPU_TIMEOUT_INFINITE,
					 0, &expired);
	CU_ASSERT_EQUAL(r, 0);
	CU_ASSERT_EQUAL(expired, true);

	/* verify if memset test result meets with expected */
	i = 0;
	while(i < bo_dst_size) {
		CU_ASSERT_EQUAL(ptr_dst[i++], 0x33);
	}

	r = amdgpu_bo_unmap_and_free(bo_dst, va_dst, mc_address_dst, bo_dst_size);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_unmap_and_free(bo_cmd, va_cmd, mc_address_cmd, bo_cmd_size);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_cs_ctx_free(context_handle);
	CU_ASSERT_EQUAL(r, 0);
}

static void amdgpu_memset_draw_test(amdgpu_device_handle device_handle,
				    uint32_t ring)
{
	amdgpu_bo_handle bo_shader_ps, bo_shader_vs;
	void *ptr_shader_ps;
	void *ptr_shader_vs;
	uint64_t mc_address_shader_ps, mc_address_shader_vs;
	amdgpu_va_handle va_shader_ps, va_shader_vs;
	int r;
	int bo_shader_size = 4096;

	r = amdgpu_bo_alloc_and_map(device_handle, bo_shader_size, 4096,
					AMDGPU_GEM_DOMAIN_VRAM, 0,
					&bo_shader_ps, &ptr_shader_ps,
					&mc_address_shader_ps, &va_shader_ps);
	CU_ASSERT_EQUAL(r, 0);
	memset(ptr_shader_ps, 0, bo_shader_size);

	r = amdgpu_bo_alloc_and_map(device_handle, bo_shader_size, 4096,
					AMDGPU_GEM_DOMAIN_VRAM, 0,
					&bo_shader_vs, &ptr_shader_vs,
					&mc_address_shader_vs, &va_shader_vs);
	CU_ASSERT_EQUAL(r, 0);
	memset(ptr_shader_vs, 0, bo_shader_size);

	r = amdgpu_draw_load_ps_shader(ptr_shader_ps, PS_CONST);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_draw_load_vs_shader(ptr_shader_vs);
	CU_ASSERT_EQUAL(r, 0);

	amdgpu_memset_draw(device_handle, bo_shader_ps, bo_shader_vs,
			mc_address_shader_ps, mc_address_shader_vs, ring);

	r = amdgpu_bo_unmap_and_free(bo_shader_ps, va_shader_ps, mc_address_shader_ps, bo_shader_size);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_unmap_and_free(bo_shader_vs, va_shader_vs, mc_address_shader_vs, bo_shader_size);
	CU_ASSERT_EQUAL(r, 0);
}

static void amdgpu_memcpy_draw(amdgpu_device_handle device_handle,
			       amdgpu_bo_handle bo_shader_ps,
			       amdgpu_bo_handle bo_shader_vs,
			       uint64_t mc_address_shader_ps,
			       uint64_t mc_address_shader_vs,
			       uint32_t ring)
{
	amdgpu_context_handle context_handle;
	amdgpu_bo_handle bo_dst, bo_src, bo_cmd, resources[5];
	volatile unsigned char *ptr_dst;
	unsigned char *ptr_src;
	uint32_t *ptr_cmd;
	uint64_t mc_address_dst, mc_address_src, mc_address_cmd;
	amdgpu_va_handle va_dst, va_src, va_cmd;
	int i, r;
	int bo_size = 16384;
	int bo_cmd_size = 4096;
	struct amdgpu_cs_request ibs_request = {0};
	struct amdgpu_cs_ib_info ib_info= {0};
	uint32_t hang_state, hangs, expired;
	amdgpu_bo_list_handle bo_list;
	struct amdgpu_cs_fence fence_status = {0};

	r = amdgpu_cs_ctx_create(device_handle, &context_handle);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_alloc_and_map(device_handle, bo_cmd_size, 4096,
				    AMDGPU_GEM_DOMAIN_GTT, 0,
				    &bo_cmd, (void **)&ptr_cmd,
				    &mc_address_cmd, &va_cmd);
	CU_ASSERT_EQUAL(r, 0);
	memset(ptr_cmd, 0, bo_cmd_size);

	r = amdgpu_bo_alloc_and_map(device_handle, bo_size, 4096,
					AMDGPU_GEM_DOMAIN_VRAM, 0,
					&bo_src, (void **)&ptr_src,
					&mc_address_src, &va_src);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_alloc_and_map(device_handle, bo_size, 4096,
					AMDGPU_GEM_DOMAIN_VRAM, 0,
					&bo_dst, (void **)&ptr_dst,
					&mc_address_dst, &va_dst);
	CU_ASSERT_EQUAL(r, 0);

	memset(ptr_src, 0x55, bo_size);

	i = 0;
	i += amdgpu_draw_init(ptr_cmd + i);

	i += amdgpu_draw_setup_and_write_drawblt_surf_info(ptr_cmd + i, mc_address_dst);

	i += amdgpu_draw_setup_and_write_drawblt_state(ptr_cmd + i);

	i += amdgpu_draw_vs_RectPosTexFast_write2hw(ptr_cmd + i, PS_TEX, mc_address_shader_vs);

	i += amdgpu_draw_ps_write2hw(ptr_cmd + i, PS_TEX, mc_address_shader_ps);

	ptr_cmd[i++] = PACKET3(PKT3_SET_SH_REG, 8);
	ptr_cmd[i++] = 0xc;
	ptr_cmd[i++] = mc_address_src >> 8;
	ptr_cmd[i++] = mc_address_src >> 40 | 0x10e00000;
	ptr_cmd[i++] = 0x7c01f;
	ptr_cmd[i++] = 0x90500fac;
	ptr_cmd[i++] = 0x3e000;
	i += 3;

	ptr_cmd[i++] = PACKET3(PKT3_SET_SH_REG, 4);
	ptr_cmd[i++] = 0x14;
	ptr_cmd[i++] = 0x92;
	i += 3;

	ptr_cmd[i++] = PACKET3(PACKET3_SET_CONTEXT_REG, 1);
	ptr_cmd[i++] = 0x191;
	ptr_cmd[i++] = 0;

	i += amdgpu_draw_draw(ptr_cmd + i);

	while (i & 7)
		ptr_cmd[i++] = 0xffff1000; /* type3 nop packet */

	resources[0] = bo_dst;
	resources[1] = bo_src;
	resources[2] = bo_shader_ps;
	resources[3] = bo_shader_vs;
	resources[4] = bo_cmd;
	r = amdgpu_bo_list_create(device_handle, 5, resources, NULL, &bo_list);
	CU_ASSERT_EQUAL(r, 0);

	ib_info.ib_mc_address = mc_address_cmd;
	ib_info.size = i;
	ibs_request.ip_type = AMDGPU_HW_IP_GFX;
	ibs_request.ring = ring;
	ibs_request.resources = bo_list;
	ibs_request.number_of_ibs = 1;
	ibs_request.ibs = &ib_info;
	ibs_request.fence_info.handle = NULL;
	r = amdgpu_cs_submit(context_handle, 0, &ibs_request, 1);
	CU_ASSERT_EQUAL(r, 0);

	fence_status.ip_type = AMDGPU_HW_IP_GFX;
	fence_status.ip_instance = 0;
	fence_status.ring = ring;
	fence_status.context = context_handle;
	fence_status.fence = ibs_request.seq_no;

	/* wait for IB accomplished */
	r = amdgpu_cs_query_fence_status(&fence_status,
					 AMDGPU_TIMEOUT_INFINITE,
					 0, &expired);
	CU_ASSERT_EQUAL(r, 0);
	CU_ASSERT_EQUAL(expired, true);

	/* verify if memcpy test result meets with expected */
	i = 0;
	while(i < bo_size) {
		CU_ASSERT_EQUAL(ptr_dst[i], ptr_src[i]);
		i++;
	}

	r = amdgpu_bo_list_destroy(bo_list);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_unmap_and_free(bo_src, va_src, mc_address_src, bo_size);
	CU_ASSERT_EQUAL(r, 0);
	r = amdgpu_bo_unmap_and_free(bo_dst, va_dst, mc_address_dst, bo_size);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_unmap_and_free(bo_cmd, va_cmd, mc_address_cmd, bo_cmd_size);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_cs_ctx_free(context_handle);
	CU_ASSERT_EQUAL(r, 0);
}

static void amdgpu_memcpy_draw_test(amdgpu_device_handle device_handle, uint32_t ring)
{
	amdgpu_bo_handle bo_shader_ps, bo_shader_vs;
	void *ptr_shader_ps;
	void *ptr_shader_vs;
	uint64_t mc_address_shader_ps, mc_address_shader_vs;
	amdgpu_va_handle va_shader_ps, va_shader_vs;
	int bo_shader_size = 4096;
	int r;

	r = amdgpu_bo_alloc_and_map(device_handle, bo_shader_size, 4096,
					AMDGPU_GEM_DOMAIN_VRAM, 0,
					&bo_shader_ps, &ptr_shader_ps,
					&mc_address_shader_ps, &va_shader_ps);
	CU_ASSERT_EQUAL(r, 0);
	memset(ptr_shader_ps, 0, bo_shader_size);

	r = amdgpu_bo_alloc_and_map(device_handle, bo_shader_size, 4096,
					AMDGPU_GEM_DOMAIN_VRAM, 0,
					&bo_shader_vs, &ptr_shader_vs,
					&mc_address_shader_vs, &va_shader_vs);
	CU_ASSERT_EQUAL(r, 0);
	memset(ptr_shader_vs, 0, bo_shader_size);

	r = amdgpu_draw_load_ps_shader(ptr_shader_ps, PS_TEX);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_draw_load_vs_shader(ptr_shader_vs);
	CU_ASSERT_EQUAL(r, 0);

	amdgpu_memcpy_draw(device_handle, bo_shader_ps, bo_shader_vs,
			mc_address_shader_ps, mc_address_shader_vs, ring);

	r = amdgpu_bo_unmap_and_free(bo_shader_ps, va_shader_ps, mc_address_shader_ps, bo_shader_size);
	CU_ASSERT_EQUAL(r, 0);

	r = amdgpu_bo_unmap_and_free(bo_shader_vs, va_shader_vs, mc_address_shader_vs, bo_shader_size);
	CU_ASSERT_EQUAL(r, 0);
}

static void amdgpu_draw_test(void)
{
	int r;
	struct drm_amdgpu_info_hw_ip info;
	uint32_t ring_id;

	r = amdgpu_query_hw_ip_info(device_handle, AMDGPU_HW_IP_GFX, 0, &info);
	CU_ASSERT_EQUAL(r, 0);
	if (!info.available_rings)
		printf("SKIP ... as there's no graphics ring\n");

	for (ring_id = 0; (1 << ring_id) & info.available_rings; ring_id++) {
		amdgpu_memset_draw_test(device_handle, ring_id);
		amdgpu_memcpy_draw_test(device_handle, ring_id);
	}
}

static void amdgpu_gpu_reset_test(void)
{
	int r;
	char debugfs_path[256], tmp[10];
	int fd;
	struct stat sbuf;
	amdgpu_context_handle context_handle;
	uint32_t hang_state, hangs;

	r = amdgpu_cs_ctx_create(device_handle, &context_handle);
	CU_ASSERT_EQUAL(r, 0);

	r = fstat(drm_amdgpu[0], &sbuf);
	CU_ASSERT_EQUAL(r, 0);

	sprintf(debugfs_path, "/sys/kernel/debug/dri/%d/amdgpu_gpu_recover", minor(sbuf.st_rdev));
	fd = open(debugfs_path, O_RDONLY);
	CU_ASSERT(fd >= 0);

	r = read(fd, tmp, sizeof(tmp)/sizeof(char));
	CU_ASSERT(r > 0);

	r = amdgpu_cs_query_reset_state(context_handle, &hang_state, &hangs);
	CU_ASSERT_EQUAL(r, 0);
	CU_ASSERT_EQUAL(hang_state, AMDGPU_CTX_UNKNOWN_RESET);

	close(fd);
	r = amdgpu_cs_ctx_free(context_handle);
	CU_ASSERT_EQUAL(r, 0);

	amdgpu_compute_dispatch_test();
	amdgpu_gfx_dispatch_test();
}
