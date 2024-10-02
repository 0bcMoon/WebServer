#!/Users/hibenouk/.brew/bin/python3.11

import requests
import json
import argparse
from beeprint import pp

def send_request(method, url, headers, body):
    try:
        headers = json.loads(headers) if headers else {}
        data = json.loads(body) if body else None
        response = requests.request(method, url, headers=headers, json=data, timeout=5)
        for k, v in response.headers.items():
            print(k, ":" , v)
        print(response.content)
        return response
        # return (f"Status Code: {response.status_code}\n\n"
        #         f"Headers:\n{json.dumps(dict(response.headers), indent=2)}\n\n"
        #         f"Body:\n{response.text}")
    except Exception as e:
        return f"Error: {str(e)}"

def main():
    parser = argparse.ArgumentParser(description="Send HTTP requests to localhost")
    parser.add_argument("--url", default="http://localhost:8080", help="URL to send the request to")
    parser.add_argument("--method", default="GET", choices=["GET", "POST", "PUT", "DELETE"], help="HTTP method")
    parser.add_argument("--headers", default="{}", help="Headers as JSON string")
    parser.add_argument("--body", default="{}", help="Request body as JSON string")
    
    args = parser.parse_args()

    response  = send_request(args.method, args.url, args.headers, args.body)

if __name__ == "__main__":
    main()

