#!/usr/bin/env python3
"""
Simple CGI test script that returns a basic HTML page
"""

print("Content-Type: text/html", end="\r\n")
print(end="\r\n")  # Empty line to separate headers from body
print("<!DOCTYPE html>")
print("<html>")
print("<head><title>Hello from Python CGI</title></head>")
print("<body>")
print("<h1>Hello from Python CGI!</h1>")
print("<p>This is a simple CGI script running with Python.</p>")
print("<p>Current time: ", end="")
import datetime
print(datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"))
print("</p>")
print("</body>")
print("</html>")
