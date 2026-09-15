import os
import shutil
import zipfile
from pathlib import Path

# 1. Setup Project Paths
ROOT = Path(__file__).resolve().parent.parent
ARCHIVE = ROOT / "archive.zip"
HUMAN_DATASET = ROOT / "human_dataset"

TRAIN_IMG = HUMAN_DATASET / "train" / "images"
TRAIN_LBL = HUMAN_DATASET / "train" / "labels"
VAL_IMG = HUMAN_DATASET / "val" / "images"
VAL_LBL = HUMAN_DATASET / "val" / "labels"

TEMP_DIR = ROOT / "_sard_extracted"
IMAGE_EXTS = {".jpg", ".jpeg", ".png", ".bmp", ".webp"}

for folder in [TRAIN_IMG, TRAIN_LBL, VAL_IMG, VAL_LBL]:
    folder.mkdir(parents=True, exist_ok=True)

if not ARCHIVE.exists():
    raise FileNotFoundError(f"Missing archive: {ARCHIVE}")

# 2. Unzip Archive
print("1. Extracting SARD archive...")
if TEMP_DIR.exists():
    shutil.rmtree(TEMP_DIR)
TEMP_DIR.mkdir(parents=True)

with zipfile.ZipFile(ARCHIVE, "r") as z:
    z.extractall(TEMP_DIR)

# 3. Locate Folders (handles 'val' or 'valid')
def locate_dir(root: Path, folder_name: str, split_aliases: list[str]):
    matches = [
        p for p in root.rglob(folder_name)
        if p.is_dir() and any(alias in p.parts for alias in split_aliases)
    ]
    if not matches:
        raise FileNotFoundError(f"Cannot find {folder_name} for splits {split_aliases}")
    return matches[0]

SARD_TRAIN_IMG = locate_dir(TEMP_DIR, "images", ["train"])
SARD_TRAIN_LBL = locate_dir(TEMP_DIR, "labels", ["train"])
SARD_VAL_IMG = locate_dir(TEMP_DIR, "images", ["val", "valid"])
SARD_VAL_LBL = locate_dir(TEMP_DIR, "labels", ["val", "valid"])

print(f"Detected train source: {SARD_TRAIN_IMG}")
print(f"Detected val source:   {SARD_VAL_IMG}")

# 4. Ingest and Pair
def ingest(src_img_dir: Path, src_lbl_dir: Path, dest_img_dir: Path, dest_lbl_dir: Path, prefix="sard_"):
    added_imgs = 0
    added_lbls = 0

    for img_path in src_img_dir.iterdir():
        if not img_path.is_file() or img_path.suffix.lower() not in IMAGE_EXTS:
            continue

        lbl_path = src_lbl_dir / f"{img_path.stem}.txt"
        
        # Only ingest if matching label exists (positive or 0-byte negative)
        if lbl_path.exists():
            new_stem = f"{prefix}{img_path.stem}"
            shutil.copy2(img_path, dest_img_dir / f"{new_stem}{img_path.suffix.lower()}")
            shutil.copy2(lbl_path, dest_lbl_dir / f"{new_stem}.txt")
            added_imgs += 1
            added_lbls += 1

    return added_imgs, added_lbls

print("\n2. Ingesting training and validation pairs...")
tr_img, tr_lbl = ingest(SARD_TRAIN_IMG, SARD_TRAIN_LBL, TRAIN_IMG, TRAIN_LBL)
vl_img, vl_lbl = ingest(SARD_VAL_IMG, SARD_VAL_LBL, VAL_IMG, VAL_LBL)

# 5. Cleanup
shutil.rmtree(TEMP_DIR)

print(f"\nIngested Training Pairs  : {tr_img}")
print(f"Ingested Validation Pairs: {vl_img}")
print("Temporary extraction files cleaned up.")