#!/bin/bash

echo "HTTP/1.1 200 OK"
echo "Content-Type: text/html"
echo "5: l"
echo ""
echo "<html>"
echo "<head><title>Simple Bash CGI Script</title></head>"
echo "<body>"

# Get the query string from the environment (if passed)
$ if [ -n "$QUERY_STRING" ]; then
    echo "<h3>Query String: $QUERY_STRING</h3>"
    # Parse and display each query parameter
    echo "<ul>"
    IFS='&' # Set Internal Field Separator to '&' for parsing query string
    for param in $QUERY_STRING; do
        key=$(echo $param | cut -d= -f1)
        value=$(echo $param | cut -d= -f2)
        echo "<li><b>$key</b>: $value</li>"
    done
    echo "</ul>"
else
    echo "<h3>No query string received</h3>"
fi

# Output current date and time
echo "<p>Current date and time: $(date)</p>"

# End HTML output
echo "</body>"
echo "</html>"
