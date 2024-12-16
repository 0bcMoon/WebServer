<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>File Upload</title>
    <style>
        body { 
            font-family: Arial, sans-serif; 
            max-width: 400px; 
            margin: 0 auto; 
            padding: 20px; 
        }
        form {
            background-color: #f4f4f4;
            padding: 20px;
            border-radius: 8px;
        }
        input[type="file"] {
            width: 100%;
            margin-bottom: 15px;
        }
        input[type="submit"] {
            width: 100%;
            padding: 10px;
            background-color: #4CAF50;
            color: white;
            border: none;
            border-radius: 4px;
        }
    </style>
</head>
<body>
    <form action="/post.php" method="POST" enctype="multipart/form-data">
        <h2>Upload File</h2>
        <input type="file" name="file" required>
        <input type="submit" value="Upload File">
    </form>
</body>
</html>