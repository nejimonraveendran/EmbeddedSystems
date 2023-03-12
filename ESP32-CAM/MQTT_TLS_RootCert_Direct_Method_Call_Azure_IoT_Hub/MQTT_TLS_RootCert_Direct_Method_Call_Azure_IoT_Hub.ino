//board manager: https://dl.espressif.com/dl/package_esp32_index.json
//esp32 board lib ver: 1.0.6 (esp32 by Espressif Systems)

#include <WiFiClientSecure.h>
#include <PubSubClient.h>

//wifi settings:
const char* _ssid = "your_wifi_ssid";
const char* _password = "your_wifi_password";

//Azure IoT Hub  MQTT settings
const char* _mqttServer = "<host>"; //IoT Hub host name, eg: myhub.azure-devices.net
const int _mqttPort = 8883; 
const char* _deviceId = "<device_id>"; //device ID configured on IoT Hub, eg: mydevice1
const char* _mqttUserName = "<host>/<device_id>/?api-version=2021-04-12"; //eg: myhub.azure-devices.net/mydevice1/?api-version=2021-04-12
const char* _mqttPassword = ""; //no password required because we are using certificate-based auth
const char* _subTopic = "devices/<device_id>/messages/devicebound/#";  //eg: devices/mydevice1/messages/devicebound/#
const char* _subDirectMethod = "$iothub/methods/POST/#";

//root server certificate
//get certificate: openssl s_client -showcerts -connect <your_mqtt_server>:<port> > server_cert.pem  
//following example shows Azure IoT Hub root certificate (DigiCert Global Root G2)  
const char* _rootCertAzureIotHub PROGMEM = R"CERT(
-----BEGIN CERTIFICATE-----
MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH
MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI
2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx
1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ
q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz
tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ
vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP
BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV
5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY
1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4
NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG
Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91
8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe
pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl
MrY=
-----END CERTIFICATE-----
)CERT";

//client certificate specific to myiotdevice01
//documentation: https://github.com/nejimonraveendran/AzureIoT/blob/main/CertBasedAuth.md
const char* _clientCert PROGMEM = R"CERT(
-----BEGIN CERTIFICATE-----
MIIDWTCCAkGgAwIBAgIRAKX25BYCZ0UOiLH2q1l33UMwDQYJKoZIhvcNAQELBQAw
GDEWMBQGA1UEAwwNTXlPcmcgUm9vdCBDQTAeFw0yMzAzMDExODQ1MjVaFw0zMzAy
MjYxODQ1MjVaMEExCzAJBgNVBAYTAkNBMQswCQYDVQQIDAJPTjEOMAwGA1UECgwF
TXlPcmcxFTATBgNVBAMMDG15aW90ZGV2aWNlMTCCASIwDQYJKoZIhvcNAQEBBQAD
ggEPADCCAQoCggEBALk8dgnrctY4flT/T76I2upTn3HKVE2UxJHNnEjNh5GHOLVW
fIKkR7zpm7eB4vLYeeIJUwtWi6Dmir36wFDw0IaTS/yA4y6iSfsi7/phv6Q35WVP
0PvcDUhZG6cCq6tp+07WhD+fJOvRELOusuc4IMpZ6mYWqtUEuxnicqNuocx6g60U
df9teltlQGtIXoFiuojldjluorejKjkfldifaaXQDE+4mOF9+mC/ii+e788lXiBQ
I7mja2tsaVwF8gsZJfb3UkhZFO064eXghMx4bSp8UOIW9EvJsJc+BVomJlNDu+vg
SavI3k1JPV4f3wiTeQYpBr0T6NwmnxjR9KAcdx0CAwEAAaN1MHMwHwYDVR0jBBgw
FoAUEdhl+9w7o+czhGELLuMEDVLN08EwDAYDVR0TAQH/BAIwADATBgNVHSUEDDAK
BggrBgEFBQcDAjAOBgNVHQ8BAf8EBAMCB4AwHQYDVR0OBBYEFIDIf7j+CK3M/B8M
t0fHALgsYe4HMA0GCSqGSIb3DQEBCwUAA4IBAQCLpmfURjsMf7lrzoli7ISpNb9o
W0sBiAvlhUGxNPd0gffrerccfrcfEfTdDL7tRwZ8KoJlIwom+dF208vLr+biXHaJ
mDaQrmgFucgflOr5APYL+eZli0wkM+dnu9allSUPA3LqokXgeTUZ0adBzWLytduA
zN4aOa3Bdc4u8/awUHIVPJYc9TpkpRjEwDlDl7oMJIedSr1WeV1C+QEl+G15e9vD
vSZC72q+MZwlJIFl7O04ZVn1z/OwQbpvf9yj/TgHg+pIrstw1mNVXNLFbjK9yvb0
F4xhXwoEKPFlsHtLWgDNYctO6a6nIdDG6X6YNBjVlHUilI5MLq8CqyTmFSmB
-----END CERTIFICATE-----
)CERT";

