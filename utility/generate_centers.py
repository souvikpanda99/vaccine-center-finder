import random
import json

total_centers = 100000
num_clusters = 100
centers = []
clusters = []

# Generate 100 cluster centers uniformly over the globe.
# (You can adjust the ranges if you want to focus on a specific region.)
for i in range(num_clusters):
    cluster_lat = random.uniform(-90.0, 90.0)
    cluster_lon = random.uniform(-180.0, 180.0)
    clusters.append((cluster_lat, cluster_lon))

centers_per_cluster = total_centers // num_clusters
id_counter = 1

# For each cluster, generate centers using a normal distribution around the cluster center.
for clust in clusters:
    for i in range(centers_per_cluster):
        lat = random.gauss(clust[0], 0.1)  # standard deviation of 0.1 degrees
        lon = random.gauss(clust[1], 0.1)
        centers.append({"lat": lat, "lon": lon, "id": id_counter})
        id_counter += 1

# In case total_centers is not perfectly divisible by num_clusters, add extra centers.
while len(centers) < total_centers:
    clust = clusters[0]
    lat = random.gauss(clust[0], 0.1)
    lon = random.gauss(clust[1], 0.1)
    centers.append({"lat": lat, "lon": lon, "id": id_counter})
    id_counter += 1

# Write the centers list to centers.json
with open("centers.json", "w") as f:
    json.dump(centers, f)

print(f"Generated {len(centers)} centers and saved to centers.json")
