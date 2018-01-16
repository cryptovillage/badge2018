#!/usr/bin/env python
# This probably won't work right on Windows

import SimpleHTTPServer
import SocketServer
import hashlib
import json
import sys
import os

PORT = 8080

def bytes_to_hex(bytes):
    return ''.join('%02x' % b for b in bytes)

class UserRequestHandler(SimpleHTTPServer.SimpleHTTPRequestHandler):
    def generate_manifest(self):
        url_base = 'http://' + self.headers['Host'] + '/'
        manifest = {}
        for dp, dn, fl in os.walk('.'):
            for filename in fl:
                try:
                    filename = os.path.join(dp, filename)[2:]
                    f = open(filename, 'rb')
                    hash = hashlib.sha256()
                    while True:
                        data = f.read(4096)
                        if len(data) == 0:
                            f.close()
                            break
                        hash.update(data)
                    hash = bytes_to_hex(bytearray(hash.digest()))
                    manifest[filename] = { 'url': url_base + filename, 'sha256': hash }
                except Exception as e:
                    print("Error while processing {}: {}".format(filename, e))
        return manifest

    def return_manifest(self):
        manifest = self.generate_manifest()
        manifest_json = str.encode(json.dumps(manifest))
        self.send_response(200)
        self.send_header("Content-type", "application/json")
        self.send_header("Content-length", len(manifest_json))
        self.end_headers()
        self.wfile.write(manifest_json)
        
    def do_GET(self):
        if self.path == '/':
            self.return_manifest()
        else:
            SimpleHTTPServer.SimpleHTTPRequestHandler.do_GET(self)

Handler = UserRequestHandler
server = SocketServer.TCPServer(('0.0.0.0', PORT), Handler)

server.serve_forever()
