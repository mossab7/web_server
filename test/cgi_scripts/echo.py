#!/usr/bin/env python3
"""
CGI script that echoes back POST data and request information
"""
import os
import sys


# Write HTTP headers manually with CRLF endings
sys.stdout.write("Content-Type: text/html\r\n")
sys.stdout.write("Cache-Control: no-cache\r\n")
sys.stdout.write("\r\n")  # End of headers

# Now print the HTML body
print("""<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>CGI Echo Test</title>
    <style>
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: #f5f5f5;
            margin: 0;
            padding: 40px 20px;
        }
        .container {
            max-width: 800px;
            margin: 0 auto;
            background: white;
            border-radius: 8px;
            box-shadow: 0 2px 8px rgba(0,0,0,0.1);
            padding: 48px;
        }
        h1 {
            color: #1a1a1a;
            margin-bottom: 32px;
            font-size: 28px;
            font-weight: 600;
        }
        .section {
            margin-bottom: 32px;
        }
        .section-title {
            color: #404040;
            font-weight: 500;
            font-size: 13px;
            text-transform: uppercase;
            letter-spacing: 0.05em;
            margin-bottom: 12px;
        }
        .info-box {
            background: #fafafa;
            border: 1px solid #e5e5e5;
            border-radius: 6px;
            padding: 16px;
            font-family: 'SF Mono', Monaco, 'Courier New', monospace;
            font-size: 13px;
            color: #404040;
            line-height: 1.5;
            overflow-x: auto;
        }
        .empty-state {
            color: #737373;
            font-style: italic;
        }
        .status {
            display: inline-flex;
            align-items: center;
            padding: 8px 16px;
            background: #f0fdf4;
            color: #166534;
            border: 1px solid #bbf7d0;
            border-radius: 6px;
            font-size: 14px;
            font-weight: 500;
            margin-bottom: 32px;
        }
        .status::before {
            content: "";
            width: 8px;
            height: 8px;
            background: #22c55e;
            border-radius: 50%;
            margin-right: 8px;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="status">Request Received</div>
        <h1>CGI Echo Test</h1>""")

# Display environment variables
print("""        <div class="section">
            <div class="section-title">Environment Variables</div>
            <div class="info-box">""")

for key in sorted(os.environ.keys()):
    if key.startswith('HTTP_') or key in [
        'REQUEST_METHOD', 'QUERY_STRING', 'CONTENT_TYPE', 'CONTENT_LENGTH',
        'SCRIPT_NAME', 'SCRIPT_FILENAME', 'SERVER_NAME', 'SERVER_PORT',
        'GATEWAY_INTERFACE', 'SERVER_PROTOCOL'
    ]:
        print(f"{key}: {os.environ[key]}<br>")

print("""            </div>
        </div>""")

# Display POST data
print("""        <div class="section">
            <div class="section-title">POST Data</div>
            <div class="info-box">""")

content_length = os.environ.get('CONTENT_LENGTH', '0')
if content_length and content_length != '0':
    post_data = sys.stdin.read(int(content_length))
    print(f"{post_data}")
else:
    print('<span class="empty-state">No POST data received</span>')

print("""            </div>
        </div>""")

# ----------------------------------

# ----------------------------------


# Display query string
print("""        <div class="section">
            <div class="section-title">Query String</div>
            <div class="info-box">""")

query_string = os.environ.get('QUERY_STRING', '')
if query_string:
    print(f"{query_string}")
else:
    print('<span class="empty-state">No query string</span>')

print("""            </div>
        </div>
    </div>
</body>
</html>""")