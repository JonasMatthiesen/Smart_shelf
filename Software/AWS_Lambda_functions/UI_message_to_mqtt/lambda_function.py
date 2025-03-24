import json
import boto3

iot_client = boto3.client("iot-data", region_name="eu-west-1")

def lambda_handler(event, context):
    try:
        # Extract data from API Gateway request
        payload = json.loads(event["body"])
        topic = payload.get("topic")  # Get the topic dynamically
        print(f"Topic: {topic}")

        if not topic:
            return {
                "statusCode": 400,
                "body": json.dumps({"error": "Missing 'topic' in request."})
            }

        # Publish message to the dynamic MQTT topic
        response = iot_client.publish(
            topic=topic,
            qos=1,
            payload=json.dumps(payload)
        )

        return {
            "statusCode": 200,
            "body": json.dumps({
                "message": f"Published successfully to topic: {topic}",
                "response": response
            })
        }
    
    except Exception as e:
        return {
            "statusCode": 500,
            "body": json.dumps({"error": str(e)})
        }
