import socket
import sys
import threading
import time

def handler(client):
    data = client.recv(1024).decode()
    method, file, *_ = data.split(" ")
    if method == "GET":
        if file == "/":
            file = "/index.html"
        filepath = "www" + file
        try:
            with open(filepath, "rb") as f:
                body = f.read()
            print("GET %s" % file)
            header = b"HTTP/1.1 200 OK\r\n\r\n"
        except FileNotFoundError:
            print("%s not found" % filepath)
            header = b"HTTP/1.1 404 OK\r\n\r\n"
            body = b"404"
        
        client.send(header+body)
        client.close()
        time.sleep(1)

def main():
    with socket.socket(family=socket.AF_INET, type=socket.SOCK_STREAM) as s:
        s.bind(("localhost", int(sys.argv[1])))
        s.listen(5)
        while True:
            client, address = s.accept()
            # handler(client)
            threading.Thread(target=handler, args=[client]).start()

if __name__ == "__main__":
    main()
