#!/usr/bin/perl
use strict;
use warnings;
use CGI;
use CGI::Carp qw(fatalsToBrowser);

# Create new CGI object
my $cgi = CGI->new;

# Set content type and status code
print $cgi->header(
    -type    => 'text/html',
    -status  => '404 Not Found'
);

# HTML template for 404 page
print <<'HTML';
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>404 - Page Not Found</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            text-align: center;
            padding: 50px;
            background-color: #f5f5f5;
        }
        h1 {
            color: #333;
            font-size: 36px;
        }
        p {
            color: #666;
            font-size: 18px;
        }
        .error-code {
            font-size: 72px;
            color: #d9534f;
            margin: 20px 0;
        }
    </style>
</head>
<body>
    <div class="error-code">404</div>
    <h1>Page Not Found</h1>
    <p>The requested page could not be found on this server.</p>
    <p>Please check the URL or return to the <a href="/">homepage</a>.</p>
</body>
</html>
HTML

# End script
exit;
