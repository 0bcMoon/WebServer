#!/Users/hibenouk/.brew/bin/python3.11

import requests

def main():
    # URL to send the GET request to
    url = "http://localhost:8080"

    try:
        # Send GET request
        response = requests.get(url)
        
        # Check if the request was successful
        response.raise_for_status()

        # Print the status code
        print(f"Status Code: {response.status_code}")

        # Print the response content
        print("Response Content:")
        print(response.json())

    except requests.RequestException as e:
        print(f"An error occurred: {e}")

if __name__ == "__main__":
    main()
