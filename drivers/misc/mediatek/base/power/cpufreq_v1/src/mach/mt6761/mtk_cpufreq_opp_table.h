/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2020 MediaTek Inc.
 */

#include "mtk_cpufreq_struct.h"
#include "mtk_cpufreq_config.h"

/* FY */
/* for DVFS OPP table LL */
#define CPU_DVFS_FREQ0_LL_FY    2600000    /* KHz */
#define CPU_DVFS_FREQ1_LL_FY    2400000    /* KHz */
#define CPU_DVFS_FREQ2_LL_FY    2200000    /* KHz */
#define CPU_DVFS_FREQ3_LL_FY    1900000    /* KHz */
#define CPU_DVFS_FREQ4_LL_FY    1800000    /* KHz */
#define CPU_DVFS_FREQ5_LL_FY    1700000    /* KHz */
#define CPU_DVFS_FREQ6_LL_FY    1600000    /* KHz */
#define CPU_DVFS_FREQ7_LL_FY    1500000    /* KHz */
#define CPU_DVFS_FREQ8_LL_FY    1400000    /* KHz */
#define CPU_DVFS_FREQ9_LL_FY    1300000    /* KHz */
#define CPU_DVFS_FREQ10_LL_FY   1200000    /* KHz */
#define CPU_DVFS_FREQ11_LL_FY   1100000    /* KHz */
#define CPU_DVFS_FREQ12_LL_FY   1000000    /* KHz */
#define CPU_DVFS_FREQ13_LL_FY    300000    /* KHz */
#define CPU_DVFS_FREQ14_LL_FY    100000     /* KHz */
#define CPU_DVFS_FREQ15_LL_FY    0     /* KHz */

#define CPU_DVFS_VOLT0_VPROC_LL_FY    120000           /* 10uV */
#define CPU_DVFS_VOLT1_VPROC_LL_FY    110000           /* 10uV */
#define CPU_DVFS_VOLT2_VPROC_LL_FY    100000           /* 10uV */
#define CPU_DVFS_VOLT3_VPROC_LL_FY     93750           /* 10uV */
#define CPU_DVFS_VOLT4_VPROC_LL_FY     91250           /* 10uV */
#define CPU_DVFS_VOLT5_VPROC_LL_FY     88750           /* 10uV */
#define CPU_DVFS_VOLT6_VPROC_LL_FY     85000           /* 10uV */
#define CPU_DVFS_VOLT7_VPROC_LL_FY     82500           /* 10uV */
#define CPU_DVFS_VOLT8_VPROC_LL_FY     80000           /* 10uV */
#define CPU_DVFS_VOLT9_VPROC_LL_FY     77500           /* 10uV */
#define CPU_DVFS_VOLT10_VPROC_LL_FY    75000           /* 10uV */
#define CPU_DVFS_VOLT11_VPROC_LL_FY    72500           /* 10uV */
#define CPU_DVFS_VOLT12_VPROC_LL_FY    70625           /* 10uV */
#define CPU_DVFS_VOLT13_VPROC_LL_FY    50000           /* 10uV */
#define CPU_DVFS_VOLT14_VPROC_LL_FY    45000           /* 10uV */
#define CPU_DVFS_VOLT15_VPROC_LL_FY    40000           /* 10uV */

/* SB */
/* for DVFS OPP table LL */
#define CPU_DVFS_FREQ0_LL_SB    2600000    /* KHz */
#define CPU_DVFS_FREQ1_LL_SB    2400000    /* KHz */
#define CPU_DVFS_FREQ2_LL_SB    2200000    /* KHz */
#define CPU_DVFS_FREQ3_LL_SB    2000000    /* KHz */
#define CPU_DVFS_FREQ4_LL_SB    1900000    /* KHz */
#define CPU_DVFS_FREQ5_LL_SB    1800000    /* KHz */
#define CPU_DVFS_FREQ6_LL_SB    1700000    /* KHz */
#define CPU_DVFS_FREQ7_LL_SB    1600000    /* KHz */
#define CPU_DVFS_FREQ8_LL_SB    1500000    /* KHz */
#define CPU_DVFS_FREQ9_LL_SB    1400000    /* KHz */
#define CPU_DVFS_FREQ10_LL_SB   1300000    /* KHz */
#define CPU_DVFS_FREQ11_LL_SB   1200000    /* KHz */
#define CPU_DVFS_FREQ12_LL_SB   1100000    /* KHz */
#define CPU_DVFS_FREQ13_LL_SB   1000000    /* KHz */
#define CPU_DVFS_FREQ14_LL_SB    300000    /* KHz */
#define CPU_DVFS_FREQ15_LL_SB    0     /* KHz */

