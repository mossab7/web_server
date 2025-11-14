import os
import sys

# Write HTTP headers manually with CRLF endings
sys.stdout.write("Content-Type: text/html\r\n")
sys.stdout.write("Cache-Control: no-cache\r\n")

visits = 0

# Check if cookie exists and parse it
if 'HTTP_COOKIE' in os.environ:
    cookies = os.environ['HTTP_COOKIE']
    # Parse the cookies string
    for cookie in cookies.split(';'):
        cookie = cookie.strip()
        if cookie.startswith('visits='):
            visits = int(cookie.split('=')[1])
            break

# Increment visit count
visits += 1

# Set the updated cookie
sys.stdout.write(f"Set-Cookie: visits={visits}; Path={os.environ['SCRIPT_NAME']}\r\n")
sys.stdout.write("\r\n")  # End of headers

print("""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Visit Counter</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, sans-serif;
            background: #f5f5f5;
            min-height: 100vh;
            padding: 40px 20px;
            display: flex;
            justify-content: center;
            align-items: center;
        }
        
        .container {
            background: white;
            border-radius: 4px;
            padding: 48px;
            max-width: 600px;
            width: 100%;
            box-shadow: 0 1px 3px rgba(0, 0, 0, 0.1);
            text-align: center;
        }
        
        h1 {
            color: #1a1a1a;
            margin-bottom: 16px;
            font-size: 24px;
            font-weight: 600;
            letter-spacing: -0.02em;
        }
        
        .counter {
            font-size: 72px;
            font-weight: 700;
            color: #1a1a1a;
            margin: 32px 0;
            font-variant-numeric: tabular-nums;
        }
        
        .message {
            color: #737373;
            font-size: 14px;
            margin-bottom: 32px;
        }
        
        .info-box {
            background: #fafafa;
            border: 1px solid #e5e5e5;
            border-radius: 3px;
            padding: 16px;
            margin-top: 32px;
            text-align: left;
        }
        
        .info-title {
            font-weight: 600;
            margin-bottom: 8px;
            font-size: 13px;
            text-transform: uppercase;
            letter-spacing: 0.05em;
            color: #404040;
        }
        
        .info-text {
            font-size: 13px;
            color: #737373;
            line-height: 1.6;
            font-family: 'SF Mono', Monaco, 'Courier New', monospace;
        }
        
        button {
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
        }
        
        button:hover {
            background: #1a1a1a;
            color: white;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>Visit Counter</h1>
        <div class="counter">""" + str(visits) + """</div>
        <div class="message">
            """ + ("This is your first visit!" if visits == 1 else f"You have visited this page {visits} times") + """
        </div>
        
        <button onclick="location.reload()">Refresh Page</button>
        
        <div class="info-box">
            <div class="info-title">How it works</div>
            <div class="info-text">
                This counter uses HTTP cookies to track your visits.<br>
                Cookie: visits=""" + str(visits) + """<br>
                The counter persists across page refreshes.
            </div>
        </div>
    </div>
</body>
</html>""")