#!/usr/bin/env python3
import os
import sys
import json
import base64

def read_stdin():
    length = os.environ.get("CONTENT_LENGTH")
    if not length:
        return b""
    try:
        n = int(length)
    except ValueError:
        return b""
    return sys.stdin.buffer.read(n)

def main():
    method = os.environ.get("REQUEST_METHOD", "")
    content_type = os.environ.get("CONTENT_TYPE", "")
    query = os.environ.get("QUERY_STRING", "")

    body = read_stdin()

    response = {
        "request_method": method,
        "content_type": content_type,
        "query_string": query,
        "content_length": len(body),
    }

    try:
        response["body_text"] = body.decode("utf-8")
    except Exception:
        response["body_base64"] = base64.b64encode(body).decode("ascii")

    out = json.dumps(response, ensure_ascii=False, indent=2)
    encoded = out.encode("utf-8")

    # Print CGI response headers with CRLF endings
    sys.stdout.buffer.write(b"Content-Type: application/json; charset=utf-8\r\n")
    sys.stdout.buffer.write(f"Content-Length: {len(encoded)}\r\n".encode("ascii"))
    sys.stdout.buffer.write(b"\r\n")  # End of headers
    sys.stdout.buffer.write(encoded)

if __name__ == "__main__":
    main()
