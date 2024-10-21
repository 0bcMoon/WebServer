
#!/bin/bash

# Output the HTTP header
echo "Content-type: text/html"
echo ""

# Start HTML output
echo "<html><body>"

# Get content length (size of the POST data)
CONTENT_LENGTH=$CONTENT_LENGTH

# Read the POST data (stdin)
if [ "$CONTENT_LENGTH" -gt 0 ]; then
    read -n "$CONTENT_LENGTH" POST_DATA
    echo "<h3>POST Data Received:</h3>"
    echo "<p>$POST_DATA</p>"

    # Parse and display each POST parameter (assuming application/x-www-form-urlencoded)
    echo "<ul>"
    IFS='&' # Set field separator to &
    for param in $POST_DATA; do
        key=$(echo $param | cut -d= -f1)
        value=$(echo $param | cut -d= -f2)
        echo "<li><b>$key</b>: $value</li>"
    done
    echo "</ul>"
else
    echo "<h3>No POST data received</h3>"
fi

# End HTML output
echo "</body></html>"
