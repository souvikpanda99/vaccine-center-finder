// api/centers.js
const fs = require('fs');
const path = require('path');

const centersPath = path.join(__dirname, '../public/centers.json');
const centers = JSON.parse(fs.readFileSync(centersPath, 'utf-8'));

module.exports = (req, res) => {
  res.status(200).json(centers);
};
