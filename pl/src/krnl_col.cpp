#include "hls_stream.h"
#include "hls_math.h"
#include "hls_vector.h"
#include "hls_burst_maxi.h"
#include "ap_int.h"
#include "../include/krnl_col.h"
#include "../include/common_def.h"


void DescF_Splitter(const double *descF, int descFn, int descFSize,
                    hls::stream<double> &descF_First_Col, hls::stream<double> &descF_After_Col) {
    for (int i = 0; i < descFn; i++) {
#pragma HLS PIPELINE II=1
        descF_First_Col.write(descF[i]);
    }
    for (int i = descFn; i < descFSize; i++) {
#pragma HLS PIPELINE II=1
        descF_After_Col.write(descF[i]);
    }
}


void Sqrt_Div(hls::stream<double> &inF_First_Col, int Fn, hls::stream<double> &outF_First_Col) {
    if (Fn <= 0) return;
    double trig = hls::sqrt((inF_First_Col.read()));
    outF_First_Col.write(trig);
    Sqrt_Div_Loop:
    for (int i = 1; i < Fn; i++) {
#pragma HLS PIPELINE II=1
        outF_First_Col.write(inF_First_Col.read() / trig);
    }
}

void Gen_Update_Matrix(hls::stream<double> &inDescF_First_Col,
                       hls::stream<double> &outU_Pre, int descFn) {
    static double descF[MAX_NZ_IN_A_COL];
    Gen_Update_Matrix_Pre_Store_To_RAM_Loop:
    for (int i = 0; i < descFn; i++)
#pragma HLS PIPELINE II=1
            descF[i] = inDescF_First_Col.read();
    Gen_Update_Matrix_Pre_Write_OutU_Outer_Loop:
    for (int i = 1; i < descFn; i++) {
#pragma HLS PIPELINE II=1
        Gen_Update_Matrix_Pre_Write_OutU_Inner_Loop:
        for (int j = i; j < descFn; j++)
#pragma HLS PIPELINE II=1
                outU_Pre.write(descF[i] * descF[j]);
    }

}

void Read_Parent_F(double *parF, hls::stream<double> &parFStream, int parFSize) {
    for (int i = 0; i < (parFSize + 7) / 8 * 8; i++) {
#pragma HLS PIPELINE II=1
        parFStream.write(parF[i]);
    }
}

void Read_P(const bool *P, hls::stream<bool> &pStream, int parFSize) {
    for (int i = 0; i < (parFSize + 7) / 8 * 8; i++) {
#pragma HLS PIPELINE II=1
        pStream.write(P[i]);
    }
}

void
U_Scale(hls::stream<double> &inU, hls::stream<bool> &inP, hls::stream<double> &outU,
        hls::stream<double> &inDescF_After_Col, int parFSize) {
    for (int i = 0; i < parFSize; i++) {
#pragma HLS PIPELINE II=1
        outU.write(inP.read() ? inDescF_After_Col.read() - inU.read() : 0);
    }
    for (int i = parFSize; i < (parFSize + 7) / 8 * 8; i++) {
#pragma HLS PIPELINE II=1
        inP.read();
        outU.write(0);
    }
}

void
Gen_New_Parent_F(hls::stream<double> &inU, hls::stream<double> &inParF,
                 hls::stream<double> &outParF,
                 int parFSize) {
    for (int i = 0; i < (parFSize + 7) / 8 * 8; i++) {
#pragma HLS PIPELINE II=1
        outParF.write(inU.read() + inParF.read());
    }
}

void Write_Parent_F(hls::stream<double> &inParF, double *outParF, int parFSize) {
    Write_Parent_F_Scale_Loop:
    for (int i = 0; i < (parFSize + 7) / 8 * 8; i++) {
        outParF[i] = inParF.read();
    }
}

void krnl_proc_col(double *descF, bool *P, double *parF,
                   int descFn, int parFn) {
#pragma HLS INTERFACE mode=m_axi port = descF bundle=gmem0 depth=50000 latency=21
#pragma HLS INTERFACE mode=m_axi port = P bundle=gmem1 depth=50000 latency=21
#pragma HLS INTERFACE mode=m_axi port = parF bundle=gmem2 depth=50000 latency=21


    hls::stream<double> descF_First_Col("descF_First_Col");
    hls::stream<double> descF_First_Col_Processed("descF_First_Col_Processed");
    hls::stream<double> descF_After_Col("descF_After_Col");
    hls::stream<double> U("U");
    hls::stream<double> U_Scaled("U_Scaled");
    hls::stream<double> newParF("newParF");
    hls::stream<bool> inP("P");
    hls::stream<double> parentF("parentF");

#pragma HLS stream variable=descF_First_Col type=FIFO depth=16
#pragma HLS stream variable=descF_First_Col_Processed type=FIFO depth=16
#pragma HLS stream variable=descF_After_Col type=FIFO depth=16
#pragma HLS stream variable=U type=FIFO depth=16
#pragma HLS stream variable=inP type=FIFO depth=4
#pragma HLS stream variable=U_Scaled type=FIFO depth=16
#pragma HLS stream variable=parentF type=FIFO depth=16
#pragma HLS stream variable=newParF type=FIFO depth=16

    int parFSize = (1 + parFn) * parFn / 2;
    int descFSize = (1 + descFn) * descFn / 2;

#pragma HLS dataflow
    DescF_Splitter(descF, descFn, descFSize, descF_First_Col, descF_After_Col);
    Read_Parent_F(parF, parentF, parFSize);
    Read_P(P, inP, parFSize);
    Sqrt_Div(descF_First_Col, descFn, descF_First_Col_Processed);
    Gen_Update_Matrix(descF_First_Col_Processed, U, descFn);
    U_Scale(U, inP, U_Scaled, descF_After_Col, parFSize);
    Gen_New_Parent_F(U_Scaled, parentF, newParF, parFSize);
    Write_Parent_F(newParF, parF, parFSize);
}
