import json
import boto3

client = boto3.client('timestream-query')

iot_client = boto3.client("iot-data", region_name="eu-west-1")

DATABASE_NAME = "demoThingDB"
TABLE_NAME = "smart_shelf_data_table"

def startup_message(smart_shelf_id):

    # First, get the latest timestamp for the specified smart shelf
    latest_time_query = f"""
        SELECT MAX("time") AS latest_time
        FROM "{DATABASE_NAME}"."{TABLE_NAME}"
        WHERE "smart_shelf_id" = '{smart_shelf_id}'
    """

    latest_time_response = client.query(QueryString=latest_time_query)

    if not latest_time_response["Rows"]:
        return {
            'statusCode': 404,
            'headers': {"Content-Type": "application/json"},
            'body': json.dumps({"message": f"No data found for smart_shelf_id: {smart_shelf_id}"})
        }

    # Extract the latest timestamp
    latest_time = latest_time_response["Rows"][0]["Data"][0]["ScalarValue"]

    print(f"Latest timestamp for smart_shelf_id {smart_shelf_id}: {latest_time}")

    # Now, get all rows with that latest timestamp
    query = f"""
        SELECT shelf_id, name, measure_name, measure_value::double
        FROM "{DATABASE_NAME}"."{TABLE_NAME}"
        WHERE "time" = '{latest_time}' 
        AND "smart_shelf_id" = '{smart_shelf_id}'
        AND "measure_name" IN ('weight_of_one_item', 'limit', 'calibrate_scalar', 'calibrate_offset_d')
    """

    response = client.query(QueryString=query)

    latest_records = []
    for row in response.get("Rows", []):
        record = {}
        columns = ["shelf_id", "name", "measure_name", "measure_value"]

        for i, col in enumerate(row["Data"]):
            record[columns[i]] = col.get("ScalarValue", None)  # Extract values

        latest_records.append(record)

    print(f"Latest records: {latest_records}")

    json_message = {
        "topic": f"device/{smart_shelf_id}/sub",
        "smart_shelf_1_name": "",
        "smart_shelf_2_name": "",
        "smart_shelf_3_name": "",
        "weight_item_1": 0.0,
        "weight_item_2": 0.0,
        "weight_item_3": 0.0,
        "limit_1": 0.0,
        "limit_2": 0.0,
        "limit_3": 0.0,
        "calibrate_offset": 0,
        "calibrate_scalar": 0.0,
        "calibrate": False,
        "wifi": False
    }

    for record in latest_records:
        if not isinstance(record, dict):  # Ensure record is a dictionary
            print(f"Skipping invalid record: {record}")  # Debugging
            continue

        measure_name = record.get("measure_name")
        measure_value = record.get("measure_value")

        if measure_value is not None:
            try:
                measure_value = float(measure_value)  # Convert measure_value to float if present
            except ValueError:
                print(f"Skipping record due to invalid measure_value: {measure_value}")
                continue

        # Handle global calibration values (do NOT fetch shelf_id)
        if measure_name in ["calibrate_offset_d", "calibrate_scalar"]:
            if measure_name == "calibrate_scalar":
                json_message["calibrate_scalar"] = measure_value
            elif measure_name == "calibrate_offset_d":
                json_message["calibrate_offset"] = measure_value
            continue  # Skip further processing since shelf_id is not relevant

        # Now, safely get shelf_id for other records
        shelf_id = record.get("shelf_id")
        name = record.get("name")

        # If shelf_id is None or empty, skip the record
        if not shelf_id:
            print(f"Skipping record with missing shelf_id: {record}")
            continue

        try:
            shelf_id = int(shelf_id)  # Convert only if it's a valid shelf ID
        except (ValueError, TypeError):
            print(f"Skipping invalid shelf_id: {shelf_id}")
            continue  # Skip this record if conversion fails

        print(f"Processing shelf-specific record: {record}")

        # Assign names to shelves
        json_message[f"smart_shelf_{shelf_id}_name"] = name

        # Assign weight, limit, and calibration values to correct shelf slots
        if measure_name == "weight_of_one_item":
            json_message[f"weight_item_{shelf_id}"] = measure_value
        elif measure_name == "limit":
            json_message[f"limit_{shelf_id}"] = measure_value



    # Publish message to MQTT
    mqtt_publish(json_message)

    return json_message


def mqtt_publish(payload):
    topic = payload.get("topic")

    if not topic:
        return {
            "statusCode": 400,
            "body": json.dumps({"error": "Missing 'topic' in request."})
        }

    print(f"Publishing to topic: {topic}")

    try:
        response = iot_client.publish(
            topic=topic,
            qos=1,
            payload=json.dumps(payload)
        )
        print(f"MQTT Publish Response: {response}")
    except Exception as e:
        print(f"Error publishing to MQTT: {str(e)}")
        return {
            "statusCode": 500,
            "body": json.dumps({"error": str(e)})
        }
