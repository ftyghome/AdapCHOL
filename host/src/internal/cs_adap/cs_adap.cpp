#include "internal/cs_adap/cs_adap.h"

extern "C" {
#include "csparse/Include/cs.h"
}

#define HEAD(k, j) (j)
#define NEXT(J) (-1)

adap_css *adap_cs_sfree(adap_css *S) {
    if (!S) return (nullptr);     /* do nothing if S already NULL */
    cs_free(S->pinv);
    cs_free(S->parent);
    cs_free(S->cp);
    cs_free(S->post);
    cs_free(S->symp);
    return ((adap_css *) cs_free(S));  /* free the css struct and return NULL */
}

adap_css *adap_cs_schol(csi order, const cs *A) {
    csi n, *c, *post, *P;
    cs *C;
    adap_css *S;
    if (!CS_CSC (A)) return (nullptr);        /* check inputs */
    n = A->n;
    S = (adap_css *) cs_calloc(1, sizeof(adap_css));       /* allocate result S */
    if (!S) return (nullptr);                 /* out of memory */
    P = cs_amd(order, A);                 /* P = amd(A+A'), or natural */
    S->pinv = cs_pinv(P, n);              /* find inverse permutation */
    cs_free(P);
    if (order && !S->pinv) return (adap_cs_sfree(S));
    C = cs_symperm(A, S->pinv, 1);        /* C = spones(triu(A(P,P))) */
    S->symp = C;
    S->AT = cs_transpose(C, 1);
    S->parent = cs_etree(C, 0);           /* find etree of C */
    post = cs_post(S->parent, n);         /* postorder the etree */
    S->post = post;
    c = adap_cs_counts(C, S->parent, post, S->AT); /* find column counts of chol(C) */
    S->cp = (int64_t *) cs_malloc(n + 1, sizeof(csi)); /* allocate result S->cp */
    S->lnz = cs_cumsum(S->cp, c, n); /* find column pointers for L */
    cs_free(c);
    return ((S->lnz >= 0) ? S : adap_cs_sfree(S));
}

csi *adap_cs_counts(const cs *A, const csi *parent, const csi *post, const cs *AT) {
    csi i, j, k, n, m, J, s, p, q, jleaf, *ATp, *ATi, *maxfirst, *prevleaf,
            *ancestor, *head = nullptr, *next = nullptr, *colcount, *w, *first, *delta;
    if (!CS_CSC (A) || !parent || !post) return (nullptr);    /* check inputs */
    m = A->m;
    n = A->n;
    s = 4 * n;
    delta = colcount = (int64_t *) cs_malloc(n, sizeof(csi));    /* allocate result */
    w = (int64_t *) cs_malloc(s, sizeof(csi));                   /* get workspace */
    if (!AT || !colcount || !w) return (cs_idone(colcount, nullptr, w, 0));
    ancestor = w;
    maxfirst = w + n;
    prevleaf = w + 2 * n;
    first = w + 3 * n;
    for (k = 0; k < s; k++) w[k] = -1;      /* clear workspace w [0..s-1] */
    for (k = 0; k < n; k++)                   /* find first [j] */
    {
        j = post[k];
        delta[j] = (first[j] == -1) ? 1 : 0;  /* delta[j]=1 if j is a leaf */
        for (; j != -1 && first[j] == -1; j = parent[j]) first[j] = k;
    }
    ATp = AT->p;
    ATi = AT->i;
    for (i = 0; i < n; i++) ancestor[i] = i; /* each node in its own set */
    for (k = 0; k < n; k++) {
        j = post[k];          /* j is the kth node in postordered etree */
        if (parent[j] != -1) delta[parent[j]]--;    /* j is not a root */
        for (J = HEAD (k, j); J != -1; J = NEXT (J))   /* J=j for LL'=A case */
        {
            for (p = ATp[J]; p < ATp[J + 1]; p++) {
                i = ATi[p];
                q = cs_leaf(i, j, first, maxfirst, prevleaf, ancestor, &jleaf);
                if (jleaf >= 1) delta[j]++;   /* A(i,j) is in skeleton */
                if (jleaf == 2) delta[q]--;   /* account for overlap in q */
            }
        }
        if (parent[j] != -1) ancestor[j] = parent[j];
    }
    for (j = 0; j < n; j++)           /* sum up delta's of each child */
    {
        if (parent[j] != -1) colcount[parent[j]] += colcount[j];
    }
    return (cs_idone(colcount, nullptr, w, 1));    /* success: free workspace */
}


cs *adap_cs_spalloc_manual(csi m, csi n, csi nzmax, csi triplet, double *Ax) {
    cs *A = (cs *) cs_calloc(1, sizeof(cs));    /* allocate the cs struct */
    if (!A) return (nullptr);                 /* out of memory */
    A->m = m;                              /* define dimensions and nzmax */
    A->n = n;
    A->nzmax = nzmax = CS_MAX (nzmax, 1);
    A->nz = triplet ? 0 : -1;              /* allocate triplet or comp.col */
    A->p = (csi *) cs_malloc(triplet ? nzmax : n + 1, sizeof(csi));
    A->i = (csi *) cs_malloc(nzmax, sizeof(csi));
    A->x = Ax;
    return ((!A->p || !A->i) ? cs_spfree(A) : A);
}
