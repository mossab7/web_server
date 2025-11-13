#!/usr/bin/env python3
"""
Simple CGI test script with clean styling
"""

import datetime

print("Content-Type: text/html", end="\r\n")
print(end="\r\n")

current_time = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")

print("""<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Python CGI</title>
    <style>
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: #f5f5f5;
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
            margin: 0;
            padding: 20px;
        }
        .container {
            background: white;
            padding: 48px;
            border-radius: 8px;
            box-shadow: 0 2px 8px rgba(0,0,0,0.1);
            text-align: center;
            max-width: 400px;
            width: 100%;
        }
        h1 {
            color: #1a1a1a;
            margin-bottom: 24px;
            font-size: 28px;
            font-weight: 600;
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
        .time {
            color: #666;
            font-size: 16px;
            line-height: 1.5;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="status">Server is running</div>
        <h1>Hello World!</h1>
        <div class="time">""" + current_time + """</div>
    </div>
</body>
</html>""")