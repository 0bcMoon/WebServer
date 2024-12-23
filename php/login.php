<?php
	session_start();
?>
<!DOCTYPE html>
<html>
<head>
    <title>Login Form</title>
    <style>
        .container {
            width: 300px;
            margin: 100px auto;
            padding: 20px;
            border: 1px solid #ccc;
            border-radius: 5px;
        }
        .form-group {
            margin-bottom: 15px;
        }
        label {
            display: block;
            margin-bottom: 5px;
        }
        input[type="text"], input[type="password"] {
            width: 100%;
            padding: 8px;
            border: 1px solid #ddd;
            border-radius: 4px;
        }
        input[type="submit"] {
            background-color: #4CAF50;
            color: white;
            padding: 10px 15px;
            border: none;
            border-radius: 4px;
            cursor: pointer;
        }
        .error {
            color: red;
            margin-top: 10px;
        }

        .info {
            color: green;
            margin-top: 10px;
        }
    </style>

</head>
<body>
    <div class="container">
        <?php
        if ($_SERVER["REQUEST_METHOD"] == "POST") {
            $username = $_POST["username"];
            $password = $_POST["password"];
            
			try {
				$db = new SQLite3('db');
				$query = "SELECT * FROM users";
				$results = $db->query($query);

				while ($row = $results->fetchArray(SQLITE3_ASSOC)) {
					echo $row['username'] . ": " . $row['email'] . "\n";
			}
			} catch (Exception $e) {
				echo "Connection failed: " . $e->getMessage();
			}

			if ($username === "hicham" && $password === "12344") {
				$_SESSION['username'] = $username;
				header("Location: home.php");
				exit();
			}
			else {
				echo '<div class="error">Login failed! Invalid username or password.</div>';
			}
        }
		if ($_SERVER["REQUEST_METHOD"] == "GET") 
		{
			if (isset($_GET['msg']))
			{
				$msg = $_GET["msg"];
				echo "<div class='info'>$msg.</div>";
			}
		}
        ?>
        
        <form method="post" action="<?php echo htmlspecialchars($_SERVER["PHP_SELF"]); ?>">
            <div class="form-group">
                <label for="username">Username:</label>
                <input type="text" id="username" name="username" required>
            </div>
            
            <div class="form-group">
                <label for="password">Password:</label>
                <input type="password" id="password" name="password" required>
            </div>
            
            <input type="submit" value="Login">
        </form>
    </div>
</body>
</html>
