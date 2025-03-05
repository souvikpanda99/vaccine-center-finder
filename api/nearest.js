// api/nearest.js
const fs = require('fs');
const path = require('path');
const RBush = require('rbush').default; // rbush default export
const rbushKnn = require('rbush-knn');
const knn = rbushKnn.default ? rbushKnn.default : rbushKnn; // Fallback import

// Load centers.json (ensure it's inside the public folder)
const centersPath = path.join(__dirname, '../public/centers.json');
const centers = JSON.parse(fs.readFileSync(centersPath, 'utf-8'));

// Build an R-tree index.
const tree = new RBush();
const items = centers.map(center => ({
  minX: center.lon,
  minY: center.lat,
  maxX: center.lon,
  maxY: center.lat,
  center: center
}));
tree.load(items);

// Function to find k nearest centers using the R-tree.
function findKNearest(lat, lon, k) {
  const nearestItems = knn(tree, lon, lat, k);
  return nearestItems.map(item => item.center);
}

// Vercel API function.
module.exports = (req, res) => {
  try {
    const lat = parseFloat(req.query.lat);
    const lon = parseFloat(req.query.lon);
    const k = parseInt(req.query.k);
    
    if (isNaN(lat) || isNaN(lon) || isNaN(k)) {
      return res.status(400).json({ error: 'Invalid query parameters' });
    }
    
    const nearest = findKNearest(lat, lon, k);
    res.status(200).json(nearest);
  } catch (err) {
    console.error('Error in /api/nearest:', err);
    res.status(500).json({ error: 'Server error' });
  }
};
