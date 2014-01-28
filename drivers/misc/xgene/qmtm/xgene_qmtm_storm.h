/*
 * AppliedMicro X-Gene SOC Queue Manager/Traffic Manager driver
 *
 * Copyright (c) 2013 Applied Micro Circuits Corporation.
 * Author: Ravi Patel <rapatel@apm.com>
 *         Keyur Chudgar <kchudgar@apm.com>
 *         Fushen Chen <fchen@apm.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef __XGENE_QMTM_STORM_H__
#define __XGENE_QMTM_STORM_H__

/* QMTM Queue State */
struct storm_qmtm_csr_qstate {
	u32 w0;
	u32 w1;
	u32 w2;
	u32 w3;
	u32 w4;
} __packed;

/*
 * Physical or free pool queue state (pq or fp)
 */
struct storm_qmtm_pq_fp_qstate {
#ifdef CONFIG_CPU_BIG_ENDIAN
	/* register word 0 (bit 31:0) */
	u32 cpu_notify:8;           /* 31:24 */
	u32 cfgsaben:1;             /* 23 enable SAB broadcasting */
	u32 cfgtmvq:10;             /* 22:13 parent vq */
	u32 cfgtmvqen:1;            /* 12 enable pq to belong to vq */
	u32 resize_done:1;          /* 11 */
	u32 resize_start:1;         /* 10 */
	u32 resize_qid:10;          /* 9:0 */

	/* register word 1 (bit 63:32) */
	u32 headptr:15;             /* 63:49 */
	u32 nummsg:16;              /* 48:33 */
	u32 cfgnotifyqne:1;         /* 32 enable Q not empty intr */

	/* register word 2 (bit 95:64) */
	u32 cfgstartaddrL:27;       /* 95:69 split 7/27 */
	u32 qcoherent:1;            /* 68 */
	u32 rid:3;                  /* 67:65 */
	u32 cfgcrid:1;              /* 64 */

	/* register word 3 (bit 127:96) */
	u32 cfgRecombBufTimeoutL:4; /* 127:124 split 3/4 */
	u32 cfgRecombBuf:1;         /* 123 */
	u32 qstatelock:1;           /* 122 */
	u32 cfgqsize:3;             /* 121:119 queue size */
	u32 fp_mode:3;              /* 118:116 free pool mode */
	u32 cfgacceptlerr:1;        /* 115 */
	u32 reserved_0:1;           /* 114 */
	u32 stashing:1;             /* 113 */
	u32 slot_pending:8;         /* 112:105 */
	u32 vc_chanid:2;            /* 104:103 */
	u32 cfgstartaddrH:7;        /* 102:96 split 7/27 */

	/* register word 4 (bit 159:128) */
	u32 resv1:11;               /* 159:149 */
	u32 cfgqtype:2;             /* 148:147 queue type */
	u32 resv2:5;                /* 146:142 */
	u32 half_64B_override:3;    /* 141:139 */
	u32 resv3:4;                /* 138:135 */
	u32 CfgSupressCmpl:1;       /* 134 */
	u32 cfgselthrsh:3;          /* 133:131 associated threshold set */
	u32 cfgRecombBufTimeoutH:3; /* 130:128 split 3/4 */
#else
	/* register word 0 (bit 31:0) */
	u32 resize_qid:10;          /* 9:0 */
	u32 resize_start:1;         /* 10 */
	u32 resize_done:1;          /* 11 */
	u32 cfgtmvqen:1;            /* 12 enable pq to belong to vq */
	u32 cfgtmvq:10;             /* 22:13 parent vq */
	u32 cfgsaben:1;             /* 23 enable SAB broadcasting */
	u32 cpu_notify:8;           /* 31:24 */

	/* register word 1 (bit 63:32) */
	u32 cfgnotifyqne:1;         /* 32 enable Q not empty intr */
	u32 nummsg:16;              /* 48:33 */
	u32 headptr:15;             /* 63:49 */

	/* register word 2 (bit 95:64) */
	u32 cfgcrid:1;              /* 64 */
	u32 rid:3;                  /* 67:65 */
	u32 qcoherent:1;            /* 68 */
	u32 cfgstartaddrL:27;       /* 95:69 split 7/27 */

	/* register word 3 (bit 127:96) */
	u32 cfgstartaddrH:7;        /* 102:96 split 7/27 */
	u32 vc_chanid:2;            /* 104:103 */
	u32 slot_pending:8;         /* 112:105 */
	u32 stashing:1;             /* 113 */
	u32 reserved_0:1;           /* 114 */
	u32 cfgacceptlerr:1;        /* 115 */
	u32 fp_mode:3;              /* 118:116 free pool mode */
	u32 cfgqsize:3;             /* 121:119 queue size */
	u32 qstatelock:1;           /* 122 */
	u32 cfgRecombBuf:1;         /* 123 */
	u32 cfgRecombBufTimeoutL:4; /* 127:124 split 3/4 */

	/* register word 4 (bit 159:128) */
	u32 cfgRecombBufTimeoutH:3; /* 130:128 split 3/4 */
	u32 cfgselthrsh:3;          /* 133:131 associated threshold set */
	u32 CfgSupressCmpl:1;       /* 134 */
	u32 resv3:4;                /* 138:135 */
	u32 half_64B_override:3;    /* 141:139 */
	u32 resv2:5;                /* 146:142 */
	u32 cfgqtype:2;             /* 148:147 queue type */
	u32 resv1:11;               /* 159:149 */
#endif
} __packed;

