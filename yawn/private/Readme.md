# Yawn 0/1/2

This challenges were inteded to demonstrate the different handling of HTTP by different applications.

Most tools omit wrong HTTP headers, and make it dificult to see everything.

The challenge names tried to hint on the different versions.

## yawn0

Request the challenge via HTTP/1.0 to get the flag.

## yawn1

Yawn1 uses HTTP Trailer header, essentially an empty chunk after the body, indicated by the trailer header, but omitted by all regular HTTP implementations.

The trailer is always send, you do not need to specify the "TE" header.

## yawn2

Yawn2 pushes the flag as a header via HTTP/2 server push.

As the resource does not make sense, chrome & co will understand the server push but never show it in the developer tools. Use something like [h2c](https://github.com/fstab/h2c) to see it.
