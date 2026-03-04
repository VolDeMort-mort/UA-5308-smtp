For command STARTTLS to work in the Server, there should be files "server.key" and "server.crt" in the certs directory

STARTTLS tested via "openssl s_client -starttls smtp -connect localhost:25000 -quiet"

For test they can be genereted with these commands:
openssl genrsa -out server.key 2048
openssl req -x509 -new -key server.key -out server.crt -days 365 -subj "/CN=localhost" -addext "subjectAltName=DNS:localhost,IP:127.0.0.1"