struct storm_qmtm_vq_qstate {
#ifdef CONFIG_CPU_BIG_ENDIAN
	/* register word 0 (bit 31:0) */
	u32 rid:3;          /* 31:29 */
	u32 cpu_notify:8;   /* 28:21 */
	u32 cfgcrid:1;      /* 20 critical rid config */
	u32 cfgnotifyqne:1; /* 19 enable Q not empty intr */
	u32 cfgsaben:1;     /* 18 enable SAB broadcasting */
	u32 nummsg:18;      /* 17:0 */

	/* register word 1 (bit 63:32) */
	u32 q5reqvld:1;     /* 63 */
	u32 q5txallowed:1;  /* 62 */
	u32 q5selarb:2;     /* 61:60 */
	u32 q6_sel:10;      /* 59:50 */
	u32 q6reqvld:1;     /* 49 */
	u32 q6txallowed:1;  /* 48 */
	u32 q6selarb:2;     /* 47:46 */
	u32 q7_sel:10;      /* 45:36 */
	u32 q7reqvld:1;     /* 35 */
	u32 q7txallowed:1;  /* 34 */
	u32 q7selarb:2;     /* 33:32 */

	/* register word 2 (bit 95:64) */
	u32 q3_selL:4;      /* 95:92 split 4/6 */
	u32 q3reqvld:1;     /* 91 */
	u32 q3txallowed:1;  /* 90 */
	u32 q3selarb:2;     /* 89:88 */
	u32 q4_sel:10;      /* 87:78 */
	u32 q4reqvld:1;     /* 77 */
	u32 q4txallowed:1;  /* 76 */
	u32 q4selarb:2;     /* 75:74 */
	u32 q5_sel:10;      /* 73:64 */

	/* register word 3 (bit 127:96) */
	u32 q1_selL:8;      /* 127:120 split 2/8 */
	u32 q1reqvld:1;     /* 119 */
	u32 q1txallowed:1;  /* 118 */
	u32 q1selarb:2;     /* 117:116 */
	u32 q2_sel:10;      /* 115:106 */
	u32 q2reqvld:1;     /* 105 */
	u32 q2txallowed:1;  /* 104 */
	u32 q2selarb:2;     /* 103:102 */
	u32 q3_selH:6;      /* 101:96 split 4/6 */

	/* register word 4 (bit 159:128) */
	u32 resv1:11;       /* 159:149 */
	u32 cfgqtype:2;     /* 148:147 queue type */
	u32 cfgselthrsh:3;  /* 146:144 associated threshold set */
	u32 q0_sel:10;      /* 143:134 */
	u32 q0reqvld:1;     /* 133 */
	u32 q0txallowed:1;  /* 132 */
	u32 q0selarb:2;     /* 131:130 */
	u32 q1_selH:2;      /* 129:128 split 2/8 */
#else
	/* register word 0 (bit 31:0) */
	u32 nummsg:18;      /* 17:0 */
	u32 cfgsaben:1;     /* 18 enable SAB broadcasting */
	u32 cfgnotifyqne:1; /* 19 enable Q not empty intr */
	u32 cfgcrid:1;      /* 20 critical rid config */
	u32 cpu_notify:8;   /* 28:21 */
	u32 rid:3;          /* 31:29 */

	/* register word 1 (bit 63:32) */
	u32 q7selarb:2;     /* 33:32 */
	u32 q7txallowed:1;  /* 34 */
	u32 q7reqvld:1;     /* 35 */
	u32 q7_sel:10;      /* 45:36 */
	u32 q6selarb:2;     /* 47:46 */
	u32 q6txallowed:1;  /* 48 */
	u32 q6reqvld:1;     /* 49 */
	u32 q6_sel:10;      /* 59:50 */
	u32 q5selarb:2;     /* 61:60 */
	u32 q5txallowed:1;  /* 62 */
	u32 q5reqvld:1;     /* 63 */

	/* register word 2 (bit 95:64) */
	u32 q5_sel:10;      /* 73:64 */
	u32 q4selarb:2;     /* 75:74 */
	u32 q4txallowed:1;  /* 76 */
	u32 q4reqvld:1;     /* 77 */
	u32 q4_sel:10;      /* 87:78 */
	u32 q3selarb:2;     /* 89:88 */
	u32 q3txallowed:1;  /* 90 */
	u32 q3reqvld:1;     /* 91 */
	u32 q3_selL:4;      /* 95:92 split 4/6 */

	/* register word 3 (bit 127:96) */
	u32 q3_selH:6;      /* 101:96 split 4/6 */
	u32 q2selarb:2;     /* 103:102 */
	u32 q2txallowed:1;  /* 104 */
	u32 q2reqvld:1;     /* 105 */
	u32 q2_sel:10;      /* 115:106 */
	u32 q1selarb:2;     /* 117:116 */
	u32 q1txallowed:1;  /* 118 */
	u32 q1reqvld:1;     /* 119 */
	u32 q1_selL:8;      /* 127:120 split 2/8 */

	/* register word 4 (bit 159:128) */
	u32 q1_selH:2;      /* 129:128 split 2/8 */
	u32 q0selarb:2;     /* 131:130 */
	u32 q0txallowed:1;  /* 132 */
	u32 q0reqvld:1;     /* 133 */
	u32 q0_sel:10;      /* 143:134 */
	u32 cfgselthrsh:3;  /* 146:144 associated threshold set */
	u32 cfgqtype:2;     /* 148:147 queue type */
	u32 resv1:11;       /* 159:149 */
#endif
} __packed;

union storm_qmtm_qstate {
	struct storm_qmtm_csr_qstate csr;
	struct storm_qmtm_pq_fp_qstate pq;
	struct storm_qmtm_pq_fp_qstate fp;
	struct storm_qmtm_vq_qstate vq;
} __packed;

#endif /* __XGENE_QMTM_STORM_H__ */
