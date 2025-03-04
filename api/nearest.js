// api/nearest.js
const fs = require('fs');
const path = require('path');

// Load centers.json (ensure it's inside the public folder)
const centersPath = path.join(__dirname, '../public/centers.json');
const centers = JSON.parse(fs.readFileSync(centersPath, 'utf-8'));

// Simple k-nearest neighbor search
function findKNearest(lat, lon, k) {
  const sorted = centers
    .map(center => {
      const dx = center.lon - lon;
      const dy = center.lat - lat;
      return { ...center, distance: Math.sqrt(dx * dx + dy * dy) };
    })
    .sort((a, b) => a.distance - b.distance);
  return sorted.slice(0, k);
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
