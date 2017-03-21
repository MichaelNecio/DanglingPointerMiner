openssl genrsa -out private.pem 1024
openssl rsa -in private.pem -outform PEM -pubout -out public.pem
openssl rsa -pubin -inform PEM -in public.pem -outform DER -out public.der
