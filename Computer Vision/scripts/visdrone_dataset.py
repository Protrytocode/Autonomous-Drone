import os
import shutil
import zipfile
from pathlib import Path
from PIL import Image
from ultralytics.utils.downloads import download

# 1. Base Paths
ROOT = Path(__file__).resolve().parent.parent
DATASET_DIR = ROOT / "human_dataset"

# 2. VisDrone URLs (Official Ultralytics asset mirrors)
URLS = {
    "train": "https://github.com/ultralytics/assets/releases/download/v0.0.0/VisDrone2019-DET-train.zip",
    "val": "https://github.com/ultralytics/assets/releases/download/v0.0.0/VisDrone2019-DET-val.zip"
}

def process_split(split_name, zip_url):
    print(f"\n--- Processing {split_name.upper()} split ---")
    zip_name = f"visdrone_{split_name}.zip"
    temp_extract_dir = ROOT / f"temp_visdrone_{split_name}"
    
    # Download archive
    print(f"Downloading {zip_name} (~1.5GB for train, ~35MB for val)...")
    download(zip_url, dir=ROOT)
    
    downloaded_zip = ROOT / Path(zip_url).name
    if downloaded_zip.exists() and downloaded_zip != (ROOT / zip_name):
        downloaded_zip.rename(ROOT / zip_name)

    # Extract archive
    print(f"Extracting {zip_name}...")
    with zipfile.ZipFile(ROOT / zip_name, 'r') as z:
        z.extractall(temp_extract_dir)

    # Locate unpacked files
    extracted_images = list(temp_extract_dir.glob("**/*.jpg"))
    print(f"Found {len(extracted_images)} raw images. Filtering and converting...")

    processed_count = 0
    for img_file in extracted_images:
        # VisDrone layout: images are sibling to annotations folder
        lbl_file = img_file.parent.parent / "annotations" / f"{img_file.stem}.txt"
        if not lbl_file.exists():
            continue

        valid_boxes = []
        with open(lbl_file, "r") as f:
            for line in f:
                parts = line.strip().split(',')
                # VisDrone class 1 = pedestrian, class 2 = people (sitting/lying/non-upright)
                if len(parts) >= 8 and parts[5] in ['1', '2']:
                    x, y, w, h = map(float, parts[:4])
                    if w <= 0 or h <= 0:
                        continue
                    
                    with Image.open(img_file) as im:
                        iw, ih = im.size
                        
                    # Calculate normalized YOLO format: class_id xc yc w h
                    xc = (x + w / 2.0) / iw
                    yc = (y + h / 2.0) / ih
                    nw = w / iw
                    nh = h / ih
                    
                    # Clamp values between 0.0 and 1.0
                    xc, yc = max(0.0, min(1.0, xc)), max(0.0, min(1.0, yc))
                    nw, nh = max(0.0, min(1.0, nw)), max(0.0, min(1.0, nh))

                    valid_boxes.append(f"0 {xc:.6f} {yc:.6f} {nw:.6f} {nh:.6f}\n")

        # Copy only images that have verified human targets
        if valid_boxes:
            dest_img = DATASET_DIR / split_name / "images" / img_file.name
            dest_lbl = DATASET_DIR / split_name / "labels" / f"{img_file.stem}.txt"
            
            shutil.copy(img_file, dest_img)
            with open(dest_lbl, "w") as out_f:
                out_f.writelines(valid_boxes)
            
            processed_count += 1

    print(f"Successfully processed {processed_count} human-positive images into {split_name}/")

    # Clean up temporary disk files
    if (ROOT / zip_name).exists():
        os.remove(ROOT / zip_name)
    if temp_extract_dir.exists():
        shutil.rmtree(temp_extract_dir)

def main():
    # Process both splits into your human_dataset structure
    process_split("train", URLS["train"])
    process_split("val", URLS["val"])
    print("\nVisDrone download, filtering, and placement complete!")

if __name__ == "__main__":
    main()