#define CPU_DVFS_VOLT0_VPROC_LL_SB    120000            /* 10uV */
#define CPU_DVFS_VOLT1_VPROC_LL_SB     99375            /* 10uV */
#define CPU_DVFS_VOLT2_VPROC_LL_SB     96250            /* 10uV */
#define CPU_DVFS_VOLT3_VPROC_LL_SB     93750            /* 10uV */
#define CPU_DVFS_VOLT4_VPROC_LL_SB     91250            /* 10uV */
#define CPU_DVFS_VOLT5_VPROC_LL_SB     88750            /* 10uV */
#define CPU_DVFS_VOLT6_VPROC_LL_SB     86250            /* 10uV */
#define CPU_DVFS_VOLT7_VPROC_LL_SB     83750            /* 10uV */
#define CPU_DVFS_VOLT8_VPROC_LL_SB     81875            /* 10uV */
#define CPU_DVFS_VOLT9_VPROC_LL_SB     80000            /* 10uV */
#define CPU_DVFS_VOLT10_VPROC_LL_SB    77500            /* 10uV */
#define CPU_DVFS_VOLT11_VPROC_LL_SB    75000            /* 10uV */
#define CPU_DVFS_VOLT12_VPROC_LL_SB    72500            /* 10uV */
#define CPU_DVFS_VOLT13_VPROC_LL_SB    70000            /* 10uV */
#define CPU_DVFS_VOLT14_VPROC_LL_SB    50000            /* 10uV */
#define CPU_DVFS_VOLT15_VPROC_LL_SB    40000            /* 10uV */

/* FY2 */
/* for DVFS OPP table LL */
#define CPU_DVFS_FREQ0_LL_FY2    2600000    /* KHz */
#define CPU_DVFS_FREQ1_LL_FY2    2400000    /* KHz */
#define CPU_DVFS_FREQ2_LL_FY2    2200000    /* KHz */
#define CPU_DVFS_FREQ3_LL_FY2    1900000    /* KHz */
#define CPU_DVFS_FREQ4_LL_FY2    1800000    /* KHz */
#define CPU_DVFS_FREQ5_LL_FY2    1700000    /* KHz */
#define CPU_DVFS_FREQ6_LL_FY2    1600000    /* KHz */
#define CPU_DVFS_FREQ7_LL_FY2    1500000    /* KHz */
#define CPU_DVFS_FREQ8_LL_FY2    1400000    /* KHz */
#define CPU_DVFS_FREQ9_LL_FY2    1300000    /* KHz */
#define CPU_DVFS_FREQ10_LL_FY2   1200000    /* KHz */
#define CPU_DVFS_FREQ11_LL_FY2   1100000    /* KHz */
#define CPU_DVFS_FREQ12_LL_FY2   1000000    /* KHz */
#define CPU_DVFS_FREQ13_LL_FY2    300000    /* KHz */
#define CPU_DVFS_FREQ14_LL_FY2    100000     /* KHz */
#define CPU_DVFS_FREQ15_LL_FY2    0     /* KHz */

#define CPU_DVFS_VOLT0_VPROC_LL_FY2    120000           /* 10uV */
#define CPU_DVFS_VOLT1_VPROC_LL_FY2    110000           /* 10uV */
#define CPU_DVFS_VOLT2_VPROC_LL_FY2    100000           /* 10uV */
#define CPU_DVFS_VOLT3_VPROC_LL_FY2     93750           /* 10uV */
#define CPU_DVFS_VOLT4_VPROC_LL_FY2     91250           /* 10uV */
#define CPU_DVFS_VOLT5_VPROC_LL_FY2     88750           /* 10uV */
#define CPU_DVFS_VOLT6_VPROC_LL_FY2     85000           /* 10uV */
#define CPU_DVFS_VOLT7_VPROC_LL_FY2     82500           /* 10uV */
#define CPU_DVFS_VOLT8_VPROC_LL_FY2     80000           /* 10uV */
#define CPU_DVFS_VOLT9_VPROC_LL_FY2     80000           /* 10uV */
#define CPU_DVFS_VOLT10_VPROC_LL_FY2    80000           /* 10uV */
#define CPU_DVFS_VOLT11_VPROC_LL_FY2    80000           /* 10uV */
#define CPU_DVFS_VOLT12_VPROC_LL_FY2    80000           /* 10uV */
#define CPU_DVFS_VOLT13_VPROC_LL_FY2    50000           /* 10uV */
#define CPU_DVFS_VOLT14_VPROC_LL_FY2    45000           /* 10uV */
#define CPU_DVFS_VOLT15_VPROC_LL_FY2    40000           /* 10uV */

