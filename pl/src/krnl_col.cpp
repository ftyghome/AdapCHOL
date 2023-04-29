#include "hls_stream.h"
#include "hls_math.h"
#include "ap_int.h"
#include "../include/krnl_col.h"
#include "../include/common_def.h"

void DescF_Splitter(const double *descF, const double *inL, int descLOffset, int descFn,
                    hls::stream<double> &descF_First_Col, hls::stream<double> &descF_After_Col,
                    unsigned char taskCtrl) {
    const double *L = inL + descLOffset;
    bool isLeaf = taskCtrl & 0b1;
    int descFSize = (1 + descFn) * descFn / 2;
    if (isLeaf) {
        for (int i = 0; i < descFn; i++) {
#pragma HLS PIPELINE II=1
            descF_First_Col.write(L[i]);
        }
    } else {
        for (int i = 0; i < descFn; i++) {
#pragma HLS PIPELINE II=1
            descF_First_Col.write(descF[i] + L[i]);
        }
        for (int i = descFn; i < descFSize; i++) {
#pragma HLS PIPELINE II=1
            descF_After_Col.write(descF[i]);
        }
    }
}


void Sqrt_Div(hls::stream<double> &inF_First_Col, int Fn, hls::stream<double> &outF_First_Col,
              hls::stream<double> &outL) {
    if (Fn <= 0) return;
    double trig = hls::sqrt((inF_First_Col.read()));
    outF_First_Col.write(trig);
    outL.write(trig);
    Sqrt_Div_Loop:
    for (int i = 1; i < Fn; i++) {
#pragma HLS PIPELINE II=1
        double elem = inF_First_Col.read() / trig;
        outF_First_Col.write(elem);
        outL.write(elem);
    }
}

void Gen_Update_Elem_Dispatch(hls::stream<double> &inDescF_First_Col,
                              hls::stream<double> &outI, hls::stream<double> &outJ, int descFn,
                              unsigned char taskCtrl) {
    bool isLeaf = taskCtrl & 0b1;
    static double descF[MAX_NZ_IN_A_COL];
    Gen_Update_Elem_Dispatch_Store_To_RAM_Loop:
    for (int i = 0; i < descFn; i++)
#pragma HLS PIPELINE II=1
            descF[i] = inDescF_First_Col.read();

    if (isLeaf) {
        Gen_Update_Elem_Dispatch_Write_OutU_Outer_Leaf_Loop:
        for (int i = 1; i < descFn; i++) {
#pragma HLS PIPELINE II=1
            double iElem = descF[i];
            Gen_Update_Elem_Dispatch_Write_OutU_Inner_Leaf_Loop:
            for (int j = i; j < descFn; j++) {
#pragma HLS PIPELINE II=1
                outI.write(iElem);
                outJ.write(descF[j]);
            }
        }

    } else {
        Gen_Update_Elem_Dispatch_Write_OutU_Outer_Loop:
        for (int i = 1; i < descFn; i++) {
#pragma HLS PIPELINE II=1
            double iElem = descF[i];
            Gen_Update_Elem_Dispatch_Write_OutU_Inner_Loop:
            for (int j = i; j < descFn; j++) {
#pragma HLS PIPELINE II=1
                outI.write(iElem);
                outJ.write(descF[j]);
            }
        }
    }
}

void Gen_Update_Matrix(hls::stream<double> &inI, hls::stream<double> &inJ, hls::stream<double> &inDescF_After_Col,
                       hls::stream<double> &outU_Pre, int descFn, unsigned char taskCtrl) {
    bool isLeaf = taskCtrl & 0b1;
    int elems = (descFn) * (descFn - 1) / 2;
    if (isLeaf) {
        for (int i = 0; i < elems; i++) {
#pragma HLS PIPELINE II=1
            outU_Pre.write(-inI.read() * inJ.read());
        }

    } else {
        for (int i = 0; i < elems; i++) {
#pragma HLS PIPELINE II=1
            outU_Pre.write(inDescF_After_Col.read() - inI.read() * inJ.read());
        }
    }
}

void Read_Parent_F(const double *parF, hls::stream<double> &parFStream, int parFn, unsigned char taskCtrl) {
    bool isNewF = taskCtrl & 0b10;
    int parFSize = (1 + parFn) * parFn / 2;
    if (!isNewF) {
        for (int i = 0; i < (parFSize + 7) / 8 * 8; i++) {
#pragma HLS PIPELINE II=1
            parFStream.write(parF[i]);
        }
    }
}

