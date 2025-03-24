import boto3
import os

# Initialize DynamoDB
dynamodb = boto3.resource('dynamodb')
table = dynamodb.Table('SmartShelfCompanies')

# Initialize AWS SES client
ses_client = boto3.client('ses', region_name="eu-west-1")  # Change the region if needed

def send_low_stock_email(name, items, limit, smart_shelf_id):
    """
    Sends an email alert if the item count is below the limit.
    
    :param name: Name of the item or shelf
    :param items: Current number of items
    :param limit: Threshold limit for the item
    :param smart_shelf_id: ID of the smart shelf for lookup in DynamoDB
    """
    if items < limit:
        # Get sender email from environment variable
        sender_email = os.getenv("SENDER_EMAIL")
        if not sender_email:
            raise ValueError("SENDER_EMAIL environment variable is not set or is empty")

        # Scan the table with a filter expression
        response = table.scan(
            FilterExpression="contains(SmartShelfNames, :shelf_id)",
            ExpressionAttributeValues={":shelf_id": smart_shelf_id}
        )

        item = response.get("Items", [])
        if not item:
            raise ValueError(f"No company found for SmartShelf ID: {smart_shelf_id}")

        company_name = item[0].get("CompanyName", "Unknown Company")
        recipient_email = item[0].get("Email", "")
        if not recipient_email:
            raise ValueError(f"No email found for company {company_name}")

        print(f"{company_name} - {recipient_email}")

        subject = f"Low Stock Alert: {name}"
        body = (
            f"Warning! The stock for '{name}' is running low.\n\n"
            f"Current count: {items}\nLimit: {limit}\n\n"
            "Please restock soon."
            f"\n\nBest regards,\nSmartShelf Team"
        )

        # Send email using AWS SES
        try:
            response = ses_client.send_email(
                Source=sender_email,
                Destination={'ToAddresses': [recipient_email]},
                Message={
                    'Subject': {'Data': subject},
                    'Body': {'Text': {'Data': body}}
                }
            )
            return response  # Return the response if successful
        except Exception as e:
            print(f"⚠️ Failed to send email to {recipient_email}: {e}")  # Log the error
            return {"error": f"Failed to send email to {recipient_email}", "details": str(e)}  # Continue execution


    else:
        return {"message": f"Stock is sufficient for {name} ({items}/{limit})"}
