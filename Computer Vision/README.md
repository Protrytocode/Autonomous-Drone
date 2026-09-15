Each YOLO output gives us a score out of 1.00 on how confident the model is at recognizing humans.

Now, we need to know at what confidence score, the model's prediction becomes unreliable. So for example, if we find that the model repeatedly considers a scarecrow as 0.10 confidence of human, then we conclude that at a confidence of 0.10 our model returns a lot of false positives. Our goal is to find a satisfactory threshold where the image mostly returns true values. For our test dataset, we would need around 100-200 images, where we will find the performance of our model, before and after training. Our images should contain high quality real life pictures (not AI generated), no duplicates, and various different backgrounds and angles to test the model effectively.
So, collect images from various sources for these types : 

1. Human-like confusers

      - Scarecrows — excellent bait.
      - Mannequins — especially realistic ones.
      - Store/display dummies — varied poses.
      - Clothing hanging on a line — surprisingly human-shaped.
      - Coats/jackets hanging from hooks or branches.
      - Clothes piles that create a vaguely human silhouette.
      - Life-size statues.
      - Sculptures resembling people.
      - ardboard cutouts of people.
      - Posters/advertisements containing people — useful because the "person" is 2D.
      - Painted/mural human figures.
      - Reflections of people in windows/water/mirrors.

2. Difficult REAL humans

We also need actual people that are difficult for the model to recognize:

      - Person partially hidden behind rubble
      - Only the head visible
      - Person lying on the ground
      - Person partially covered by debris
      - Person behind vegetation
      - Person at a large distance
      - Person in unusual/awkward poses
      - Person surrounded by clutter/debris
      - Person in low-light conditions
      - Person partially obscured by smoke/dust
      - Multiple people partially overlapping each other