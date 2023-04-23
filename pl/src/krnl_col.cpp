#include "hls_stream.h"
#include "hls_math.h"
#include "ap_int.h"
#include "../include/krnl_col.h"
#include "../include/common_def.h"

void DescF_Splitter(const double *descF, int64_t descFn,
                    hls::stream<double> &descF_First_Col, hls::stream<double> &descF_After_Col) {
    int64_t descFSize = (1 + descFn) * descFn / 2;
    for (int64_t i = 0; i < descFn; i++){
#pragma HLS PIPELINE II=1
        descF_First_Col.write(descF[i]);
    }
    for (int64_t i = descFn; i < descFSize; i++) {
#pragma HLS PIPELINE II=1
        descF_After_Col.write(descF[i]);
    }

}


void Sqrt_Div(hls::stream<double> &inF_First_Col, int64_t Fn, hls::stream<double> &outF_First_Col) {
    if (Fn <= 0) return;
    double trig = hls::sqrt((inF_First_Col.read()));
    outF_First_Col.write(trig);
    Sqrt_Div_Loop:
    for (int i = 1; i < Fn; i++) {
#pragma HLS PIPELINE II=1
        outF_First_Col.write(inF_First_Col.read() / trig);
    }
}

void Gen_Update_Matrix(hls::stream<double> &inDescF_First_Col, hls::stream<double> &inDescF_After_Col,
                       hls::stream<double> &outU_Pre, int64_t descFn) {
    static double descF[MAX_NZ_IN_A_COL];
    Gen_Update_Matrix_Pre_Store_To_RAM_Loop:
    for (int i = 0; i < descFn; i++)
        descF[i] = inDescF_First_Col.read();
    Gen_Update_Matrix_Pre_Write_OutU_Outer_Loop:
    for (int i = 1; i < descFn; i++) {
        Gen_Update_Matrix_Pre_Write_OutU_Inner_Loop:
        for (int j = i; j < descFn; j++)
#pragma HLS PIPELINE II=1
                outU_Pre.write(inDescF_After_Col.read() - descF[i] * descF[j]);
    }

}

void Write_Parent_F(hls::stream<double> &inU, const bool *inP, double *parF, int64_t parFn) {
    int64_t parFSize = (1 + parFn) * parFn / 2;
    Write_Parent_F_Loop:
    for (int i = 0; i < parFSize; i++) {
#pragma HLS PIPELINE II=1
        if (inP[i]) {
            parF[i] = parF[i] + inU.read();
        }
    }
}

void krnl_proc_col(double *descF, bool *P, double *parF,
                   int64_t descFn, int64_t parFn) {
#pragma HLS INTERFACE mode=m_axi port = descF bundle=gmem0 depth=100
#pragma HLS INTERFACE mode=m_axi port = P bundle=gmem1 depth=100
#pragma HLS INTERFACE mode=m_axi port = parF bundle=gmem2 depth=100


    hls::stream<double> descF_First_Col("descF_First_Col");
    hls::stream<double> descF_First_Col_Processed("descF_First_Col_Processed");
    hls::stream<double> descF_After_Col("descF_After_Col");
    hls::stream<double> U("U");

#pragma HLS stream variable=descF_First_Col type=FIFO depth=100
#pragma HLS stream variable=descF_First_Col_Processed type=FIFO depth=100
#pragma HLS stream variable=descF_After_Col type=FIFO depth=100
#pragma HLS stream variable=U type=FIFO depth=100

#pragma HLS dataflow
    DescF_Splitter(descF, descFn, descF_First_Col, descF_After_Col);
    Sqrt_Div(descF_First_Col, descFn, descF_First_Col_Processed);
    Gen_Update_Matrix(descF_First_Col_Processed, descF_After_Col, U, descFn);
    Write_Parent_F(U, P, parF, parFn);
}