//client certificate's private key
//documentation: https://github.com/nejimonraveendran/AzureIoT/blob/main/CertBasedAuth.md
const char* _clientPrivateKey PROGMEM = R"CERT(
-----BEGIN PRIVATE KEY-----
MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQC5PHYJ63LWOH5U
/0++iNrqU59xylRNlMSRzZxIzYeRhzi1VnyCpEe86Zu3geLy2HniCVMLVoug5oq9
+sBQ8NCGk0v8gOMuokn7Iu/6Yb+kN+VlT9D73A1IWRunAquraftO1oQ/nyTr0RCz
rrLnOCDKWepmFqrVBLsZ4nKjbqHMeoOtFHX/bXpbZUBrSF6BbMRn58omJ5rTmgpf
qRq9e2ml0AxPuJjhffpgv4ovnu/PJV4gUCO5o2trbGlcBfILGSX291JIWRTtOuHl
4ITMeG0qfFDiFvRLybCXPgVaJiZTQ7vr4EmryN5NST1eH98Ik3kGKQa9E+jcJp8Y
0fSgHHcdAgMBAAECggEBAKHRs7yOt091IsfWEi/9FmFGlC+v9V2g7OgCmtju09PK
MTwZRcRLB3B9E8+H5F8JxdA1HRr2jBwPynyUpZUQCtrFqsf3QH/ovoBp0QMGiYhI
XFnZFFQxtFjTn1iS+K1SdHw24mlB+dsjkjfkjorlclcljqmIe8yDyX+fV052w1oA
3kPkCnI1wQ6x1M7TDxDVZ57djOeVDsr/jm/gjv/ElC2mCzE/c4ZSRqW4GxgqChzZ
FhRhHoae0TmAgdQTT1ubk+eVSCiJodKV99OntOFEE4u8vY+3LMs95CbKrc4ik/DC
X4qjfQUN8Cff4dkFZnerlNeRoaX1fhBVwMxMz27yDAECgYEA8+XXqdWjc4EDowL3
HFF2M/PMl4rgwwiulkfjlkorlclorlckdt58AGeXQg2r8RVPU4WjOsfq/bJmkzOu
TBReZOFuy7wU3dx88kKQ4QJUkNFi1cRyli7mUat/xz3tIdPsuCTDul3iiOcoXgl7
MXMRq4HpIhjUU6fO68ghXRlFaN0CgYEAwm11SdZC66D5BU2rCXjKy6welEIW06HR
OA2waqNbJia4Q9riz0B9vfvz/3GAFhJpDMebtWqRq7Iccn6+Z6wYvxo/9sFqSJMc
3QLy4acCDYjxYARG0onxqg5kquyOJRagOFuM646Y9Dwmce/S/J3i5xjerZvT9rEm
mUg4/ZEHQ0ECgYAplZitxlU2Dls2Yf0MAP+sX/cUJ4Om/b6MQJRwEXoEy8kn5p6t
RA3Ua9UoV/hnBIEf9nscPJXeZ5qBoqAnLJnjjWQ8rHbKVRugmqTlGGaewnSdif8X
iCuQ5TLGH9Lcn6uOwgX1Mqi7Ot5xGiC9D6eFnILJY35gPQ5UuSynGPXyqQKBgHSZ
fjh8xX9X91+F16gDh+MjYudYxyW+KqGqG/2n3/b+S41uDI1sSLySCB9OaxlRiyx9
3NSsh5XnaAYGGpxD6kdZL7kE/39ssmec6BoJtUJ2uQLzLaH/1i09GoQi/5AghI+n
39g4mWQHWqsDv41+BmC/WnPVOwX78YEFfiAbRLbBAoGAKLs/vY+GtBG84lc4YxBo
1/Vjh/oG/HhetBuSOcLkTTRizKUXpGmg9UvYA9MRwzUGO44/2PeqCcK+qoSO1oI7
aCSMS+aSuDeMtHva4gUpuXQU+VwGy3UpYLyTwY/EZGJ1/dgyTnlbNjIZSjTHieau
sZINvfojonO67ATVyaluF18=
-----END PRIVATE KEY-----
)CERT";


