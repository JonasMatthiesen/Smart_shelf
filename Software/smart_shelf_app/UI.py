from flask import Flask, request, jsonify, render_template
from pprint import pprint
import json
import boto3
import requests
from API import fetch_shelf_data

app = Flask(__name__)

# AWS IoT Region
AWS_REGION = "eu-west-1"
# API Gateway URL & Key
API_COMPANY_INFO_URL = "https://m9ab5gi6o4.execute-api.eu-west-1.amazonaws.com/smart_shelf_company/companyInfo"
API_UPDATE_INFO_URL = "https://hz2hc4sr80.execute-api.eu-west-1.amazonaws.com/smart_shelf_stage/"
API_KEY = "H0fQFzrA2V5VWhOCX1ifd8JcDEvJQPsZ4rqX1iJU"

def get_smart_shelves(username):
    """Fetch Smart Shelf data from API dynamically."""
    try:
        response = requests.get(API_COMPANY_INFO_URL, params={"companyName": username})
        if response.status_code == 200:
            data = response.json()
            body_data = json.loads(data["body"])  # Decode the nested JSON

            total_shelves = body_data.get("TotalShelves", 0)
            smart_shelves = body_data.get("SmartShelfNames", [])  # Keep list format

            return total_shelves, smart_shelves
        else:
            print(f"API Error: {response.status_code} - {response.text}")
            return 0, []  # Default values on failure

    except requests.exceptions.RequestException as e:
        print("Request failed:", e)
        return 0, []

@app.route('/')
def home():
    """ Serves the login page """
    return render_template("index.html")

@app.route('/fetch_shelf_data', methods=['GET'])
def fetch_shelf_data_api():
    """ Fetches Smart Shelf Data and returns it as JSON """

    smart_shelf_id = request.args.get("smart_shelf_id", "")

    print(f"Fetching data for: {smart_shelf_id}")  # Debugging

    structured_data = fetch_shelf_data(smart_shelf_id)

    # print("\n📦 Structured Smart Shelf Data:")
    # pprint(structured_data, sort_dicts=False)  # Pretty-print the structured JSON

    if not structured_data:
        return jsonify({"error": "No data found"}), 404

    return jsonify(structured_data)

@app.route('/login/<username>', methods=['GET', 'POST'])
def login(username):
    formatted_username = username.replace("-", " ")

    # ✅ Fetch Smart Shelf Data from API instead of hardcoding
    total_shelves, smart_shelves = get_smart_shelves(username)

    if request.method == 'POST':
        action = request.form.get('action')  # Get which button was pressed
        topic = request.form.get("selected_topic", "")

        

        if action == "calibrate":
            # ✅ Send calibration message
            message_payload = {
                "body": json.dumps({
                    "topic": topic,
                    "smart_shelf_1_name": "",
                    "smart_shelf_2_name": "",
                    "smart_shelf_3_name": "",
                    "weight_item_1": 0.0,
                    "weight_item_2": 0.0,
                    "weight_item_3": 0.0,
                    "limit_1": 0.0,
                    "limit_2": 0.0,
                    "limit_3": 0.0,
                    "calibrate_offset": int(0),
                    "calibrate_scalar": 0.0,
                    "calibrate": True,  # 🔹 Set calibrate to True
                    "wifi": False  # 🔹 Regular update (not WiFi)
                })
            }
            print("🔧 Calibration triggered:", message_payload)

        elif action == "wifi":
            # ✅ Send WiFi message
            message_payload = {
                "body": json.dumps({
                    "topic": topic,
                    "smart_shelf_1_name": "",
                    "smart_shelf_2_name": "",
                    "smart_shelf_3_name": "",
                    "weight_item_1": 0.0,
                    "weight_item_2": 0.0,
                    "weight_item_3": 0.0,
                    "limit_1": 0.0,
                    "limit_2": 0.0,
                    "limit_3": 0.0,
                    "calibarte_offset": 0.0,
                    "calibarte_scalar": int(0),
                    "calibrate": False,  # 🔹 Regular update (not calibration)
                    "wifi": True  # 🔹 Regular update (not WiFi)
                })
            }
            print("📡 WiFi message triggered:", message_payload)

        else:
            # ✅ Regular form submission
            def convert_to_float(value):
                try:
                    return float(value) if value else 0.0
                except ValueError:
                    return 0.0

            message_payload = {
                "body": json.dumps({
                    "topic": topic,
                    "smart_shelf_1_name": request.form.get('smart_shelf_1_name', ''),
                    "smart_shelf_2_name": request.form.get('smart_shelf_2_name', ''),
                    "smart_shelf_3_name": request.form.get('smart_shelf_3_name', ''),
                    "weight_item_1": convert_to_float(request.form.get('weight_item_1', '')),
                    "weight_item_2": convert_to_float(request.form.get('weight_item_2', '')),
                    "weight_item_3": convert_to_float(request.form.get('weight_item_3', '')),
                    "limit_1": convert_to_float(request.form.get('limit_1', '')),
                    "limit_2": convert_to_float(request.form.get('limit_2', '')),
                    "limit_3": convert_to_float(request.form.get('limit_3', '')),
                    "calibarte_offset": 0,
                    "calibarte_scalar": int(0),
                    "calibrate": False,  # 🔹 Regular update (not calibration)
                    "wifi": False  # 🔹 Regular update (not WiFi)
                })
            }
            print("✅ Regular update triggered:", message_payload)

        # ✅ Send message to API
        headers = {"x-api-key": API_KEY}
        response = requests.post(API_UPDATE_INFO_URL, headers=headers, json=message_payload)

        print("📡 Message sent to AWS IoT:", response.text)

        return render_template("login.html", username=formatted_username, smart_shelves=smart_shelves, success=True)

    return render_template("login.html", username=formatted_username, smart_shelves=smart_shelves, total_shelves=total_shelves)

if __name__ == '__main__':
    app.run(debug=True)
