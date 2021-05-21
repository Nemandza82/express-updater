const crypto = require('crypto');
const fs = require('fs');

// Algorithm depends on availability of OpenSSL on platform
// Another algorithms: 'sha1', 'md5', 'sha256', 'sha512' ...
function fileHash(filename, algorithm)
{
    return new Promise((resolve, reject) =>
    {
        try
        {
            let shasum = crypto.createHash(algorithm);
            let stream = fs.ReadStream(filename);
            
            stream.on('data', (data) => {
                shasum.update(data);
            });

            // making digest
            stream.on('end', () => {
                const hash = shasum.digest('hex');
                return resolve(hash);
            });
        }
        catch (error)
        {
            return reject('calc fail');
        }
    });
}

module.exports = fileHash;
