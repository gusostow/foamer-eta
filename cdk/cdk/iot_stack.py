from aws_cdk import (
    Stack,
    RemovalPolicy,
    Duration,
    CfnOutput,
    aws_iot as iot,
    aws_s3 as s3,
    aws_logs as logs,
    aws_iam as iam,
)
from constructs import Construct


class IoTStack(Stack):
    """
    AWS IoT Core resources for ESP32-S3 device management.

    This stack provisions:
    - IoT Thing (device registration)
    - IoT Policy (MQTT permissions)
    - S3 bucket for OTA firmware updates
    - CloudWatch Logs for device logging
    - IoT Topic Rules for log routing
    """

    def __init__(
        self, scope: Construct, construct_id: str, env_name: str, **kwargs
    ) -> None:
        super().__init__(scope, construct_id, **kwargs)

        # S3 bucket for OTA firmware storage
        firmware_bucket = s3.Bucket(
            self,
            "FirmwareBucket",
            bucket_name=f"foamer-{env_name}-firmware",
            versioned=True,  # Enable versioning for rollback capability
            encryption=s3.BucketEncryption.S3_MANAGED,
            removal_policy=RemovalPolicy.RETAIN,  # Always retain firmware
            lifecycle_rules=[
                s3.LifecycleRule(
                    enabled=True,
                    noncurrent_version_expiration=Duration.days(90),
                )
            ],
        )

        # CloudWatch Log Group for device logs
        device_log_group = logs.LogGroup(
            self,
            "DeviceLogGroup",
            log_group_name=f"/aws/iot/foamer-{env_name}/devices",
            retention=logs.RetentionDays.ONE_MONTH
            if env_name == "dev"
            else logs.RetentionDays.THREE_MONTHS,
            removal_policy=RemovalPolicy.DESTROY
            if env_name == "dev"
            else RemovalPolicy.RETAIN,
        )

        # IAM role for IoT to write to CloudWatch Logs
        iot_logs_role = iam.Role(
            self,
            "IoTLogsRole",
            assumed_by=iam.ServicePrincipal("iot.amazonaws.com"),
            description="Allows AWS IoT to write device logs to CloudWatch",
        )

        device_log_group.grant_write(iot_logs_role)

        # IoT Topic Rule to route device logs to CloudWatch
        # Devices publish to: device/{thingName}/logs
        iot.CfnTopicRule(
            self,
            "DeviceLogsTopicRule",
            topic_rule_payload=iot.CfnTopicRule.TopicRulePayloadProperty(
                sql="SELECT * FROM 'device/+/logs'",
                description="Route device logs to CloudWatch Logs",
                actions=[
                    iot.CfnTopicRule.ActionProperty(
                        cloudwatch_logs=iot.CfnTopicRule.CloudwatchLogsActionProperty(
                            log_group_name=device_log_group.log_group_name,
                            role_arn=iot_logs_role.role_arn,
                            batch_mode=False,  # Stream logs immediately
                        )
                    )
                ],
                rule_disabled=False,
            ),
        )

        # IAM role for IoT Jobs to access S3 for OTA updates
        iot_ota_role = iam.Role(
            self,
            "IoTOTARole",
            assumed_by=iam.ServicePrincipal("iot.amazonaws.com"),
            description="Allows AWS IoT Jobs to read firmware from S3 for OTA updates",
        )

        firmware_bucket.grant_read(iot_ota_role)

        # IoT Policy for device MQTT permissions
        # This policy will be attached to device certificates
        device_policy = iot.CfnPolicy(
            self,
            "DevicePolicy",
            policy_name=f"foamer-{env_name}-device-policy",
            policy_document={
                "Version": "2012-10-17",
                "Statement": [
                    {
                        "Effect": "Allow",
                        "Action": ["iot:Connect"],
                        "Resource": [
                            f"arn:aws:iot:{self.region}:{self.account}:client/${{iot:Connection.Thing.ThingName}}"
                        ],
                    },
                    {
                        "Effect": "Allow",
                        "Action": ["iot:Publish"],
                        "Resource": [
                            # Allow publishing logs
                            f"arn:aws:iot:{self.region}:{self.account}:topic/device/${{iot:Connection.Thing.ThingName}}/logs",
                            # Allow publishing to shadow update topics
                            f"arn:aws:iot:{self.region}:{self.account}:topic/$aws/things/${{iot:Connection.Thing.ThingName}}/shadow/update",
                            # Allow publishing OTA job status
                            f"arn:aws:iot:{self.region}:{self.account}:topic/$aws/things/${{iot:Connection.Thing.ThingName}}/jobs/*/update",
                        ],
                    },
                    {
                        "Effect": "Allow",
                        "Action": ["iot:Subscribe"],
                        "Resource": [
                            # Allow subscribing to OTA job notifications
                            f"arn:aws:iot:{self.region}:{self.account}:topicfilter/$aws/things/${{iot:Connection.Thing.ThingName}}/jobs/*",
                            # Allow subscribing to shadow delta
                            f"arn:aws:iot:{self.region}:{self.account}:topicfilter/$aws/things/${{iot:Connection.Thing.ThingName}}/shadow/*",
                        ],
                    },
                    {
                        "Effect": "Allow",
                        "Action": ["iot:Receive"],
                        "Resource": [
                            # Allow receiving OTA job documents
                            f"arn:aws:iot:{self.region}:{self.account}:topic/$aws/things/${{iot:Connection.Thing.ThingName}}/jobs/*",
                            # Allow receiving shadow updates
                            f"arn:aws:iot:{self.region}:{self.account}:topic/$aws/things/${{iot:Connection.Thing.ThingName}}/shadow/*",
                        ],
                    },
                    {
                        "Effect": "Allow",
                        "Action": ["iot:UpdateThingShadow", "iot:GetThingShadow"],
                        "Resource": [
                            f"arn:aws:iot:{self.region}:{self.account}:thing/${{iot:Connection.Thing.ThingName}}"
                        ],
                    },
                ],
            },
        )

        # Output IoT endpoint (devices need this to connect)
        CfnOutput(
            self,
            "IoTEndpoint",
            value=f"{self.account}.iot.{self.region}.amazonaws.com",
            description=f"AWS IoT endpoint for {env_name} devices",
            export_name=f"foamer-{env_name}-iot-endpoint",
        )

        # Output firmware bucket name
        CfnOutput(
            self,
            "FirmwareBucketName",
            value=firmware_bucket.bucket_name,
            description=f"S3 bucket for firmware storage ({env_name})",
            export_name=f"foamer-{env_name}-firmware-bucket",
        )

        # Output device policy name for certificate attachment
        CfnOutput(
            self,
            "DevicePolicyName",
            value=device_policy.policy_name,
            description=f"IoT policy to attach to device certificates ({env_name})",
            export_name=f"foamer-{env_name}-device-policy",
        )

        # Output log group name
        CfnOutput(
            self,
            "DeviceLogGroupName",
            value=device_log_group.log_group_name,
            description=f"CloudWatch Log Group for device logs ({env_name})",
            export_name=f"foamer-{env_name}-device-logs",
        )

        # Output OTA role ARN
        CfnOutput(
            self,
            "OTARoleArn",
            value=iot_ota_role.role_arn,
            description=f"IAM role for IoT OTA jobs ({env_name})",
            export_name=f"foamer-{env_name}-ota-role-arn",
        )