/* Lite */
/* for DVFS OPP table LL */
#define CPU_DVFS_FREQ0_LL_LITE    2600000    /* KHz */
#define CPU_DVFS_FREQ1_LL_LITE    2400000    /* KHz */
#define CPU_DVFS_FREQ2_LL_LITE    2200000    /* KHz */
#define CPU_DVFS_FREQ3_LL_LITE    1900000    /* KHz */
#define CPU_DVFS_FREQ4_LL_LITE    1800000    /* KHz */
#define CPU_DVFS_FREQ5_LL_LITE    1700000    /* KHz */
#define CPU_DVFS_FREQ6_LL_LITE    1600000    /* KHz */
#define CPU_DVFS_FREQ7_LL_LITE    1500000    /* KHz */
#define CPU_DVFS_FREQ8_LL_LITE    1400000    /* KHz */
#define CPU_DVFS_FREQ9_LL_LITE    1300000    /* KHz */
#define CPU_DVFS_FREQ10_LL_LITE   1200000    /* KHz */
#define CPU_DVFS_FREQ11_LL_LITE   1100000    /* KHz */
#define CPU_DVFS_FREQ12_LL_LITE   1000000    /* KHz */
#define CPU_DVFS_FREQ13_LL_LITE    300000    /* KHz */
#define CPU_DVFS_FREQ14_LL_LITE    100000     /* KHz */
#define CPU_DVFS_FREQ15_LL_LITE    0     /* KHz */

#define CPU_DVFS_VOLT0_VPROC_LL_LITE     120000          /* 10uV */
#define CPU_DVFS_VOLT1_VPROC_LL_LITE     110000          /* 10uV */
#define CPU_DVFS_VOLT2_VPROC_LL_LITE     100000          /* 10uV */
#define CPU_DVFS_VOLT3_VPROC_LL_LITE     91250           /* 10uV */
#define CPU_DVFS_VOLT4_VPROC_LL_LITE     90000           /* 10uV */
#define CPU_DVFS_VOLT5_VPROC_LL_LITE     88750           /* 10uV */
#define CPU_DVFS_VOLT6_VPROC_LL_LITE     85000           /* 10uV */
#define CPU_DVFS_VOLT7_VPROC_LL_LITE     82500           /* 10uV */
#define CPU_DVFS_VOLT8_VPROC_LL_LITE     80000           /* 10uV */
#define CPU_DVFS_VOLT9_VPROC_LL_LITE     77500           /* 10uV */
#define CPU_DVFS_VOLT10_VPROC_LL_LITE    75000           /* 10uV */
#define CPU_DVFS_VOLT11_VPROC_LL_LITE    72500           /* 10uV */
#define CPU_DVFS_VOLT12_VPROC_LL_LITE    70625           /* 10uV */
#define CPU_DVFS_VOLT13_VPROC_LL_LITE    50000           /* 10uV */
#define CPU_DVFS_VOLT14_VPROC_LL_LITE    45000           /* 10uV */
#define CPU_DVFS_VOLT15_VPROC_LL_LITE    40000           /* 10uV */

/* DVFS OPP table */
#define OPP_TBL(cluster, seg, lv, vol)	\
static struct mt_cpu_freq_info opp_tbl_##cluster##_e##lv##_0[] = {        \
	OP                                                                \
