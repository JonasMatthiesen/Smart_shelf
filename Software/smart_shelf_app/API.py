import requests
import json
from collections import OrderedDict  # Use OrderedDict for sorted grouping
from collections import defaultdict  # Use defaultdict for nested dictionaries

# API Gateway URL & Key
API_URL = "https://z15rivu3vh.execute-api.eu-west-1.amazonaws.com/Smart_Shelf_stage"
API_KEY = "H0fQFzrA2V5VWhOCX1ifd8JcDEvJQPsZ4rqX1iJU"

# Define a struct-like class to store shelf data
class SmartShelfData:
    def __init__(self, smart_shelf_id):
        self.smart_shelf_id = smart_shelf_id
        self.shelves = defaultdict(dict)  # Stores shelf_id as key

    def add_entry(self, time, shelf_id, name, measure_name, measure_value):
        """ Adds a new measurement to the shelf. """
        shelf = self.shelves.setdefault(shelf_id, {
            "name": name,
            "time": time,
            "items": "N/A",
            "weight": "N/A",
            "weight_of_one_item": "N/A",
            "limit": "N/A"
        })
        shelf[measure_name] = measure_value  # Set the corresponding measurement

    def to_dict(self):
        """ Converts the class object into a serializable dictionary. """
        return {self.smart_shelf_id: self.shelves}

def fetch_shelf_data(smart_shelf_id):
    """
    Fetches and groups Smart Shelf data from AWS API Gateway.

    Args:
        smart_shelf_id (str): The ID of the smart shelf.

    Returns:
        dict: A structured dictionary representing the smart shelf data.
    """
    headers = {"x-api-key": API_KEY}
    params = {"smart_shelf_id": smart_shelf_id}

    try:
        response = requests.get(API_URL, headers=headers, params=params)

        if response.status_code == 200:
            response_data = response.json()

            if "body" in response_data:
                data = json.loads(response_data["body"])
            else:
                data = response_data

            # Initialize a SmartShelfData object
            smart_shelf = SmartShelfData(smart_shelf_id)

            for item in data:
                smart_shelf.add_entry(
                    time=item["time"],
                    shelf_id=item["shelf_id"],
                    name=item["name"],
                    measure_name=item["measure_name"],
                    measure_value=item["measure_value"]
                )

            return smart_shelf.to_dict()  # Return as a dictionary

        else:
            print(f"Error: {response.status_code}, Message: {response.text}")
            return None

    except requests.exceptions.RequestException as e:
        print("Request failed:", e)
        return None

