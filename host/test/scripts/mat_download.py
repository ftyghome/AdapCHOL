import ssgetpy
from joblib import Parallel, delayed


def download_process(matrix):
    matrix.download(extract=True, destpath="mats")


if __name__ == "__main__":
    results = ssgetpy.search(isspd=True, limit=1e5)
    print("Downloading", len(results),"matrices")
    Parallel(n_jobs=64)(delayed(download_process)(i) for i in results)
