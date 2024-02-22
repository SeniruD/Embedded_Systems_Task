from flask import Flask, render_template,request, jsonify
import socket
from datetime import datetime

app = Flask(__name__)

webpage_data = {}

# IP address and port of the STM32 MCU
STM32_IP_ADDRESS = "192.168.1.111"
STM32_PORT = 80

# Function to send timing details to STM32
def send_timing_details_to_stm32(timing_details):

    timing_details_str = str(timing_details)
    try:
        # Connect to the STM32 MCU
        stm32_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        stm32_socket.connect((STM32_IP_ADDRESS, STM32_PORT))
        stm32_socket.send(timing_details_str.encode())
        stm32_socket.close()

    except Exception as e:
        print("Error sending timing details to STM32:", e)


@app.route('/get-time')
def get_timing_details():

    # Get the current time and format it
    current_time = datetime.now()
    formatted_time = current_time.strftime("[%d/%m/%Y %H:%M:%S]")
    timing_details = {"date": formatted_time} 
    send_timing_details_to_stm32(timing_details)
    return jsonify({"message": "Timing details received and sent to STM32"})

@app.route('/webpage_request', methods=['POST'])
def recive_webpage_request():
    global webpage_data
    webpage_data = request.json  # Get the JSON data from the POST request
    print("Received WebPage Data:", webpage_data)
    return jsonify(webpage_data)

@app.route('/')
def index():
    global webpage_data
    return render_template("index.html", webpage_data=webpage_data)

if __name__ == "__main__":
    app.run(host='0.0.0.0', port=5000,
    debug=True)


