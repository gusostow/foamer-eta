# AWS IoT Setup Guide

This guide covers setting up AWS IoT Core infrastructure for the foamer-eta ESP32-S3 display device, including OTA firmware updates and CloudWatch logging.

## Table of Contents

1. [Deploy IoT Infrastructure](#deploy-iot-infrastructure)
2. [Create Device Certificate](#create-device-certificate)
3. [Register IoT Thing](#register-iot-thing)
4. [Configure Firmware](#configure-firmware)
5. [Testing Connection](#testing-connection)
6. [OTA Firmware Updates](#ota-firmware-updates)
7. [CloudWatch Logging](#cloudwatch-logging)

## Deploy IoT Infrastructure

The CDK `IoTStack` provisions the core AWS resources needed for device management.

### 1. Deploy the Stack

```bash
cd cdk
cdk deploy FoamerIoTDev  # or FoamerIoTProd
```

This creates:
- S3 bucket for firmware storage
- CloudWatch Log Group for device logs
- IoT Topic Rule routing device logs to CloudWatch
- IoT Policy defining device MQTT permissions
- IAM roles for logging and OTA updates

### 2. Note the Outputs

After deployment, save these outputs:
- **IoTEndpoint**: AWS IoT MQTT broker endpoint
- **DevicePolicyName**: Policy to attach to certificates
- **FirmwareBucketName**: S3 bucket for OTA firmware
- **DeviceLogGroupName**: CloudWatch log destination

## Create Device Certificate

Each device needs a unique X.509 certificate for authentication.

### Method 1: AWS Console (Easiest)

1. Open [AWS IoT Console](https://console.aws.amazon.com/iot)
2. Navigate to **Security** → **Certificates** → **Create certificate**
3. Choose **Auto-generate a new certificate**
4. Download all files:
   - `xxx-certificate.pem.crt` (Device certificate)
   - `xxx-private.pem.key` (Private key)
   - `xxx-public.pem.key` (Public key - optional)
   - Amazon Root CA 1 from [here](https://www.amazontrust.com/repository/AmazonRootCA1.pem)
5. Click **Activate** to enable the certificate

### Method 2: AWS CLI

```bash
# Create certificate
aws iot create-keys-and-certificate \
  --set-as-active \
  --certificate-pem-outfile device-cert.pem \
  --private-key-outfile private-key.pem \
  --public-key-outfile public-key.pem

# Download Amazon Root CA 1
curl -o root-ca.pem https://www.amazontrust.com/repository/AmazonRootCA1.pem
```

**IMPORTANT**: Store these files securely. The private key cannot be retrieved after creation.

## Register IoT Thing

Create an IoT Thing to represent your physical device.

### Using AWS Console

1. Navigate to **Manage** → **All devices** → **Things** → **Create things**
2. Choose **Create single thing**
3. Thing name: `foamer-display-001` (or your chosen name)
4. No thing type needed
5. Skip shadow for now (auto-created on first use)
6. Attach the certificate you created earlier
7. Attach the policy `foamer-dev-device-policy` (from CDK outputs)

### Using AWS CLI

```bash
# Get certificate ARN from previous step
CERT_ARN="arn:aws:iot:us-east-1:ACCOUNT_ID:cert/CERT_ID"

# Get policy name from CDK output
POLICY_NAME="foamer-dev-device-policy"

# Create thing
aws iot create-thing --thing-name foamer-display-001

# Attach certificate to thing
aws iot attach-thing-principal \
  --thing-name foamer-display-001 \
  --principal $CERT_ARN

# Attach policy to certificate
aws iot attach-policy \
  --policy-name $POLICY_NAME \
  --target $CERT_ARN
```

## Configure Firmware

Add the AWS IoT configuration to your firmware profile.

### 1. Update Profile JSON

Edit `firmware/profiles/dev.json` (or `prod.json`):

```json
{
  "wifi": { ... },
  "api": { ... },
  "geo": { ... },
  "display": { ... },
  "aws_iot": {
    "enabled": true,
    "endpoint": "xxxxxxxxxxxxx-ats.iot.us-east-1.amazonaws.com",
    "thing_name": "foamer-display-001",
    "log_topic": "device/foamer-display-001/logs",
    "cert_pem": "-----BEGIN CERTIFICATE-----\n[paste certificate]\n-----END CERTIFICATE-----",
    "private_key": "-----BEGIN RSA PRIVATE KEY-----\n[paste key]\n-----END RSA PRIVATE KEY-----",
    "root_ca": "-----BEGIN CERTIFICATE-----\n[paste root CA]\n-----END CERTIFICATE-----"
  }
}
```

**Note**: Replace newlines in certificates with `\n` when pasting into JSON.

### 2. Get IoT Endpoint

```bash
aws iot describe-endpoint --endpoint-type iot:Data-ATS
```

The endpoint format is: `ACCOUNT_ID-ats.iot.REGION.amazonaws.com`

### 3. Format Certificates

Convert PEM files to JSON-safe format:

```bash
# On macOS/Linux
awk 'NF {sub(/\r/, ""); printf "%s\\n",$0;}' device-cert.pem
awk 'NF {sub(/\r/, ""); printf "%s\\n",$0;}' private-key.pem
awk 'NF {sub(/\r/, ""); printf "%s\\n",$0;}' root-ca.pem
```

## Testing Connection

### 1. Monitor MQTT Test Client

1. Open [AWS IoT Console](https://console.aws.amazon.com/iot)
2. Navigate to **Test** → **MQTT test client**
3. Subscribe to: `device/foamer-display-001/logs`

### 2. Flash and Boot Device

```bash
cd /Users/aostow/dev/foamer-eta
make upload PROFILE=dev
```

### 3. Monitor Serial Output

Watch for connection messages:
```
Connecting to AWS IoT...
Connected to AWS IoT endpoint
```

### 4. Verify Logs in CloudWatch

```bash
aws logs tail /aws/iot/foamer-dev/devices --follow
```

## OTA Firmware Updates

### Prerequisites

- Firmware built and uploaded to S3
- Device connected and subscribed to job topics

### 1. Build and Upload Firmware

```bash
# Build firmware
make compile PROFILE=prod

# Upload to S3 (get bucket name from CDK outputs)
aws s3 cp firmware/foamer-display/.pio/build/adafruit_matrixportal_esp32s3/firmware.bin \
  s3://foamer-dev-firmware/firmware-v1.0.0.bin
```

### 2. Create OTA Job

```bash
# Get OTA role ARN from CDK outputs
OTA_ROLE_ARN="arn:aws:iam::ACCOUNT_ID:role/FoamerIoTDev-IoTOTARole..."

# Create job document
cat > job-document.json <<EOF
{
  "operation": "firmware_update",
  "version": "1.0.0",
  "files": [{
    "fileName": "firmware.bin",
    "fileSource": {
      "fileId": "1",
      "location": {
        "s3Location": {
          "bucket": "foamer-dev-firmware",
          "key": "firmware-v1.0.0.bin",
          "version": "latest"
        }
      }
    }
  }],
  "roleArn": "$OTA_ROLE_ARN"
}
EOF

# Create IoT job
aws iot create-job \
  --job-id firmware-update-v1.0.0 \
  --targets arn:aws:iot:us-east-1:ACCOUNT_ID:thing/foamer-display-001 \
  --document file://job-document.json \
  --description "Firmware update to v1.0.0"
```

### 3. Monitor Job Status

```bash
# Check job status
aws iot describe-job --job-id firmware-update-v1.0.0

# Watch device job execution
aws iot describe-job-execution \
  --job-id firmware-update-v1.0.0 \
  --thing-name foamer-display-001
```

## CloudWatch Logging

### MQTT Log Format

Devices should publish structured JSON logs:

```json
{
  "timestamp": 1234567890,
  "level": "INFO",
  "message": "WiFi connected",
  "metadata": {
    "rssi": -45,
    "ip": "192.168.1.100"
  }
}
```

### Query Logs

```bash
# Tail logs
aws logs tail /aws/iot/foamer-dev/devices --follow

# Query with CloudWatch Insights
aws logs start-query \
  --log-group-name /aws/iot/foamer-dev/devices \
  --start-time $(date -u -d '1 hour ago' +%s) \
  --end-time $(date -u +%s) \
  --query-string 'fields @timestamp, level, message | filter level = "ERROR" | sort @timestamp desc'
```

## Troubleshooting

### Certificate Issues

**Problem**: Device fails to connect with TLS error

**Solution**:
- Verify certificate is active: `aws iot describe-certificate --certificate-id CERT_ID`
- Check policy is attached: `aws iot list-attached-policies --target CERT_ARN`
- Ensure thing principal is attached: `aws iot list-thing-principals --thing-name foamer-display-001`

### Time Sync Issues

**Problem**: Certificate validation fails

**Solution**: ESP32 requires accurate time for certificate validation. Ensure NTP sync completes before connecting to AWS IoT.

### Permission Denied

**Problem**: Device connects but cannot publish/subscribe

**Solution**: Review IoT policy. Ensure policy uses policy variables like `${iot:Connection.Thing.ThingName}` correctly.

### OTA Fails to Download

**Problem**: Job created but device doesn't receive firmware

**Solution**:
- Check S3 bucket permissions for OTA role
- Verify firmware file exists: `aws s3 ls s3://foamer-dev-firmware/`
- Monitor job execution status: `aws iot describe-job-execution ...`

## Security Best Practices

1. **Certificate Rotation**: Rotate device certificates annually
2. **Least Privilege**: IoT policy grants minimum required permissions
3. **Firmware Signing**: Consider AWS IoT Code Signing for production
4. **Secure Storage**: Store private keys in device NVS partition, not app partition
5. **Monitoring**: Set up CloudWatch alarms for failed connection attempts
6. **Secrets**: Never commit certificates to git (use profiles/*.json in .gitignore)

## Next Steps

- [ ] Implement MQTT client in firmware (PubSubClient library)
- [ ] Add NTP time synchronization for certificate validation
- [ ] Implement OTA download handler in firmware
- [ ] Add rollback mechanism for failed OTA updates
- [ ] Set up CloudWatch alarms for device health monitoring
