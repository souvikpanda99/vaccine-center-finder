// server.js
const express = require('express');
const app = express();
const port = 3000;

const path = require('path');
const fs = require('fs');

// Build the full path to centers.json in the public folder.
const centersPath = path.join(__dirname, 'centers.json');

// Synchronously read and parse the JSON file.
const centers = JSON.parse(fs.readFileSync(centersPath, 'utf-8'));

// For demonstration, we simulate the vaccine centers dataset.
// In production, you could load data generated from your C++ R-tree build.
// const centers = require('./centers.json'); // assume centers.json contains our 100K centers

// Simple k-nearest neighbor search (using Euclidean distance).
// In a real-world scenario, you’d use your pre-built R-tree index.
function findKNearest(lat, lon, k) {
  // Compute Euclidean distance for each center.
  const sorted = centers
    .map(center => {
      const dx = center.lon - lon;
      const dy = center.lat - lat;
      return { ...center, distance: Math.sqrt(dx * dx + dy * dy) };
    })
    .sort((a, b) => a.distance - b.distance);
  return sorted.slice(0, k);
}

// Endpoint to get all centers.
app.get('/centers', (req, res) => {
  res.json(centers);
});

// Endpoint to get k nearest centers.
app.get('/nearest', (req, res) => {
  const lat = parseFloat(req.query.lat);
  const lon = parseFloat(req.query.lon);
  const k = parseInt(req.query.k);
  if (isNaN(lat) || isNaN(lon) || isNaN(k)) {
    return res.status(400).json({ error: 'Invalid query parameters' });
  }
  const nearest = findKNearest(lat, lon, k);
  res.json(nearest);
});

// Serve static files (frontend) from the "public" directory.
app.use(express.static('public'));

app.listen(port, () => {
  console.log(`Server listening at http://localhost:${port}`);
});

