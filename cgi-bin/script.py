import os
import datetime
from urllib.parse import parse_qs

# Enable debugging
# cgitb.enable()

    # Print HTTP header
print("HTTP/1.1 200 OK")
print("Content-Type: text/html\r\n")

# Start HTML output
print("<html>")
print("<head><title>Simple Python CGI Script</title></head>")
print("<body>")
print("Hello")
# Get the query string (GET request parameters)
query_string = os.environ.get('QUERY_STRING', '')
parsed_params = parse_qs(query_string)

# If there are parameters in the query string, display them
if parsed_params:
    print("<h3>Query Parameters Received:</h3>")
    print("<ul>")
    for key, values in parsed_params.items():
        for value in values:
            print(f"<li><b>{key}</b>: {value}</li>")
    print("</ul>")
else:
    print("<h3>No query parameters received</h3>")

# Print the current environment variables
print("<h3>Environment Variables:</h3>")
print("<ul>")
for key, value in os.environ.items():
    print(f"<li><b>{key}</b>: {value}</li>")
print("</ul>")

# Output the current date and time
print(f"<p>Current date and time: {datetime.datetime.now()}</p>")

# End HTML output
print("</body>")
print("</html>")
