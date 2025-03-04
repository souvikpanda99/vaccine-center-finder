#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <cmath>
#include <random> 
#include <fstream>
#include "json.hpp"  // Include the nlohmann/json header

using json = nlohmann::json;


// Structure representing a vaccine center (using lat as y and lon as x)
struct VaccineCenter {
    double lat, lon; // Latitude and longitude
    int id;          // Unique identifier
};

// A rectangle (bounding box) used in the R‑tree nodes.
struct Rect {
    double minX, minY, maxX, maxY;
    Rect() : minX(1e9), minY(1e9), maxX(-1e9), maxY(-1e9) {}
    Rect(double x, double y) : minX(x), minY(y), maxX(x), maxY(y) {}

    // Expand the rectangle to include the point (x, y).
    void expand(double x, double y) {
        minX = std::min(minX, x);
        minY = std::min(minY, y);
        maxX = std::max(maxX, x);
        maxY = std::max(maxY, y);
    }

    // Compute the area of the rectangle.
    double area() const {
        return (maxX - minX) * (maxY - minY);
    }

    // Minimum Euclidean distance from a point (x, y) to this rectangle.
    double distance(double x, double y) const {
        double dx = 0, dy = 0;
        if (x < minX)      dx = minX - x;
        else if (x > maxX) dx = x - maxX;
        if (y < minY)      dy = minY - y;
        else if (y > maxY) dy = y - maxY;
        return std::sqrt(dx * dx + dy * dy);
    }
};

// R‑tree node definition.
struct RTreeNode {
    bool isLeaf;         // true if this is a leaf node
    Rect mbr;            // minimum bounding rectangle for this node
    std::vector<VaccineCenter> points;     // data points (only if leaf)
    std::vector<RTreeNode*> children;        // child nodes (if not leaf)

    RTreeNode(bool leaf) : isLeaf(leaf) {}
};

// An R‑tree built via a bulk‑loading (static) approach.
class RTree {
public:
    RTreeNode* root;
    int maxEntries; // maximum entries per node

    // Build the tree given the vector of vaccine centers.
    RTree(const std::vector<VaccineCenter>& points, int maxEntries = 4) : maxEntries(maxEntries) {
        // Build leaf nodes first.
        std::vector<RTreeNode*> leaves = buildLeafNodes(points);
        // Build the tree upward.
        root = buildTree(leaves);
    }

    // Perform a nearest-neighbor query to return the k closest vaccine centers.
    // (This example uses Euclidean distance on lat/lon coordinates.)
    std::vector<VaccineCenter> nearestNeighbors(double queryLat, double queryLon, int k) {
        // Priority queue for tree nodes ordered by minimum distance to their MBR.
        typedef std::pair<double, RTreeNode*> NodeQueueItem;
        auto cmp = [](const NodeQueueItem &a, const NodeQueueItem &b) {
            return a.first > b.first;
        };
        std::priority_queue<NodeQueueItem, std::vector<NodeQueueItem>, decltype(cmp)> nodeQueue(cmp);
        nodeQueue.push({ root->mbr.distance(queryLon, queryLat), root });
        
        // Priority queue to hold the best (closest) points found so far.
        // We use a max-heap so that the point with the largest distance is on top.
        typedef std::pair<double, VaccineCenter> PointQueueItem;
        auto cmpPoint = [](const PointQueueItem &a, const PointQueueItem &b) {
            return a.first < b.first;
        };
        std::priority_queue<PointQueueItem, std::vector<PointQueueItem>, decltype(cmpPoint)> bestPoints(cmpPoint);

        while (!nodeQueue.empty()) {
            auto current = nodeQueue.top();
            nodeQueue.pop();
            double nodeDist = current.first;
            RTreeNode* node = current.second;

            // If we already have k points and this node's distance exceeds the worst candidate, skip.
            if (bestPoints.size() == (size_t)k && nodeDist > bestPoints.top().first)
                continue;

            if (node->isLeaf) {
                // Evaluate every point in the leaf.
                for (auto &pt : node->points) {
                    double d = std::sqrt((pt.lon - queryLon) * (pt.lon - queryLon) +
                                         (pt.lat - queryLat) * (pt.lat - queryLat));
                    if (bestPoints.size() < (size_t)k) {
                        bestPoints.push({ d, pt });
                    } else if (d < bestPoints.top().first) {
                        bestPoints.pop();
                        bestPoints.push({ d, pt });
                    }
                }
            } else {
                // For internal nodes, add children that might contain closer points.
                for (auto child : node->children) {
                    double d = child->mbr.distance(queryLon, queryLat);
                    if (bestPoints.size() < (size_t)k || d < bestPoints.top().first) {
                        nodeQueue.push({ d, child });
                    }
                }
            }
        }

        // Extract points from the bestPoints heap.
        std::vector<VaccineCenter> results;
        while (!bestPoints.empty()) {
            results.push_back(bestPoints.top().second);
            bestPoints.pop();
        }
        // Reverse the result to have closest points first.
        std::reverse(results.begin(), results.end());
        return results;
    }

private:
    // Build leaf nodes using a simple sort (by latitude) and grouping method.
    std::vector<RTreeNode*> buildLeafNodes(const std::vector<VaccineCenter>& points) {
        std::vector<VaccineCenter> pts = points;
        std::sort(pts.begin(), pts.end(), [](const VaccineCenter &a, const VaccineCenter &b) {
            return a.lat < b.lat;
        });
        std::vector<RTreeNode*> leaves;
        for (size_t i = 0; i < pts.size(); i += maxEntries) {
            RTreeNode* leaf = new RTreeNode(true);
            for (size_t j = i; j < i + maxEntries && j < pts.size(); j++) {
                leaf->points.push_back(pts[j]);
                // Initialize or expand the leaf's bounding rectangle.
                if (leaf->points.size() == 1)
                    leaf->mbr = Rect(pts[j].lon, pts[j].lat);
                else
                    leaf->mbr.expand(pts[j].lon, pts[j].lat);
            }
            leaves.push_back(leaf);
        }
        return leaves;
    }

