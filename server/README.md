For command STARTTLS to work in the Server, there should be files "server.key" and "server.crt" in the certs directory

For test they can be genereted with these commands
(Windows PowerShell)
openssl genrsa -out server.key 2048
openssl req -x509 -new -key server.key -out server.crt -days 365 -subj "/CN=localhost" -addext "subjectAltName=DNS:localhost,IP:127.0.0.1"