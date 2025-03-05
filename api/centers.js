// api/centers.js
import fs from 'fs';
import path from 'path';

const centersPath = path.join(process.cwd(), 'public', 'centers.json');
const centers = JSON.parse(fs.readFileSync(centersPath, 'utf-8'));

function handler(req, res) {
  res.status(200).json(centers);
}

export default handler;