    // Build higher-level nodes until a single root remains.
    RTreeNode* buildTree(std::vector<RTreeNode*>& nodes) {
        if (nodes.empty())
            return nullptr;
        while (nodes.size() > 1) {
            std::vector<RTreeNode*> newLevel;
            // Sort nodes by the center x coordinate of their MBR.
            std::sort(nodes.begin(), nodes.end(), [](RTreeNode* a, RTreeNode* b) {
                double centerA = (a->mbr.minX + a->mbr.maxX) / 2.0;
                double centerB = (b->mbr.minX + b->mbr.maxX) / 2.0;
                return centerA < centerB;
            });
            for (size_t i = 0; i < nodes.size(); i += maxEntries) {
                RTreeNode* parent = new RTreeNode(false);
                for (size_t j = i; j < i + maxEntries && j < nodes.size(); j++) {
                    parent->children.push_back(nodes[j]);
                    if (parent->children.size() == 1)
                        parent->mbr = nodes[j]->mbr;
                    else {
                        // Expand parent's MBR to include the child's MBR.
                        parent->mbr.expand(nodes[j]->mbr.minX, nodes[j]->mbr.minY);
                        parent->mbr.expand(nodes[j]->mbr.maxX, nodes[j]->mbr.maxY);
                    }
                }
                newLevel.push_back(parent);
            }
            nodes = newLevel;
        }
        return nodes[0];
    }
};

std::vector<VaccineCenter> loadCenters(const std::string& filename) {
    std::vector<VaccineCenter> centers;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open " << filename << std::endl;
        return centers;
    }

    json j;
    file >> j;
    for (const auto& item : j) {
        VaccineCenter center;
        center.lat = item["lat"].get<double>();
        center.lon = item["lon"].get<double>();
        center.id  = item["id"].get<int>();
        centers.push_back(center);
    }
    return centers;
}


//--------------------------
// Main: Generate 100,000 Centers with 100 Clusters and Sample Usage
//--------------------------
int main() {
    const int totalCenters = 100000;
    const int numClusters = 100;
    /*const int totalCenters = 100000;
    const int numClusters = 100;
    std::vector<VaccineCenter> centers;
    centers.reserve(totalCenters);

    // Random generators for data.
    std::random_device rd;
    std::mt19937 gen(rd());

    // Generate 100 cluster centers uniformly over the globe.
    std::uniform_real_distribution<double> clusterLatDist(-90.0, 90.0);
    std::uniform_real_distribution<double> clusterLonDist(-180.0, 180.0);
    std::vector<std::pair<double, double>> clusterCenters;
    for (int i = 0; i < numClusters; i++) {
        double clustLat = clusterLatDist(gen);
        double clustLon = clusterLonDist(gen);
        clusterCenters.push_back({ clustLat, clustLon });
    }

    // Normal distribution for offsets around each cluster center (std dev = 0.1 degrees).
    std::normal_distribution<double> offsetDist(0.0, 0.1);
    int centersPerCluster = totalCenters / numClusters;
    int idCounter = 1;
    for (auto& clust : clusterCenters) {
        for (int i = 0; i < centersPerCluster; i++) {
            double lat = clust.first + offsetDist(gen);
            double lon = clust.second + offsetDist(gen);
            centers.push_back({ lat, lon, idCounter++ });
        }
    }
    // In case totalCenters is not perfectly divisible, add extra centers.
    while (centers.size() < (size_t)totalCenters) {
        double lat = clusterCenters[0].first + offsetDist(gen);
        double lon = clusterCenters[0].second + offsetDist(gen);
        centers.push_back({ lat, lon, idCounter++ });
    }*/

    std::vector<VaccineCenter> centers = loadCenters("public/centers.json");

    std::cout << "Generated " << centers.size() << " vaccine centers across " << numClusters << " clusters." << std::endl;

    // Build the R‑tree using the centers.
    RTree rtree(centers, 4);

    // Define a query coordinate (for example, near Los Angeles).
    double queryLat = 34.0;
    double queryLon = -118.0;
    int k = 3; // Number of nearest neighbors to retrieve

    // Perform the nearest neighbor search.
    std::vector<VaccineCenter> nearest = rtree.nearestNeighbors(queryLat, queryLon, k);

    // Output the results.
    std::cout << "The " << k << " nearest vaccine centers to (" << queryLat << ", " << queryLon << ") are:\n";
    for (const auto &center : nearest) {
        double d = std::sqrt((center.lon - queryLon) * (center.lon - queryLon) +
                             (center.lat - queryLat) * (center.lat - queryLat));
        std::cout << "Center " << center.id << " at (" << center.lat << ", " << center.lon 
                  << ") with distance " << d << "\n";
    }
    return 0;
}
