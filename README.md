# Summary of Task Completion

## 1. HTTP WebServer Implementation on MCU
a. developed a web server on the MCU.
b. Created a webpage interface for controlling the LEDs
c. Control the LEDs via the web page.  

## 2. STM32 Setup as Client and Python Flask Web Server  
a. Developed an external web server using Python Flask. 
b. Configured STM32 as a client to communicate with the external web server.  
c. Created a web interface to display webpage request details and also send time data to the MCU.

## Issue Encountered

- The MCU, acting as a client, faced difficulties in receiving data from the external server.   
- Although the connection to the web server was established successfully, only the initial request of the webpage data was sent to the external server. Subsequent requests are not received by the external server.   

## Incomplete Tasks:

1. Sending time details from the external server to the MCU for time syncing.
2. Continuous updating of web request data from the MCU.  

### Fig 1: Webpage interface for controlling the LEDs
![Webpage interface for LED control](https://imgur.com/Cb1M9ni.png)
### Fig 2: Web Interface for displaying webpage request data and sync time
![Webpage request data display](https://imgur.com/ulNjI9Z.png)
