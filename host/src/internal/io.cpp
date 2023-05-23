#include "internal/io.h"
#include <iomanip>

namespace AdapChol {

    template<typename T>
    void printDenseTrig(const T *F, int64_t size, std::ostream &stream) {
        auto *fullF = new double[size * size];
        memset(fullF, 0, sizeof(double) * (size * size));

        int pos = 0;
        for (int col = 0; col < size; col++) {
            for (int row = col; row < size; row++) {
                fullF[row * size + col] = F[pos++];
            }
        }
        stream << std::fixed << std::setprecision(2);
        for (int row = 0; row < size; row++) {
            for (int col = 0; col < size; col++) {
                if (col <= row) {
                    stream << fullF[row * size + col] << '\t';
                } else {
                    stream << "-\t";
                }

            }
            stream << '\n';
        }
    }

    template void printDenseTrig<bool>(const bool *F, int64_t size, std::ostream &stream);

    template void printDenseTrig<double>(const double *F, int64_t size, std::ostream &stream);

    void printCS(const cs *mat) {
        const int64_t n = mat->n, m = mat->m;
        auto *nonzero = new bool[n * m];
        auto *val = new double[n * m];
        memset(nonzero, 0, sizeof(bool) * n * m);
        for (int col = 0; col < n; col++) {
            int64_t entryBegin = mat->p[col], entryEnd = mat->p[col + 1];
            for (int row = 0; row < m; row++) {
                for (int64_t entry = entryBegin; entry < entryEnd; entry++) {
                    if (mat->i[entry] == row) {
                        nonzero[row * n + col] = true;
                        val[row * n + col] = mat->x[entry];
                        break;
                    }
                }
            }
        }
        std::cout << std::setprecision(2);
        for (int row = 0; row < m; row++) {
            for (int col = 0; col < n; col++) {
                if (nonzero[row * n + col]) {
                    std::cout << std::setw(10) << val[row * n + col];
                } else std::cout << std::setw(10) << "-";
            }
            std::cout << '\n';
        }
    }


    void dumpFormalResult(std::ofstream &stream, cs *mat) {
        csi n = mat->n;
        stream << n << '\n';
        stream << "P:---\n";
        for (int i = 0; i <= n; i++) {
            stream << mat->p[i] << " ";
            if ((i + 1) % 10 == 0) stream << '\n';
        }
        stream << "\nI:---\n";
        for (int i = 0; i < mat->p[n]; i++) {
            stream << mat->i[i] << ' ';
            if ((i + 1) % 10 == 0) stream << '\n';
        }
        stream << "\nX:---\n";
        for (int i = 0; i < mat->p[n]; i++) {
            stream << mat->x[i] << ' ';
            if ((i + 1) % 10 == 0) stream << '\n';
        }
    }
}
