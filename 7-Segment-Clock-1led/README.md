### Important

Create a file in the same folder as this file with the name "Credentials.h" and add and change the following code to be able to connect to your Wi-Fi network:

```
#define UN "your-ssid-username"
#define PW "your-ssid-password"
```

curl 'http://192.168.1.61/brightness' --data-raw 'brightness=10'
