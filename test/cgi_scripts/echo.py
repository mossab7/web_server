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

# Now print the HTML body (normal \n fine here — browsers handle HTML)
print("<!DOCTYPE html>")
print("<html>")
print("<head><title>CGI Echo Test</title></head>")
print("<body>")
print("<h1>CGI Echo Test</h1>")

# Display environment variables
print("<h2>Environment Variables</h2>")
print("<table border='1'>")
print("<tr><th>Variable</th><th>Value</th></tr>")
for key in sorted(os.environ.keys()):
    if key.startswith('HTTP_') or key in [
        'REQUEST_METHOD', 'QUERY_STRING', 'CONTENT_TYPE', 'CONTENT_LENGTH',
        'SCRIPT_NAME', 'SCRIPT_FILENAME', 'SERVER_NAME', 'SERVER_PORT',
        'GATEWAY_INTERFACE', 'SERVER_PROTOCOL'
    ]:
        print(f"<tr><td>{key}</td><td>{os.environ[key]}</td></tr>")
print("</table>")

# Display POST data
print("<h2>POST Data</h2>")
content_length = os.environ.get('CONTENT_LENGTH', '0')
if content_length and content_length != '0':
    post_data = sys.stdin.read(int(content_length))
    print(f"<pre>{post_data}</pre>")
else:
    print("<p>No POST data received</p>")

# Display query string
print("<h2>Query String</h2>")
query_string = os.environ.get('QUERY_STRING', '')
if query_string:
    print(f"<pre>{query_string}</pre>")
else:
    print("<p>No query string</p>")

print("</body>")
print("</html>")
