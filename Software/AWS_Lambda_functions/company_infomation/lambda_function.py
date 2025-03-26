import json
import boto3
from decimal import Decimal

# Initialize DynamoDB
dynamodb = boto3.resource('dynamodb')
table = dynamodb.Table('SmartShelfCompanies')

def convert_decimal(obj):
    """ Convert Decimal to int or float in a JSON serializable way. """
    if isinstance(obj, Decimal):
        return int(obj) if obj % 1 == 0 else float(obj)
    if isinstance(obj, list):
        return [convert_decimal(i) for i in obj]
    if isinstance(obj, dict):
        return {k: convert_decimal(v) for k, v in obj.items()}
    return obj

def lambda_handler(event, context):
    try:
        # Extract company name from query parameters
        company_name = event.get('queryStringParameters', {}).get('companyName', None)

        if not company_name:
            return {
                "statusCode": 400,
                "body": json.dumps({"error": "Missing companyName parameter"})
            }

        # Query the database for company details
        response = table.get_item(Key={'CompanyName': company_name})

        if 'Item' not in response:
            return {
                "statusCode": 404,
                "body": json.dumps({"error": "Company not found"})
            }

        print("Response:", response['Item'])

        data = response['Item']

        # Convert Decimal to JSON-serializable types
        data_serialized = convert_decimal(data)

        print("Data:", data_serialized)

        return {
            "statusCode": 200,
            "body": json.dumps(data_serialized)
        }

    except Exception as e:
        return {
            "statusCode": 500,
            "body": json.dumps({"error": str(e)})
        }
