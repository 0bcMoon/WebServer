<?php
// Simple PHP CGI script to print "Hello, World!"
// Set the status code to 404
header("HTTP/1.1 404i");

// Optionally, set additional custom headers
header("X-Error-Message: Resource not found");

// Now output the custom error page
echo "<h1>404 - Page Not Found</h1>";
echo "<p>The page you're looking for doesn't exist.</p>";

?>
