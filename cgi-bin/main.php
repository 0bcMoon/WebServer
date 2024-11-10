<?php
// Method 1: Using header() function
header("HTTP/1.1 404 Not Found");

// Method 2: Using http_response_code() function (PHP 5.4+)
// http_response_code(404);

// // Example function to set status code with error handling
// function setStatusCode($code) {
//     $valid_codes = array(
//         200 => 'OK',
//         201 => 'Created',
//         400 => 'Bad Request',
//         401 => 'Unauthorized',
//         403 => 'Forbidden',
//         404 => 'Not Found',
//         500 => 'Internal Server Error',
//         503 => 'Service Unavailable'
//     );
//     
//     if (isset($valid_codes[$code])) {
//         if (!headers_sent()) {
//             header("HTTP/1.1 $code " . $valid_codes[$code]);
//             return true;
//         }
//     }
//     return false;
// }

// // Usage examples:
// // 1. Basic 404 error
// setStatusCode(404);

// // 2. API response with status code
// function apiResponse($data, $status = 200) {
//     setStatusCode($status);
//     header('Content-Type: application/json');
//     echo json_encode($data);
//     exit;
// }

// // Example API usage
// $response = ['error' => 'Resource not found'];
// apiResponse($response, 404);cho "hello worl from php";
