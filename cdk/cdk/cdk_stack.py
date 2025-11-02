from aws_cdk import (
    Stack,
    RemovalPolicy,
    Duration,
    CfnOutput,
    aws_secretsmanager as secretsmanager,
    aws_lambda as lambda_,
    aws_apigateway as apigw,
    aws_dynamodb as dynamodb,
    aws_s3 as s3,
)
from constructs import Construct


class CdkStack(Stack):
    def __init__(
        self, scope: Construct, construct_id: str, env_name: str, **kwargs
    ) -> None:
        super().__init__(scope, construct_id, **kwargs)

        # Secrets Manager secret for Transit API key
        transit_key_secret = secretsmanager.Secret(
            self,
            "TransitKeySecret",
            secret_name=f"foamer/{env_name}/transit-key",
            description=f"Transit API key for {env_name} environment",
            removal_policy=RemovalPolicy.DESTROY
            if env_name == "dev"
            else RemovalPolicy.RETAIN,
        )

        # Secrets Manager secret for shared API password
        shared_password_secret = secretsmanager.Secret(
            self,
            "SharedPasswordSecret",
            secret_name=f"foamer/{env_name}/shared-password",
            description=f"Shared password for API access in {env_name} environment",
            removal_policy=RemovalPolicy.DESTROY
            if env_name == "dev"
            else RemovalPolicy.RETAIN,
        )

        # DynamoDB table for messages
        messages_table = dynamodb.Table(
            self,
            "MessagesTable",
            table_name=f"foamer-{env_name}-messages",
            partition_key=dynamodb.Attribute(
                name="id", type=dynamodb.AttributeType.STRING
            ),
            billing_mode=dynamodb.BillingMode.PAY_PER_REQUEST,
            removal_policy=RemovalPolicy.DESTROY
            if env_name == "dev"
            else RemovalPolicy.RETAIN,
            point_in_time_recovery=env_name == "prod",
        )

        # Lambda function using pre-built cargo-lambda output
        # Build with: cargo lambda build -p svc --release --x86-64
        lambda_function = lambda_.Function(
            self,
            "FoamerSvcFunction",
            function_name=f"foamer-{env_name}-svc",
            runtime=lambda_.Runtime.PROVIDED_AL2023,
            handler="not.used",  # Not used with Lambda Web Adapter
            code=lambda_.Code.from_asset("../target/lambda/svc"),
            timeout=Duration.seconds(30),
            memory_size=512,
            environment={
                "RUST_LOG": "svc=debug,api=debug",
                "AWS_LAMBDA_EXEC_WRAPPER": "/opt/bootstrap",
                "PORT": "8080",
            },
            layers=[
                # Lambda Web Adapter layer (us-east-1)
                lambda_.LayerVersion.from_layer_version_arn(
                    self,
                    "LambdaWebAdapterLayer",
                    layer_version_arn=f"arn:aws:lambda:{self.region}:753240598075:layer:LambdaAdapterLayerX86:22",
                )
            ],
        )

        # Add TRANSIT_KEY secret to Lambda
        transit_key_secret.grant_read(lambda_function)
        lambda_function.add_environment(
            "TRANSIT_KEY", transit_key_secret.secret_value.unsafe_unwrap()
        )

        # Grant Lambda access to shared password secret
        shared_password_secret.grant_read(lambda_function)
        lambda_function.add_environment(
            "FOAMER_SECRET", shared_password_secret.secret_value.unsafe_unwrap()
        )

        # Grant Lambda access to DynamoDB table and add table name to environment
        messages_table.grant_read_write_data(lambda_function)
        lambda_function.add_environment(
            "FOAMER_MESSAGES_TABLE", messages_table.table_name
        )

        # API Gateway REST API
        api = apigw.RestApi(
            self,
            "FoamerRestApi",
            rest_api_name=f"foamer-{env_name}-api",
            description=f"REST API for foamer {env_name} environment",
            deploy_options=apigw.StageOptions(
                stage_name="prod",
                throttling_rate_limit=10,
                throttling_burst_limit=20,
            ),
        )

        # Lambda integration
        lambda_integration = apigw.LambdaIntegration(
            lambda_function,
            proxy=True,
        )

        # Add proxy resource to handle all paths
        api.root.add_proxy(
            default_integration=lambda_integration,
            any_method=True,
        )

        # S3 bucket for frontend static site
        frontend_bucket = s3.Bucket(
            self,
            "FrontendBucket",
            bucket_name=f"foamer-{env_name}-frontend",
            website_index_document="index.html",
            website_error_document="index.html",
            public_read_access=True,
            block_public_access=s3.BlockPublicAccess(
                block_public_acls=False,
                block_public_policy=False,
                ignore_public_acls=False,
                restrict_public_buckets=False,
            ),
            removal_policy=RemovalPolicy.DESTROY if env_name == "dev" else RemovalPolicy.RETAIN,
            auto_delete_objects=True if env_name == "dev" else False,
        )

        # Output the API Gateway URL
        CfnOutput(
            self,
            "ApiUrl",
            value=api.url,
            description=f"API Gateway URL for {env_name}",
        )

        # Output the DynamoDB table name
        CfnOutput(
            self,
            "MessagesTableName",
            value=messages_table.table_name,
            description=f"DynamoDB messages table name for {env_name}",
        )

        # Output the S3 bucket name
        CfnOutput(
            self,
            "FrontendBucketName",
            value=frontend_bucket.bucket_name,
            description=f"S3 bucket name for frontend in {env_name}",
        )

        # Output the S3 website URL
        CfnOutput(
            self,
            "FrontendWebsiteUrl",
            value=frontend_bucket.bucket_website_url,
            description=f"S3 website URL for frontend in {env_name}",
        )
