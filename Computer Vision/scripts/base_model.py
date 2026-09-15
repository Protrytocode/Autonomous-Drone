from os import makedirs
from ultralytics import YOLO

model1 = YOLO("yolo11s.pt")
makedirs("results", exist_ok=True)

image_list = ["datasets/test/image_1.avif", "datasets/test/image_2.avif", "datasets/test/image_3.webp", "datasets/test/image_4.jpg", "datasets/test/image_5.jpg", "datasets/test/image_6.jpg", "datasets/test/img_7.jpg", "datasets/test/img_8.jpg", "datasets/test/img_9.jpg"]

results = model1(image_list)

for i,result in enumerate(results) : 
      result.save(filename=f"results/img_{i}.jpg")