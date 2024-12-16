<?php
// Ensure the script is running in a CGI/CLI context.
if (php_sapi_name() !== 'cgi-fcgi') {
    header('Content-Type: text/plain');
    echo "Error: This script must be run in a CGI environment.\n";
    exit(1);
}

// Set appropriate headers for the response.
header('Content-Type: text/plain');

// Read the raw POST body.
$postBody = file_get_contents('php://input');

if ($postBody === false) {
    echo "Error reading POST body.\n";
    exit(1);
}

// Output the POST body for debugging (optional, careful with binary data).
echo "Received POST body:\n";
echo $postBody;

// Optionally save or process the POST body.
file_put_contents('post_body.bin', $postBody);
?>
