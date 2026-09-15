import os
import shutil
import time
from pathlib import Path
import requests
from PIL import Image
from duckduckgo_search import DDGS

# 1. Paths
ROOT = Path(__file__).resolve().parent.parent
TRAIN_IMG = ROOT / "human_dataset" / "train" / "images"
TRAIN_LBL = ROOT / "human_dataset" / "train" / "labels"
VAL_IMG = ROOT / "human_dataset" / "val" / "images"
VAL_LBL = ROOT / "human_dataset" / "val" / "labels"

TEMP_SCRAPE = ROOT / "temp_scraped_negatives"
TEMP_SCRAPE.mkdir(exist_ok=True)

# 2. Targeted search queries that trigger false positives in SAR models
SEARCH_QUERIES = [
    "clothesline hanging laundry outdoors",
    "aerial view clothes drying line",
    "farm scarecrow in field",
    "outdoor mannequin display clothes",
    "pile of blankets clothes outdoor"
]

IMAGES_PER_QUERY = 30  # ~150 total negative images

print("1. Scraping targeted false-alarm candidates...")
downloaded_images = []

with DDGS() as ddgs:
    for query in SEARCH_QUERIES:
        print(f"   Searching: '{query}'...")
        results = list(ddgs.images(query, max_results=IMAGES_PER_QUERY))
        time.sleep(1.0)  # Gentle rate limiting
        
        for idx, item in enumerate(results):
            img_url = item.get("image")
            if not img_url:
                continue
            
            clean_q = query.replace(" ", "_")[:12]
            target_path = TEMP_SCRAPE / f"{clean_q}_{idx}_{int(time.time())}.jpg"
            
            try:
                res = requests.get(img_url, timeout=5, headers={"User-Agent": "Mozilla/5.0"})
                if res.status_code == 200:
                    with open(target_path, "wb") as f:
                        f.write(res.content)
                    
                    # Validate that the file is a healthy, openable image
                    with Image.open(target_path) as im:
                        im.verify()
                    downloaded_images.append(target_path)
            except Exception:
                if target_path.exists():
                    os.remove(target_path)

print(f"2. Successfully downloaded and verified {len(downloaded_images)} clean images.")

# 3. Split 80% train / 20% val
split_idx = int(0.8 * len(downloaded_images))
train_items = downloaded_images[:split_idx]
val_items = downloaded_images[split_idx:]

def ingest(image_list, dest_img_dir, dest_lbl_dir, prefix="false_alarm_"):
    added = 0
    for idx, img_path in enumerate(image_list):
        stem = f"{prefix}{img_path.stem}"
        dest_img = dest_img_dir / f"{stem}.jpg"
        dest_lbl = dest_lbl_dir / f"{stem}.txt"
        
        # Save as standard RGB JPEG
        try:
            with Image.open(img_path) as im:
                rgb_im = im.convert("RGB")
                rgb_im.save(dest_img, "JPEG")
            
            # Create the 0-byte label file
            dest_lbl.touch()
            added += 1
        except Exception:
            continue
    return added

n_train = ingest(train_items, TRAIN_IMG, TRAIN_LBL)
n_val = ingest(val_items, VAL_IMG, VAL_LBL)

# 4. Clean up temporary directory
shutil.rmtree(TEMP_SCRAPE)

print("\n--- Ingestion Complete ---")
print(f"Added {n_train} false-positive suppression images to human_dataset/train")
print(f"Added {n_val} false-positive suppression images to human_dataset/val")
print("All matching 0-byte text labels created successfully.")