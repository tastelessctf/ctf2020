const http = require('http');
const url = require('url');
const fs = require('fs');
const path = require('path');

http.createServer(function (req, res) {
    console.log(`${req.method} ${req.url}`);
    const parsedUrl = url.parse(req.url);
    let pathname = `.${parsedUrl.pathname}`;

    if (pathname === './dynamic-scoring.js') {
        fs.readFile(pathname, function (err, data) {
            if (err) {
                res.statusCode = 500;
                res.end(`Error getting the file: ${err}.`);
            } else {
                res.setHeader('Content-type', 'text/javascript');
                res.end(data);
            }
        });
    } else {
        res.statusCode = 404;
        res.end(`File not found!`);
        return;
    }
}).listen(1338);

console.log(`Server listening on port 1338`);
