

import cgi
import os
import sys

# Enable debugging
import cgitb
cgitb.enable()

# Print HTTP headers
print("Content-Type: text/html\n")

# Start HTML output
print("<html>")
print("<head><title>Simple Python CGI POST Example</title></head>")
print("<body>")

# Get the content length (size of POST data)
content_length = int(os.environ.get('CONTENT_LENGTH', 0))

# Read the POST data from stdin
if content_length > 0:
    post_data = sys.stdin.read(content_length)
    print("<h3>POST Data Received:</h3>")
    print(f"<p>{post_data}</p>")

    # Parse POST data (assuming application/x-www-form-urlencoded)
    params = cgi.parse_qs(post_data)
    print("<ul>")
    for key, values in params.items():
        for value in values:
            print(f"<li><b>{key}</b>: {value}</li>")
    print("</ul>")
else:
    print("<h3>No POST data received</h3>")
