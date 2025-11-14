#!/usr/bin/env python3
import os
import sys
import json
import time
from datetime import datetime

SESSION_FILE = "/tmp/sessions.json"
SESSION_TIMEOUT = 60

# Load sessions
try:
    sessions = json.load(open(SESSION_FILE))
except:
    sessions = {}

# Clean expired sessions
current_time = time.time()
valid_sessions = {}
for session_id, session_data in sessions.items():
    timestamp = session_data.get('timestamp', 0)
    if current_time - timestamp < SESSION_TIMEOUT:
        valid_sessions[session_id] = session_data
sessions = valid_sessions

# Get session ID from cookie
cookie = os.environ.get('HTTP_COOKIE', '')
session_id = None
for part in cookie.split(';'):
    if 'sid=' in part:
        session_id = part.split('sid=')[1].strip()
        break

id_found = True

# Check if session is valid
if session_id not in sessions:
    session_id = None
    id_found = False

# Create new session or update existing
if session_id:
    sessions[session_id]['count'] += 1
    username = sessions[session_id]['username']
    count = sessions[session_id]['count']
    created_time = sessions[session_id]['timestamp']
else:
    session_id = str(int(current_time))
    username = f"user_{session_id[-4:]}"
    count = 1
    created_time = current_time
    sessions[session_id] = {
        'username': username,
        'count': count,
        'timestamp': created_time
    }

# Save sessions
json.dump(sessions, open(SESSION_FILE, 'w'))

# Calculate expiration
expiration = datetime.fromtimestamp(created_time + SESSION_TIMEOUT)
expiration_str = expiration.strftime('%Y-%m-%d %H:%M:%S')

# HTTP headers
sys.stdout.write("Content-Type: text/html\r\n")
if id_found == False:
    sys.stdout.write(f"Set-Cookie: sid={session_id}; Path={os.environ['SCRIPT_NAME']}\r\n")
sys.stdout.write("\r\n")

# HTML output
print(f"""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Session Manager</title>
    <style>
        * {{
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }}
        
        body {{
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, sans-serif;
            background: #f5f5f5;
            min-height: 100vh;
            padding: 40px 20px;
            display: flex;
            justify-content: center;
            align-items: center;
        }}
        
        .container {{
            background: white;
            border-radius: 4px;
            padding: 48px;
            max-width: 600px;
            width: 100%;
            box-shadow: 0 1px 3px rgba(0, 0, 0, 0.1);
            text-align: center;
        }}
        
        h1 {{
            color: #1a1a1a;
            margin-bottom: 16px;
            font-size: 24px;
            font-weight: 600;
            letter-spacing: -0.02em;
        }}
        
        .username {{
            font-size: 48px;
            font-weight: 700;
            color: #1a1a1a;
            margin: 32px 0 16px 0;
            font-variant-numeric: tabular-nums;
        }}
        
        .counter {{
            font-size: 72px;
            font-weight: 700;
            color: #1a1a1a;
            margin: 16px 0 32px 0;
            font-variant-numeric: tabular-nums;
        }}
        
        .message {{
            color: #737373;
            font-size: 14px;
            margin-bottom: 32px;
        }}
        
        .info-box {{
            background: #fafafa;
            border: 1px solid #e5e5e5;
            border-radius: 3px;
            padding: 16px;
            margin-top: 32px;
            text-align: left;
        }}
        
        .info-title {{
            font-weight: 600;
            margin-bottom: 8px;
            font-size: 13px;
            text-transform: uppercase;
            letter-spacing: 0.05em;
            color: #404040;
        }}
        
        .info-text {{
            font-size: 13px;
            color: #737373;
            line-height: 1.6;
            font-family: 'SF Mono', Monaco, 'Courier New', monospace;
        }}
        
        button {{
            padding: 12px 24px;
            border: 1px solid #1a1a1a;
            border-radius: 3px;
            font-size: 13px;
            font-weight: 500;
            cursor: pointer;
            transition: all 0.2s;
            text-transform: uppercase;
            letter-spacing: 0.05em;
            background: white;
            color: #1a1a1a;
            margin-top: 16px;
        }}
        
        button:hover {{
            background: #1a1a1a;
            color: white;
        }}
    </style>
</head>
<body>
    <div class="container">
        <h1>Session Manager</h1>
        <div class="username">{username}</div>
        <div class="counter">{count}</div>
        <div class="message">
            You have visited this page {count} time(s)
        </div>
        
        <button onclick="location.reload()">Refresh Page</button>
        
        <div class="info-box">
            <div class="info-title">Session Information</div>
            <div class="info-text">
                Session ID: {session_id}<br>
                Storage: {SESSION_FILE}<br>
                Expires: {expiration_str}<br>
            </div>
        </div>
    </div>
</body>
</html>""")