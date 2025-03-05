// api/nearest.js
const fs = require('fs');
const path = require('path');
const RBush = require('rbush').default;  // use .default because rbush is an ES module
const knn = require('rbush-knn').default;

// Load centers.json (ensure it's inside the public folder)
const centersPath = path.join(__dirname, '../public/centers.json');
const centers = JSON.parse(fs.readFileSync(centersPath, 'utf-8'));

// Build an R-tree index.
// Each center is converted to an object with a bounding box.
const tree = new RBush();
const items = centers.map(center => ({
  minX: center.lon,
  minY: center.lat,
  maxX: center.lon,
  maxY: center.lat,
  center: center
}));
tree.load(items);

// Function to find k nearest centers using the R-tree
function findKNearest(lat, lon, k) {
  // rbush-knn expects (x, y) so we use (lon, lat)
  const nearestItems = knn(tree, lon, lat, k);
  // Extract the original center objects from the results.
  return nearestItems.map(item => item.center);
}

// Vercel API function format (req, res)
module.exports = (req, res) => {
  const lat = parseFloat(req.query.lat);
  const lon = parseFloat(req.query.lon);
  const k = parseInt(req.query.k);
  
  if (isNaN(lat) || isNaN(lon) || isNaN(k)) {
    return res.status(400).json({ error: 'Invalid query parameters' });
  }

  const nearest = findKNearest(lat, lon, k);
  res.status(200).json(nearest);
};

