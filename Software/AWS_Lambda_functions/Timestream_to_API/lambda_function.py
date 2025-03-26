import json
import boto3

client = boto3.client('timestream-query')

DATABASE_NAME = "demoThingDB"
TABLE_NAME = "smart_shelf_data_table"

def lambda_handler(event, context):
    print("Received event:", json.dumps(event))  # Debugging: Print event data

    # Extract 'smart_shelf_id' from API Gateway query parameters
    query_params = event.get("queryStringParameters") or {}

    if not query_params or "smart_shelf_id" not in query_params:
        return {
            'statusCode': 400,
            'body': json.dumps({"message": "Missing required parameter: smart_shelf_id"})
        }

    smart_shelf_id = query_params["smart_shelf_id"]

    # smart_shelf_id = "smart_shelf_1"

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
        SELECT time, smart_shelf_id, shelf_id, name, measure_name, measure_value::double
        FROM "{DATABASE_NAME}"."{TABLE_NAME}"
        WHERE "time" = '{latest_time}' 
        AND "smart_shelf_id" = '{smart_shelf_id}' 
        AND "measure_name" IN ('items', 'weight', 'weight_of_one_item', 'limit')
    """

    response = client.query(QueryString=query)

     # Extract and format response as a valid JSON object
    latest_records = []
    for row in response.get("Rows", []):
        record = {}
        columns = ["time", "smart_shelf_id", "shelf_id", "name", "measure_name", "measure_value"]

        for i, col in enumerate(row["Data"]):
            record[columns[i]] = col.get("ScalarValue", None)  # Extract values

        latest_records.append(record)

    print(f"Decoded Response at {latest_time} for smart_shelf_id {smart_shelf_id}: {latest_records}")

    return {
        'statusCode': 200,
        'headers': {"Content-Type": "application/json"},
        'body': json.dumps(latest_records)
    }
