// Needs express framework installed
// npm install express

// Settings
const algorithm = 'sha1';
const supportedOss = ["win"];
let dataDir = './app';

// Key used for security so, no random atack can use the service too much...
const apiKey = "bb5014d3d821b5dc88dcdc225884e161cfa6c6ce";

const express = require('express');
var app = express();
const fs = require('fs');
const fileHash = require('./filehash.js');

// Cashed results, so not to calculate sha1 every time...
var _fileList = {};

app.get('/', (req, res) => {
    res.send('Hello, I am Smartlab automatic update service.');
});

let folderHash = async (res, dir, baseDir) => 
{
    return new Promise((resolve, reject) =>
    {
        fs.readdir(dir, async (err, files) =>
        {
            if (err)
            {
                console.log("Error while reading source dir.\n" + err); 
                return;
            }

            for (let i = 0; i < files.length; i++)
            {
                let itemName = dir + '/' + files[i];
                const stat = fs.statSync(itemName);

                if (stat.isDirectory())
                {
                    await folderHash(res, itemName, baseDir);
                }
                else
                {
                    let hash = await fileHash(itemName, algorithm); 
                    res.push({name:itemName.substring(baseDir.length+1), hash, size:stat.size});
                }
            }

            return resolve();
        });
    });
};

let checkKeys = (req, res) => 
{
    if (!req.query.os || !supportedOss.includes(req.query.os))
    {
        res.status(404);
        res.send('Os key not recognized.');
        return false;
    }

    if (!req.query.key || req.query.key != apiKey)
    {
        res.status(404);
        res.send('Invalid API Key.');
        return false;
    }

    return true;
}

app.get('/api/list', async (req, res) =>
{
    if (!checkKeys(req, res))
        return;

    const reqDir = dataDir + req.query.os;
    console.log(`Listing ${reqDir}`);
    let result = [];
    console.time('FileList');

    if (_fileList[req.query.os])
    {
        console.log(`Getting already calculated result`);
        result = _fileList[req.query.os];
    }
    else
    {
        console.log(`Traversing folder and calculating sha1`);
        await folderHash(result, reqDir, reqDir);

        // Save it for next call
        _fileList[req.query.os] = result;
    }
    
    console.timeEnd('FileList');
    res.send(result);
});

app.get('/api/download', (req, res) =>
{
    if (!checkKeys(req, res))
        return;
    
    if (req.query.item)
    {
        const filePath = dataDir + req.query.os + "/" + req.query.item;
        console.log(`Downloading ${filePath}`);

        if (fs.existsSync(filePath))
        {
            let stream = fs.createReadStream(filePath);
        
            stream.on('error', function() {
                console.log("Error while sending file in response.");
            });

            const stat = fs.statSync(filePath);

            res.writeHead(200, {
                'Content-Type': 'application/octet-stream',
                'Content-Length': stat.size
            });

            stream.pipe(res);
        }
        else
        {
            res.status(404);
            res.send('File does not exist.');
        }
    }
    else
    {
        res.status(404);
        res.send('Item for download was not set.');
    }
});

const port = process.env.PORT || 3000;

app.listen(port, () => {
    console.log(`Listenning on port ${port}...`);
});
