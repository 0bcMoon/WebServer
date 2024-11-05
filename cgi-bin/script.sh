#!/bin/bash

echo "HTTP/1.1 200 OK"
echo "Content-Type: text/html"
echo ""
echo "<html>"
echo "<head><title>Simple Bash CGI Script</title></head>"
echo "<body>"

# Get the query string from the environment (if passed)
echo "<h3>No query string received</h3>"

# Output current date and time
echo "<p>Current date and time: $(date)</p>"

# End HTML output
echo "</body>"
echo "</html>"
