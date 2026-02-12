#!/usr/bin/env python3
import argparse
import errno
import http.server
import os
import socketserver


class ReusableTCPServer(socketserver.TCPServer):
    allow_reuse_address = True


def create_server(host: str, port: int, handler: type[http.server.BaseHTTPRequestHandler], max_offset: int = 20) -> tuple[ReusableTCPServer, int]:
    for offset in range(max_offset + 1):
        candidate_port = port + offset
        try:
            server = ReusableTCPServer((host, candidate_port), handler)
            return server, candidate_port
        except OSError as exc:
            if exc.errno != errno.EADDRINUSE:
                raise
    raise SystemExit(f"No available port in range {port}-{port + max_offset}")

def main() -> None:
    parser = argparse.ArgumentParser(description="Simple static file server")
    parser.add_argument("--dir", default="dist", help="Directory to serve")
    parser.add_argument("--host", default="0.0.0.0", help="Bind host")
    parser.add_argument("--port", type=int, default=5173, help="Bind port")
    args = parser.parse_args()

    serve_dir = os.path.abspath(args.dir)
    if not os.path.isdir(serve_dir):
        raise SystemExit(f"Directory not found: {serve_dir}")

    os.chdir(serve_dir)
    handler = http.server.SimpleHTTPRequestHandler
    httpd, selected_port = create_server(args.host, args.port, handler)
    with httpd:
        if selected_port != args.port:
            print(f"Port {args.port} is in use, switched to {selected_port}")
        print(f"Serving {serve_dir} at http://{args.host}:{selected_port}")
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("Shutting down...")

if __name__ == "__main__":
    main()
