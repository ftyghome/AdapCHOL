import os
import subprocess
import pathlib
from subprocess import Popen, PIPE, STDOUT
import filecmp
from joblib import Parallel, delayed
import argparse

output_intermediate = True

blacklists = ['bibd_81_2', 'mhd1280b','plat1919']


def load_result(path: pathlib.Path):
    current = ''
    p, i, x = [], [], []
    with open(path) as f:
        lines = f.readlines()
    n = int(lines[0].strip())
    for line in lines[1:]:
        if line == "P:---\n":
            current = 'P'
        elif line == 'I:---\n':
            current = 'I'
        elif line == 'X:---\n':
            current = 'X'
        else:
            if line == '\n':
                continue
            elems = line.strip().split(" ")
            if current == 'P':
                p += [int(i) for i in elems]
            elif current == 'I':
                i += [int(i) for i in elems]
            elif current == 'X':
                x += [float(i) for i in elems]

    return n, p, i, x


def load_result_sol(path: pathlib.Path):
    ret = []
    with open(path) as f:
        lines = f.readlines()
    for line in lines:
        ret.append(float(line.strip()))
    return ret


def compare(a_path: pathlib.Path, b_path: pathlib.Path):
    try:
        an, ap, ai, ax = load_result(a_path)
        bn, bp, bi, bx = load_result(b_path)
    except Exception as e:
        print("Exception occurred comparing", a_path, b_path, e)
        exit(0)
    if an != bn or ap != bp or ai != bi or len(ax) != len(bx):
        return False
    length = len(ax)
    for i in range(length):
        if abs(bx[i]) > 1e-1:
            if abs((ax[i] - bx[i]) / bx[i]) > 1e-2:
                return False
        else:
            if abs(ax[i] - bx[i]) > 1e-2:
                return False

    return True


def compare_sol(a_path: pathlib.Path, b_path: pathlib.Path):
    try:
        ar = load_result_sol(a_path)
        br = load_result_sol(b_path)
    except Exception as e:
        print("Exception occurred comparing", a_path, b_path, e)
        exit(0)
    if len(ar) != len(br):
        return False
    length = len(ar)
    for i in range(length):
        if abs(br[i]) > 1e-1:
            if abs((ar[i] - br[i]) / br[i]) > 1e-2:
                return False
        else:
            if abs(ar[i] - br[i]) > 1e-2:
                return False

    return True


def test(mat_path: pathlib.Path):
    mat_name = mat_path.stem
    result_dir = pathlib.Path(args.testing_path + "/result") / mat_name
    result_dir.mkdir(exist_ok=True, parents=True)
    mat_input = result_dir / (mat_name + "_input.txt")
    prog_result = result_dir / (mat_name + "_result.txt")
    csparse_result = result_dir / (mat_name + "_cs_result.txt")
    frontal_result = result_dir / (mat_name + "_frontal.txt") if output_intermediate else "/dev/null"
    update_result = result_dir / (mat_name + "_update.txt") if output_intermediate else "/dev/null"
    p_result = result_dir / (mat_name + "_p.txt") if output_intermediate else "/dev/null"

    with open(mat_path.parent / (mat_name + "_input.txt"), "r") as input:
        p = Popen([args.testing_path + '/build/test_host', prog_result, csparse_result, frontal_result, update_result,
                   p_result],
                  stdout=PIPE, stdin=input, stderr=PIPE)
    output = p.communicate()
    result = compare(prog_result, csparse_result)
    if result and prog_result.stat().st_size > 0 and csparse_result.stat().st_size > 0:
        print("✓ Test passed for", mat_path)
    else:
        print("⬛ TEST FAILED for", mat_path)
    return result


def test_sol(mat_path: pathlib.Path):
    mat_name = mat_path.stem
    result_dir = pathlib.Path(args.testing_path + "/result") / mat_name
    result_dir.mkdir(exist_ok=True, parents=True)
    mat_input = result_dir / (mat_name + "_input.txt")
    prog_result = result_dir / (mat_name + "_result_sol.txt")
    csparse_result = result_dir / (mat_name + "_cs_result_sol.txt")
    frontal_result = result_dir / (mat_name + "_frontal.txt") if output_intermediate else "/dev/null"
    update_result = result_dir / (mat_name + "_update.txt") if output_intermediate else "/dev/null"
    p_result = result_dir / (mat_name + "_p.txt") if output_intermediate else "/dev/null"

    with open(mat_path.parent / (mat_name + "_input.txt"), "r") as input:
        p = Popen(
            [args.testing_path + '/build/testsol_host', prog_result, csparse_result, frontal_result, update_result,
             p_result],
            stdout=PIPE, stdin=input, stderr=PIPE)
    output = p.communicate()
    result = compare_sol(prog_result, csparse_result)
    if result and prog_result.stat().st_size > 0 and csparse_result.stat().st_size > 0:
        print("✓ Test passed for", mat_path)
    else:
        print("⬛ TEST FAILED for", mat_path)
    return result


def filter_files(files: list, max_n: int):
    ret = []
    for file in files:
        if str(file.stem) in blacklists:
            continue
        with open(file / (file.stem + ".mtx"), "r") as f:
            line = f.readline()
            n, m, nonzero = -1, -1, -1
            while line:
                if line[0] != '%':
                    if n == -1:
                        n, m, nonzero = [int(x) for x in line.strip().split(" ")]
                    break
                line = f.readline()
            if n <= max_n:
                ret.append(file)

    return ret


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--testing_path")
    parser.add_argument("--source_path")
    parser.add_argument("--mat_path")
    parser.add_argument("--filter_max_n")
    parser.add_argument("--max_jobs")
    args = parser.parse_args()

    pathlib.Path.mkdir(pathlib.Path(args.testing_path + "/build"), parents=True, exist_ok=True)
    pathlib.Path.mkdir(pathlib.Path(args.testing_path + "/result"), parents=True, exist_ok=True)
    subprocess.run("cd " + args.testing_path + "/build && cmake " + args.source_path + " && make -j12", shell=True)
    p = pathlib.Path(args.mat_path).glob('**/*')
    files = [x for x in p if x.is_dir()]
    files = filter_files(files, max_n=int(args.filter_max_n))
    files = sorted(files)
    print("Number of files:", len(files))

    # for file in files:
    #     test(pathlib.Path(file / (file.name + ".mtx")))

    print("Factorization Test Start ---")

    test_result = Parallel(n_jobs=int(args.max_jobs))(
        delayed(test)(pathlib.Path(file / (file.name + ".mtx"))) for file in files)

    print("Solve Test Start ---")

    test_result = Parallel(n_jobs=int(args.max_jobs))(
        delayed(test_sol)(pathlib.Path(file / (file.name + ".mtx"))) for file in files)

    # test(pathlib.Path("mats/nd3k/nd3k.mtx"))