(CPU_DVFS_FREQ0_##cluster##_##seg, CPU_DVFS_VOLT0_VPROC_##vol##_##seg),   \
	OP                                                                \
(CPU_DVFS_FREQ1_##cluster##_##seg, CPU_DVFS_VOLT1_VPROC_##vol##_##seg),   \
	OP                                                                \
(CPU_DVFS_FREQ2_##cluster##_##seg, CPU_DVFS_VOLT2_VPROC_##vol##_##seg),   \
	OP                                                                \
(CPU_DVFS_FREQ3_##cluster##_##seg, CPU_DVFS_VOLT3_VPROC_##vol##_##seg),   \
	OP                                                                \
(CPU_DVFS_FREQ4_##cluster##_##seg, CPU_DVFS_VOLT4_VPROC_##vol##_##seg),   \
	OP                                                                \
(CPU_DVFS_FREQ5_##cluster##_##seg, CPU_DVFS_VOLT5_VPROC_##vol##_##seg),   \
	OP                                                                \
(CPU_DVFS_FREQ6_##cluster##_##seg, CPU_DVFS_VOLT6_VPROC_##vol##_##seg),   \
	OP                                                                \
(CPU_DVFS_FREQ7_##cluster##_##seg, CPU_DVFS_VOLT7_VPROC_##vol##_##seg),   \
	OP                                                                \
(CPU_DVFS_FREQ8_##cluster##_##seg, CPU_DVFS_VOLT8_VPROC_##vol##_##seg),   \
	OP                                                                \
(CPU_DVFS_FREQ9_##cluster##_##seg, CPU_DVFS_VOLT9_VPROC_##vol##_##seg),   \
	OP                                                                \
(CPU_DVFS_FREQ10_##cluster##_##seg, CPU_DVFS_VOLT10_VPROC_##vol##_##seg), \
	OP                                                                \
(CPU_DVFS_FREQ11_##cluster##_##seg, CPU_DVFS_VOLT11_VPROC_##vol##_##seg), \
	OP                                                                \
(CPU_DVFS_FREQ12_##cluster##_##seg, CPU_DVFS_VOLT12_VPROC_##vol##_##seg), \
	OP                                                                \
(CPU_DVFS_FREQ13_##cluster##_##seg, CPU_DVFS_VOLT13_VPROC_##vol##_##seg), \
	OP                                                                \
(CPU_DVFS_FREQ14_##cluster##_##seg, CPU_DVFS_VOLT14_VPROC_##vol##_##seg), \
	OP                                                                \
(CPU_DVFS_FREQ15_##cluster##_##seg, CPU_DVFS_VOLT15_VPROC_##vol##_##seg), \
}

OPP_TBL(LL,  FY, 0, LL); /* opp_tbl_LL_e0_0  */
OPP_TBL(LL,  SB, 1, LL); /* opp_tbl_LL_e1_0  */
OPP_TBL(LL,  FY2, 2, LL); /* opp_tbl_LL_e2_0  */
OPP_TBL(LL,  LITE, 3, LL); /* opp_tbl_LL_e2_0  */


/* v0.3 */
struct opp_tbl_info opp_tbls[NR_MT_CPU_DVFS][NUM_CPU_LEVEL] = {
	/* LL */
	{
		[CPU_LEVEL_0] = { opp_tbl_LL_e0_0,
				ARRAY_SIZE(opp_tbl_LL_e0_0) },
		[CPU_LEVEL_1] = { opp_tbl_LL_e1_0,
				ARRAY_SIZE(opp_tbl_LL_e1_0) },
		[CPU_LEVEL_2] = { opp_tbl_LL_e2_0,
				ARRAY_SIZE(opp_tbl_LL_e2_0) },
		[CPU_LEVEL_3] = { opp_tbl_LL_e3_0,
				ARRAY_SIZE(opp_tbl_LL_e2_0) },
	},
};

/* 16 steps OPP table */
/* FY */
static struct mt_cpu_freq_method opp_tbl_method_LL_FY[] = {
	/* POS,	CLK */
	FP(1,	1),
	FP(1,	1),
	FP(1,	1),
	FP(1,	1),
	FP(1,	1),
	FP(1,	1),
	FP(1,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
};

/* SB */
static struct mt_cpu_freq_method opp_tbl_method_LL_SB[] = {
	/* POS,	CLK */
	FP(1,	1),
	FP(1,	1),
	FP(1,	1),
	FP(1,	1),
	FP(1,	1),
	FP(1,	1),
	FP(1,	1),
	FP(1,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
};

/* FY2 */
static struct mt_cpu_freq_method opp_tbl_method_LL_FY2[] = {
	/* POS,	CLK */
	FP(1,	1),
	FP(1,	1),
	FP(1,	1),
	FP(1,	1),
	FP(1,	1),
	FP(1,	1),
	FP(1,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
};
/* Lite */
static struct mt_cpu_freq_method opp_tbl_method_LL_LITE[] = {
	/* POS,	CLK */
	FP(1,	1),
	FP(1,	1),
	FP(1,	1),
	FP(1,	1),
	FP(1,	1),
	FP(1,	1),
	FP(1,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
};
/* v0.3 */
struct opp_tbl_m_info opp_tbls_m[NR_MT_CPU_DVFS][NUM_CPU_LEVEL] = {
	/* LL */
	{
		[CPU_LEVEL_0] = { opp_tbl_method_LL_FY },
		[CPU_LEVEL_1] = { opp_tbl_method_LL_SB },
		[CPU_LEVEL_2] = { opp_tbl_method_LL_FY2 },
		[CPU_LEVEL_3] = { opp_tbl_method_LL_LITE },
	},
};
