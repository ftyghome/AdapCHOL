#pragma once
#include "common.h"
extern "C" {
#include "csparse/Include/cs.h"
}

typedef struct adap_cs_symbolic  /* symbolic Cholesky, LU, or QR analysis */
{
    csi *pinv;     /* inverse row perm. for QR, fill red. perm for Chol */
    csi *parent;   /* elimination tree for Cholesky and QR */
    csi *cp;       /* column pointers for Cholesky, row counts for QR */
    csi *post;
    cs *symp;
    cs *AT;
    double lnz;    /* # entries in L for LU or Cholesky; in V for QR */
} adap_css;

adap_css *adap_cs_schol(csi order, const cs *A);

csi *adap_cs_counts(const cs *A, const csi *parent, const csi *post, const cs *AT);

cs *adap_cs_spalloc_manual(csi m, csi n, csi nzmax, csi triplet, double *Ax);