from flask import Flask, render_template,request, jsonify

app = Flask(__name__)

# Initialize sensor data as an empty dictionary
sensor_data = {}

@app.route("/get-user/<user_id>")
def get_user(user_id):
    user_data = {"id": user_id, "name": "John Doe"}
    extra = request.args.get("extra")
    if extra:
        user_data["extra"] = extra
        
    return jsonify(user_data),200

@app.route('/receive_sensor_data', methods=['POST'])
def receive_sensor_data():
    global sensor_data
    sensor_data = request.json  # Assuming data is sent in JSON format
    # Process and store the sensor data as needed
    print("Received Sensor Data:", sensor_data)
    return jsonify(sensor_data), 201

@app.route("/")
def index():
    global sensor_data
    return render_template("index.html", sensor_data=sensor_data)

if __name__ == "__main__":
    app.run(host='0.0.0.0')

