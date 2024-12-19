<?php
// Set headers for CGI response
header('Content-Type: text/html; charset=utf-8');

// Function to sanitize input
function sanitizeInput($input) {
    return htmlspecialchars(trim($input));
}

// Function to write to file safely
function writeToFile($content, $filename) {
    try {
        // Create logs directory if it doesn't exist
        $dir = 'logs';
        if (!is_dir($dir)) {
            mkdir($dir, 0755, true);
        }
        
        // Full path to file
        $filepath = $dir . '/' . $filename;
        
        // Write content to file
        if (file_put_contents($filepath, $content . PHP_EOL, FILE_APPEND | LOCK_EX) === false) {
            throw new Exception("Failed to write to file");
        }
        
        return true;
    } catch (Exception $e) {
        return false;
    }
}

// Get input data (supports both GET and POST methods)
$input = isset($_POST['input']) ? $_POST['input'] : (isset($_GET['input']) ? $_GET['input'] : '');
$input = sanitizeInput($input);

// Process the input
if (!empty($input)) {
    $filename = 'output_' . date('Y-m-d') . '.txt';
    $success = writeToFile($input, $filename);
    
    // Return response
    if ($success) {
        echo json_encode([
            'status' => 'success',
            'message' => 'Data written successfully',
            'filename' => $filename
        ]);
    } else {
        echo json_encode([
            'status' => 'error',
            'message' => 'Failed to write data'
        ]);
    }
} else {
    echo json_encode([
        'status' => 'error',
        'message' => 'No input provided'
    ]);
}
?>
