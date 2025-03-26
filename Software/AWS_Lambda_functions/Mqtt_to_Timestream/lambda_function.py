import json
import boto3
import time
from send_email import send_low_stock_email
from startup import startup_message

# Initialize Timestream client
timestream = boto3.client('timestream-write')

# Define database and table names
DATABASE_NAME = "demoThingDB"
TABLE_NAME = "smart_shelf_data_table"
# TABLE_NAME = "test"

def lambda_handler(event, context):
    current_time = int(time.time() * 1000)  # Keep the same timestamp for all records

    # Extract Smart Shelf ID
    smart_shelf_id = event.get("smart_shelf_id", "unknown_shelf")

    # Extract shelves array
    shelves = event.get("shelves", [])
    startup = event.get("startup", False)

    # Extract calibration parameters
    calibrate_offset = event.get("calibrate_offset", None)
    calibrate_scalar = event.get("calibrate_scalar", None)

    records = []

    print(f"Received data from: {smart_shelf_id}")
    # print("Shelves Data:", json.dumps(shelves, indent=2))

    if startup:
        print("Startup data received. Sending data down to device")
        json_message = startup_message(smart_shelf_id)

        return {
            'statusCode': 200,
            'body': json_message
        }
        

    if not shelves:
        print("No shelves data received. Skipping processing.")
        return {
            'statusCode': 200,
            'body': json.dumps('No shelves data received. Skipping processing.')
        }

    # ✅ Add calibration parameters to Timestream records
    if calibrate_offset is not None:
        records.append({
            'Dimensions': [
                {'Name': 'smart_shelf_id', 'Value': str(smart_shelf_id)}
            ],
            'MeasureName': 'calibrate_offset_d',
            'MeasureValue': str(calibrate_offset),
            'MeasureValueType': 'DOUBLE',
            'Time': str(current_time),
            'TimeUnit': 'MILLISECONDS'
        })

    if calibrate_scalar is not None:
        records.append({
            'Dimensions': [
                {'Name': 'smart_shelf_id', 'Value': str(smart_shelf_id)}
            ],
            'MeasureName': 'calibrate_scalar',
            'MeasureValue': str(calibrate_scalar),
            'MeasureValueType': 'DOUBLE',
            'Time': str(current_time),
            'TimeUnit': 'MILLISECONDS'
        })

    for shelf in shelves:
        try:
            # Ensure all required attributes exist
            shelf_id = shelf.get("shelf_id")
            if isinstance(shelf_id, int):  
                shelf_id_str = str(shelf_id)  # Convert to string for Timestream
            else:
                raise ValueError(f"shelf_id is not an integer: {shelf_id}")

            name = shelf.get("name", "unknown_name")
            weight = shelf.get("weight", 0)
            weight_of_one_item = shelf.get("weight_of_one_item", 1)
            items = shelf.get("items", 0)
            limit = shelf.get("limit", 0)


            # Debugging: Print the extracted values
            print(f"Processing Shelf {shelf_id} ({smart_shelf_id}) - name: {name}, weight: {weight}, weight_of_one_item: {weight_of_one_item}, items: {items}, limit: {limit}")

            # send email
            send_low_stock_email(name, items, limit, smart_shelf_id)


            # ✅ Ensure dimensions match **EXACTLY** with Timestream's partition key schema
            timestream_records = [
                {
                    'Dimensions': [
                        {'Name': 'smart_shelf_id', 'Value': str(smart_shelf_id)},  # Ensure it's a string
                        {'Name': 'shelf_id', 'Value': shelf_id_str},  # Convert shelf_id to string
                        {'Name': 'name', 'Value': str(name)}  # Ensure name is a string
                    ],
                    'MeasureName': 'weight',
                    'MeasureValue': str(weight),
                    'MeasureValueType': 'DOUBLE',
                    'Time': str(current_time),
                    'TimeUnit': 'MILLISECONDS'
                },
                {
                    'Dimensions': [
                        {'Name': 'smart_shelf_id', 'Value': str(smart_shelf_id)},
                        {'Name': 'shelf_id', 'Value': shelf_id_str},
                        {'Name': 'name', 'Value': str(name)}
                    ],
                    'MeasureName': 'weight_of_one_item',
                    'MeasureValue': str(weight_of_one_item),
                    'MeasureValueType': 'DOUBLE',
                    'Time': str(current_time),
                    'TimeUnit': 'MILLISECONDS'
                },
                {
                    'Dimensions': [
                        {'Name': 'smart_shelf_id', 'Value': str(smart_shelf_id)},
                        {'Name': 'shelf_id', 'Value': shelf_id_str},
                        {'Name': 'name', 'Value': str(name)}
                    ],
                    'MeasureName': 'items',
                    'MeasureValue': str(items),
                    'MeasureValueType': 'DOUBLE',
                    'Time': str(current_time),
                    'TimeUnit': 'MILLISECONDS'
                },
                {
                    'Dimensions': [
                        {'Name': 'smart_shelf_id', 'Value': str(smart_shelf_id)},
                        {'Name': 'shelf_id', 'Value': shelf_id_str},
                        {'Name': 'name', 'Value': str(name)}
                    ],
                    'MeasureName': 'limit',
                    'MeasureValue': str(limit),
                    'MeasureValueType': 'DOUBLE',
                    'Time': str(current_time),
                    'TimeUnit': 'MILLISECONDS'
                }
            ]

            # Debugging: Print the record before writing to Timestream
            # print("Timestream Record:", json.dumps(timestream_records, indent=2))

            # Append records to the batch
            records.extend(timestream_records)

        except Exception as e:
            print(f"Error processing shelf {shelf}: {str(e)}")

    print("Timestream Records:", json.dumps(records, indent=2))

    # Write batch to Timestream
    if records:
        try:
            response = timestream.write_records(
                DatabaseName=DATABASE_NAME,
                TableName=TABLE_NAME,
                Records=records
            )
            print("Timestream Write Response:", response)

        except boto3.exceptions.Boto3Error as e:
            print("Error writing to Timestream:", e)

        except timestream.exceptions.RejectedRecordsException as e:
            print("Timestream Rejected Records Error:", e)
            if hasattr(e, 'response') and 'RejectedRecords' in e.response:
                print("Rejected Records Details:", json.dumps(e.response['RejectedRecords'], indent=2))

        except Exception as e:
            print("General Error writing to Timestream:", e)

    return {
        'statusCode': 200,
        'body': json.dumps({"message": "Data processed successfully", "records_written": len(records)}, indent=4)
    }
