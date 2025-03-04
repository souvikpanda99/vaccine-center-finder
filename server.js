// server.js

const express = require('express');
const app = express();
const port = 3001;

// Serve static files (frontend) from "public" directory.
app.use(express.static('public'));

app.use('/api/centers', require('./api/centers'));
app.use('/api/nearest', require('./api/nearest'));

// Start local development server
app.listen(port, () => {
  console.log(`Server listening at http://localhost:${port}`);
});