WiFiClientSecure _wifiClient;
PubSubClient _mqttClient(_wifiClient);

int _flashLightPin = 4; //the buil-in flash light LED on ESP33-CAM

void setup() {
  pinMode(_flashLightPin, OUTPUT);
  digitalWrite(_flashLightPin, LOW); //keep the light off

  Serial.begin(115200);
  
  connectToWifi();

  _wifiClient.setCACert(_rootCertAzureIotHub); //set TLS server certificate
  _wifiClient.setCertificate(_clientCert); // set client certificate for auth
  _wifiClient.setPrivateKey(_clientPrivateKey);	// set client certificate private key

  _mqttClient.setServer(_mqttServer, _mqttPort);
  _mqttClient.setCallback(messageReceivedHandler);

}


void loop() {
  if(!_mqttClient.connected()){
    connectToMqttServer();
  }

  _mqttClient.loop();
}


void connectToWifi(){
  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(_ssid);

  //WiFi.mode(WIFI_STA);
  //WiFi.config(_ip, _gateway, _subnet, _dns); 
  WiFi.begin(_ssid, _password);
   
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected, IP Address: ");
  Serial.print(WiFi.localIP());  
  Serial.println();
  
  delay(100);
}


void connectToMqttServer() {  
  // Loop until we're reconnected
  while (!_mqttClient.connected()) {
    Serial.println("Attempting MQTT connection...");
    // Attempt to connect
    if (_mqttClient.connect(_deviceId, _mqttUserName, _mqttPassword)) {
      Serial.println("MQTT connected.");
      
      _mqttClient.subscribe(_subDirectMethod); //subscribe to all Cloud2Device direct method calls.
      
    } else {
      Serial.print("Failed to connect, connection state = ");
      Serial.println(_mqttClient.state());
      Serial.println(". Trying again in 5 seconds...");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void messageReceivedHandler(char* topic, byte* payload, unsigned int length) {
  if(length <= 0){
    Serial.println("Empty message received");
    return;    
  }
  
  String message((char*)payload);
  message = message.substring(0, length);
  message.replace("\"", ""); //replace any surrounding quotes (this occurs in Direct Method calls)
  
  Serial.printf("Message received, topic: [%s]. Message: [%s]", topic, message.c_str());
  Serial.println();  

  String directMethodIdentifier = "$iothub/methods/POST/";
  String ridIdentifier = "/?$rid=";
  
  String topicName(topic);
  if(topicName.indexOf(directMethodIdentifier) < 0 ){ //not a Direct Method call.
    Serial.println("Not a method call");
    return;
  }  

  String methodName = topicName.substring(topicName.indexOf(directMethodIdentifier) + directMethodIdentifier.length(), topicName.lastIndexOf(ridIdentifier));
  String requestId = topicName.substring(topicName.indexOf(ridIdentifier) + ridIdentifier.length());
  String responseUrl = "$iothub/methods/res/200" + ridIdentifier + requestId;
  String responsePayload = "";

  Serial.println("Receiced Method: " + methodName);
  Serial.println("Receiced Request Id: " + requestId);
  
  if(methodName == "OnOff"){
    if(message == "c:on") //command:on
    {
      digitalWrite(_flashLightPin, HIGH); //turn light on
      Serial.println("Light turned on");
    } else if(message == "c:off") //command:off
    {
      digitalWrite(_flashLightPin, LOW); //turn light off
      Serial.println("Light turned off");
    }

    responsePayload = "{\"status\":\"success\"}";

  }else{    
    responsePayload = "{\"status\":\"invalid method\"}";
  }

  Serial.println("Response [Url: " + responseUrl + ", Payload: " + responsePayload + "]");
  publishMessage(responseUrl.c_str(), responsePayload.c_str());
  
}

void publishMessage(const char* topic, const char* message){
  _mqttClient.publish(topic, message); 
}
