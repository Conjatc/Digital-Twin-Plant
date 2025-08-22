import json
import boto3
import base64

s3 = boto3.client('s3')
bucket_name = 'plantdatabase'
log_file = 'sensor_log.json'

def lambda_handler(event, context):
    # The IoT Rule delivers the message as 'event'
    data = event

    # Check which topic this came from
    topic = event.get('topic', '')

    # If you don't have topic in the payload, you can decide by field names
    if 'temp' in data and 'moist' in data:
        store_sensor_status(data)
        return {'statusCode': 200, 'body': 'Sensor data stored.'}

    elif 'i' in data and 't' in data:
        store_image(data)
        return {'statusCode': 200, 'body': 'Image stored.'}

    else:
        return {'statusCode': 400, 'body': 'Unknown format.'}

def store_sensor_status(data):
    try:
        response = s3.get_object(Bucket=bucket_name, Key=log_file)
        log_data = json.loads(response['Body'].read())
    except s3.exceptions.NoSuchKey:
        log_data = []

    log_data.append(data)

    s3.put_object(
        Bucket=bucket_name,
        Key=log_file,
        Body=json.dumps(log_data, indent=2)
    )

def store_image(data):
    image_bytes = base64.b64decode(data['i'])
    safe_timestamp = data['t'].replace(":", "-").replace(" ", "_")
    filename = f"images/{safe_timestamp}.jpg"

    s3.put_object(
        Bucket=bucket_name,
        Key=filename,
        Body=image_bytes,
        ContentType='image/jpeg'
    )