void Read_P(const bool *inP, hls::stream<bool> &outP, int parFn) {
    static bool P[MAX_NZ_IN_A_COL];
    for (int i = 0; i < parFn; i++) {
#pragma HLS PIPELINE II=1
        P[i] = inP[i];
    }
    for (int i = 0; i < parFn; i++) {
        for (int j = i; j < parFn; j++) {
#pragma HLS PIPELINE II=1
            outP.write(P[i] && P[j]);
        }
    }
    int parFSize = (1 + parFn) * parFn / 2;
    for (int i = parFSize; i < (parFSize + 7) / 8 * 8; i++) {
        outP.write(false);
    }

}

void Write_Parent_F(hls::stream<double> &inU, hls::stream<double> &inParF, hls::stream<bool> &inP, double *outParF,
                    int parFn, unsigned char taskCtrl) {
    bool isNewF = taskCtrl & 0b10;
    int parFSize = (1 + parFn) * parFn / 2;
    if (!isNewF) {
        Write_Parent_F_OldF_Loop:
        for (int i = 0; i < (parFSize + 7) / 8 * 8; i++) {
#pragma HLS PIPELINE II=1
            outParF[i] = inP.read() ? inParF.read() + inU.read() : inParF.read();
        }
    } else {
        Write_Parent_F_NewF_Loop:
        for (int i = 0; i < (parFSize + 7) / 8 * 8; i++) {
#pragma HLS PIPELINE II=1
            outParF[i] = inP.read() ? inU.read() : 0;
        }
    }

}

void Write_L(hls::stream<double> &inLStream, double *inL, int descLOffset, int descFn) {
    double *L = inL + descLOffset;
    for (int i = 0; i < descFn; i++) {
#pragma HLS PIPELINE II=1
        L[i] = inLStream.read();
    }
}

void krnl_proc_col(double *descF, bool *P, double *parF, double *L, int descLOffset,
                   int descFn, int parFn, unsigned char taskCtrl) {
#pragma HLS INTERFACE mode=m_axi port = descF bundle=gmem0 depth=20000
#pragma HLS INTERFACE mode=m_axi port = P bundle=gmem1 depth=20000
#pragma HLS INTERFACE mode=m_axi port = parF bundle=gmem2 depth=20000
#pragma HLS INTERFACE mode=m_axi port = L bundle=gmem3 depth=20000


    hls::stream<double> descF_First_Col("descF_First_Col");
    hls::stream<double> descF_First_Col_Processed("descF_First_Col_Processed");
    hls::stream<double> descF_After_Col("descF_After_Col");
    hls::stream<double> U("U");
    hls::stream<bool> inP("P");
    hls::stream<double> iStream("iStream");
    hls::stream<double> jStream("jStream");
    hls::stream<double> outL("outL");
    hls::stream<double> parentF("parentF");

#pragma HLS stream variable=descF_First_Col type=FIFO depth=2048
#pragma HLS stream variable=descF_First_Col_Processed type=FIFO depth=2048
#pragma HLS stream variable=descF_After_Col type=FIFO depth=2048
#pragma HLS stream variable=U type=FIFO depth=2048
#pragma HLS stream variable=iStream type=FIFO depth=2048
#pragma HLS stream variable=jStream type=FIFO depth=2048
#pragma HLS stream variable=inP type=FIFO depth=2048
#pragma HLS stream variable=outL type=FIFO depth=2048
#pragma HLS stream variable=parentF type=FIFO depth=2048


#pragma HLS dataflow
    DescF_Splitter(descF, L, descLOffset, descFn, descF_First_Col, descF_After_Col, taskCtrl);
    Read_Parent_F(parF, parentF, parFn, taskCtrl);
    Read_P(P, inP, parFn);
    Sqrt_Div(descF_First_Col, descFn, descF_First_Col_Processed, outL);
    Gen_Update_Elem_Dispatch(descF_First_Col_Processed, iStream, jStream, descFn, taskCtrl);
    Write_L(outL, L, descLOffset, descFn);
    Gen_Update_Matrix(iStream, jStream, descF_After_Col, U, descFn, taskCtrl);
    Write_Parent_F(U, parentF, inP, parF, parFn, taskCtrl);
}
