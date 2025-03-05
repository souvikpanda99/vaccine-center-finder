// api/nearest.js
import fs from 'fs';
import path from 'path';
import RBush from 'rbush';
import knn from 'rbush-knn';

const centersPath = path.join(process.cwd(), 'public', 'centers.json');
const centers = JSON.parse(fs.readFileSync(centersPath, 'utf-8'));

const tree = new RBush();
const items = centers.map(center => ({
  minX: center.lon,
  minY: center.lat,
  maxX: center.lon,
  maxY: center.lat,
  center: center
}));
tree.load(items);

function findKNearest(lat, lon, k) {
  const nearestItems = knn(tree, lon, lat, k);
  return nearestItems.map(item => item.center);
}

function handler(req, res) {
  const lat = parseFloat(req.query.lat);
  const lon = parseFloat(req.query.lon);
  const k = parseInt(req.query.k);
  
  if (isNaN(lat) || isNaN(lon) || isNaN(k)) {
    return res.status(400).json({ error: 'Invalid query parameters' });
  }
  
  const nearest = findKNearest(lat, lon, k);
  res.status(200).json(nearest);
}

export default handler;
