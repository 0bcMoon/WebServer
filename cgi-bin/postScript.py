#!/usr/bin/env python3
import cgi
import cgitb

# Enable debugging
cgitb.enable()

# Get form data
form = cgi.FieldStorage()
name = form.getvalue("name", "Guest")

# Output CGI headers
print("Content-Type: text/html")
print()

# Output HTML content
print(f"""
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>CGI Script Example</title>
</head>
<body>
    <h1>Hello, {name}!</h1>
    <p>Welcome to this example of a CGI script.</p>
</body>
</html>
""")
