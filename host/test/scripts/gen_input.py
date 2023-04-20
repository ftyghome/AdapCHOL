import os
import subprocess
import pathlib
from subprocess import Popen, PIPE, STDOUT
import filecmp
from joblib import Parallel, delayed
import argparse

output_intermediate = True

blacklists = ['bibd_81_2', 'mhd1280b']


def gen_input(mat_path: pathlib.Path):
    mat_name = mat_path.stem
    mat_input = mat_path.parent / (mat_name + "_input.txt")

    input_str = ''
    with open(mat_path, "r") as f:
        line = f.readline()
        n, m, nonzero = -1, -1, -1
        while line:
            if line[0] != '%':
                if n == -1:
                    n, m, nonzero = [int(x) for x in line.strip().split(" ")]
                else:
                    i, j, x = line.strip().split(" ")
                    i = int(i) - 1
                    j = int(j) - 1
                    x = float(x)
                    input_str += str(i) + ' ' + str(j) + ' ' + str(x) + '\n'
                    if i != j:
                        input_str += str(j) + ' ' + str(i) + ' ' + str(x) + '\n'
            line = f.readline()

    with open(mat_input, "w") as f:
        f.write(input_str)
    # print(output)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--mat_path")
    parser.add_argument("--max_jobs")
    args = parser.parse_args()
    p = pathlib.Path(args.mat_path).glob('**/*')
    files = [x for x in p if x.is_dir()]
    files = sorted(files)
    print("Number of files:", len(files))

    test_result = Parallel(n_jobs=int(args.max_jobs))(
        delayed(gen_input)(pathlib.Path(file / (file.name + ".mtx"))) for file in files)
