# An example PageBuilder with HTML page stored SPIFFS.

This example shows how to reduce the size of program memory by storing the HTML source in ESP8266's flash file system and separating it from the sketch source code.  
Before executing this sketch, you need to upload the data folder of **libraries/PageBuilder/examples/FSPage** to SPIFFS of ESP8266 by the tool as "ESP8266 Sketch Data Upload" in Tools menu in Arduino IDE. Refer to [**Uploading file to file system**](https://arduino-esp8266.readthedocs.io/en/latest/filesystem.html#uploading-files-to-file-system) of [ESP8266 Arduino Core's documentation](https://arduino-esp8266.readthedocs.io/en/latest/index.html).

## What does this example do?
<img src="https://user-images.githubusercontent.com/12591771/34141303-aba4b284-e4c3-11e7-872b-69b9301cef10.png" style="float:right;width:225px;height:400px;margin-left:1.5em;margin-bottom:1.0em;border:1px solid #3366cc;"><img src="https://user-images.githubusercontent.com/12591771/34141214-14cb8cfc-e4c3-11e7-8afd-90efbdd6ac87.png" style="float:right;width:225px;height:400px;margin-left:1.0em;margin-bottom:1.0em;border:1px solid #3366cc;">It can connects ESP8266 to your existing an access point by using WIFI_AP_STA mode. Instead of embedded SSID and PSK in the sketch, you can connect to the access point using interactive operation via smartphone.  

SoftAP starts at the beginning of this sketch. The IP address is probably 192.168.4.1 fixed but you can change the sketch if you want other addresses. Start the sketch then open the WiFi setting on your smartphone. You will see **esp8266ap** in the Wi-Fi list. Connect to it with password as **12345678**. Once connected, you should be able to access 192.168.4.1 from your smartphone browser.  

<img src="https://user-images.githubusercontent.com/12591771/34141367-23577c26-e4c4-11e7-9e26-528497368e97.png" style="float:left;width:225px;height:400px;margin-right:1.5em;margin-top:1.0em;margin-bottom:1.0em;border:1px solid #3366cc">A page such as a screenshot will be responded from ESP8266 as a scan result of the nearby WiFi signal. It is the button list of the SSID where the signal was found. Each signal strength and encryption status are also displayed together and **"?"** is indicated for hidden access point.  
Taps the SSID or "?" in the list you want to connect. Enter the passphrase for that SSID on the following page. If you tap the hidden SSID, you also need to enter the SSID.  **Apply** button will start the connection attempt with that SSID.  

<img src="https://user-images.githubusercontent.com/12591771/34141438-94224f6c-e4c4-11e7-9dd1-52582e10975d.png" style="float:right;margin-left:1.5em;margin-top:1.0em;margin-bottom:1.0em;width:225px;height:400px;border:1px solid #3366cc"><img src="https://user-images.githubusercontent.com/12591771/34141320-cf390af6-e4c3-11e7-9bbd-44d7f80aa076.png" style="float:right;margin-left:1.0em;margin-top:1.0em;margin-bottom:1.0em;width:225px;height:400px;border:1px solid #3366cc">When the connection is successful, the welcome page with new IP address which joined the network of its access point is displayed. The response route goes through the access point you have just connected. If the access point is connected to Internet, you can modify the sketch like accessing Internet site directly from ESP8266. Also if it could not be connected, **'Failed page'** will appear. <div style="clear:none;"></div>

## What is the effect of the PageBuilder brought with SPIFFS?

The size of the HTML of this example is about 6,500 bytes. On the other hand, the sketch size is 276,005 bytes. The sketch using the **ESP8266WebServer** class with empty setup() and loop() is even consumed 227,445 bytes. And more CSS codes are required to improve operability on smartphones. If you want to do more complicated Web page, you probably need to include JavaScript too. It is not preferable to incorporate that code into the sketch. The test takes too much work and the sketch size becomes bigger.  
With PageBuilder you can relatively easily place the HTML source in external files.  
**Note:** But eventually, a size of String instance that populates into memory is not practical, it is limited by heap size for large complex HTML.

## Hints - How it works 

This section is not related to PageBuilder's case. It is only a commentary on the program structure of this example sketch. I hope that this hint will help somewhat if you are thinking something similar to this example. However, this hint may not be accurate in some cases.  

The following figure explains about http request sequence between ESP8266WebServer instance and Client of inside this example sketch.<img src="https://user-images.githubusercontent.com/12591771/34141467-cb29e542-e4c4-11e7-9d06-f515383823e4.png" style="float:right;margin-left:1.5em;margin-top:2.0em;margin-bottom:1.0em;">  
ESP8266 initially started up in SoftAP with STA mode and waits for a request from the client. PageBuilder is implemented inheriting **RequestHandler** class attached ESP8266WebServer. Therefore, it is invoked from **handleClient()** in **loop()** function.  

### Step 1

The client issues the initial http request. The PageBuilder returns **http response 302** for this request as redirect to **/result** uri and it sets an internal flag **CONN_REQ** for starting **WiFi.begin** procedure.

### Step 2

The client will issue a request to **/result** after receiving the **302** response. On ESP8266 side, **handleClient()** was already ended and the inside process of **loop()** has been started. It starts **WiFi.begin** caused by **CONN_REQ** flag affection. Namely, **WiFi.begin** attempts STA mode for connecting to specified SSID is executed on the outside of series of the process in **handleClient()**. Http response will not be returned until WiFi.begin completed and CONN_REQ returns the value to false in the end of connWiFi().

### Step 3

When WiFi.begin completes, two kinds of http responses would be returned according to the WiFi connection result. Both are responses redirect indicated 302, but their destinations are different.  
In case of success, the location would  be addressed **/welcome** subordinate to **WiFi.localIP** (from parent access point). It should be the network segment of the connected access point. It is not 192.168.4.1. In failure case, redirect to **/failed** and it located still **softAPIP** probably 192.168.4.1.

### Step 4

The client will request a page which indicated connection result according to the redirect response. It is either **http\://`{WiFi.localIP()}`/welcome** or **http\://`{WiFi.softAPIP()}`/failed**. At this time the page returns the **response 200**.

#### Note:  
Please be careful when quoting this and please let me know if you find some misunderstanding of the recognition for the connection sequence with http requests and ESP8266WebServer.