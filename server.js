// server.js
import express from 'express'
import centersHandler from './api/centers.js';
import nearestHandler from './api/nearest.js';

const app = express();
const port = 3001;

// Serve static files (frontend) from "public" directory.
app.use(express.static('public'));

app.use('/api/centers', centersHandler);
app.use('/api/nearest', nearestHandler);

// Start local development server
app.listen(port, () => {
  console.log(`Server listening at http://localhost:${port}`);
});

