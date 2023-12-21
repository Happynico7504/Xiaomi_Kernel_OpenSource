/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2020 MediaTek Inc.
 */

#include "mtk_cpufreq_config.h"

#define NR_FREQ		16
#define ARRAY_COL_SIZE	4

static unsigned int fyTbl[NR_FREQ * NR_MT_CPU_DVFS][ARRAY_COL_SIZE] = {
	/* Freq, Vproc, post_div, clk_div */
	{ 2600, 100, 1, 1 },	/* LL */
	{ 2400, 95, 1, 1 },
	{ 2200, 90, 1, 1 },
	{ 1900, 67, 1, 1 },
	{ 1800, 63, 1, 1 },
	{ 1700, 59, 1, 1 },
	{ 1600, 53, 1, 1 },
	{ 1500, 49, 2, 1 },
	{ 1400, 45, 2, 1 },
	{ 1300, 41, 2, 1 },
	{ 1200, 37, 2, 1 },
	{ 1100, 33, 2, 1 },
	{ 1000, 30, 2, 1 },
	{  500, 25, 2, 1 },
	{  300, 25, 2, 1 },
	{  100, 25, 2, 1 },
};

static unsigned int sbTbl[NR_FREQ * NR_MT_CPU_DVFS][ARRAY_COL_SIZE] = {
	/* Freq, Vproc, post_div, clk_div */
        { 2600, 100, 1, 1 },	/* LL */
	{ 2400, 95, 1, 1 },
	{ 2200, 90, 1, 1 },
	{ 2000, 67, 1, 1 },
	{ 1900, 63, 1, 1 },
	{ 1800, 59, 1, 1 },
	{ 1700, 55, 1, 1 },
	{ 1600, 51, 1, 1 },
	{ 1500, 48, 2, 1 },
	{ 1400, 45, 2, 1 },
	{ 1300, 41, 2, 1 },
	{ 1200, 37, 2, 1 },
	{ 1100, 33, 2, 1 },
	{ 1000, 29, 2, 1 },
	{ 300, 25, 2, 1 },
	{ 100, 25, 2, 1 },
};

static unsigned int fy2Tbl[NR_FREQ * NR_MT_CPU_DVFS][ARRAY_COL_SIZE] = {
	/* Freq, Vproc, post_div, clk_div */
        { 2600, 100, 1, 1 },	/* LL */
	{ 2400, 95, 1, 1 },
	{ 2200, 90, 1, 1 },
	{ 1900, 67, 1, 1 },
	{ 1800, 63, 1, 1 },
	{ 1700, 59, 1, 1 },
	{ 1600, 53, 1, 1 },
	{ 1500, 49, 2, 1 },
	{ 1400, 45, 2, 1 },
	{ 1300, 45, 2, 1 },
	{ 1200, 45, 2, 1 },
	{ 1100, 45, 2, 1 },
	{ 1000, 45, 2, 1 },
	{  500, 40, 2, 1 },
	{  300, 40, 2, 1 },
	{  100, 40, 2, 1 },
};

static unsigned int LiteTbl[NR_FREQ * NR_MT_CPU_DVFS][ARRAY_COL_SIZE] = {
	/* Freq, Vproc, post_div, clk_div */
	{ 2600, 100, 1, 1 },	/* LL */
	{ 2400, 95, 1, 1 },
	{ 2200, 90, 1, 1 },
	{ 1900, 63, 1, 1 },
	{ 1800, 61, 1, 1 },
	{ 1700, 59, 1, 1 },
	{ 1600, 53, 1, 1 },
	{ 1500, 49, 2, 1 },
	{ 1400, 45, 2, 1 },
	{ 1300, 41, 2, 1 },
	{ 1200, 37, 2, 1 },
	{ 1100, 33, 2, 1 },
	{ 1000, 30, 2, 1 },
	{  500, 25, 2, 1 },
	{  300, 25, 2, 1 },
	{  100, 25, 2, 1 },
};

unsigned int *xrecordTbl[NUM_CPU_LEVEL] = {	/* v0.3 */
	[CPU_LEVEL_0] = &fyTbl[0][0],
	[CPU_LEVEL_1] = &sbTbl[0][0],
	[CPU_LEVEL_2] = &fy2Tbl[0][0],
	[CPU_LEVEL_3] = &LiteTbl[0][0],
};
