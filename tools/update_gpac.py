#!/usr/bin/env python3
import argparse
import os
import shutil
from pathlib import Path


def find_gpac_filepath(base_search_dir: Path, filename) -> Path:
    paths = list(base_search_dir.rglob(filename))
    assert len(paths) == 1, f"paths: {paths}, filename: {filename}"
    return paths[0]


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("gpac", help="path to gpac", type=Path)
    parser.add_argument("dest", help="path to vendored gpac to be updated", type=Path)
    args = parser.parse_args()
    for root, dirs, files in os.walk(args.dest):
        for filename in files:
            filepath: Path = root / Path(filename)
            if filepath.suffix == ".c":
                base_dir = args.gpac / "src"
            elif filepath.suffix == ".h":
                base_dir = args.gpac / "include"
            else:
                continue
            path_to_copy_from = find_gpac_filepath(base_dir, filepath.name)
            print(f"Copying from {path_to_copy_from} to {filepath}")
            shutil.copy(path_to_copy_from, filepath)


if __name__ == "__main__":
    main()
