#!/usr/bin/env python3
"""
CGI script that simulates a slow response for timeout testing
Usage: /cgi/slow?sleep=5 (sleeps for 5 seconds)
"""
import os
import time


time.sleep(int(os.environ.get('QUERY_STRING', '0').split('=')[1]) if '=' in os.environ.get('QUERY_STRING', '') else 0)

print("Content-Type: text/html", end="\r\n")
print(end="\r\n")
print("<!DOCTYPE html>")
print("<html>")
print("<head><title>Hello from Python CGI</title></head>")
print("<body>")
print("<h1>Hello from Python CGI!</h1>")
print("<p>This is a simple CGI script running with Python for CGI timeout testing.</p>")
print("<p>Current time: ", end="")
import datetime
print(datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"))
print("</p>")
print("</body>")
print("</html>")