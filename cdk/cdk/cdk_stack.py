from aws_cdk import (
    Stack,
    RemovalPolicy,
    Duration,
    CfnOutput,
    aws_secretsmanager as secretsmanager,
    aws_lambda as lambda_,
    aws_apigateway as apigw,
    aws_dynamodb as dynamodb,
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

        # Grant Lambda access to DynamoDB table and add table name to environment
        messages_table.grant_read_write_data(lambda_function)
        lambda_function.add_environment(
            "FOAMER_MESSAGES_TABLE", messages_table.table_name
        )

        # API Gateway REST API (required for API key support)
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

        # Add proxy resource to handle all paths with API key required
        proxy_resource = api.root.add_proxy(
            default_integration=lambda_integration,
            any_method=False,  # We'll add methods manually to set api_key_required
        )

        # Add ANY method with API key required
        proxy_resource.add_method(
            "ANY",
            lambda_integration,
            api_key_required=True,
        )

        # Create API key with custom value from Secrets Manager
        api_key = apigw.CfnApiKey(
            self,
            f"FoamerApiKey{env_name}",
            name=f"foamer-{env_name}-key",
            enabled=True,
            value=shared_password_secret.secret_value.unsafe_unwrap(),
        )

        # Create usage plan with throttling
        usage_plan = api.add_usage_plan(
            f"FoamerUsagePlan{env_name}",
            name=f"foamer-{env_name}-usage-plan",
            throttle=apigw.ThrottleSettings(
                rate_limit=10,
                burst_limit=20,
            ),
            quota=apigw.QuotaSettings(
                limit=10000,
                period=apigw.Period.DAY,
            ),
        )

        # Associate API key with usage plan using L1 construct
        apigw.CfnUsagePlanKey(
            self,
            "UsagePlanKey",
            key_id=api_key.ref,
            key_type="API_KEY",
            usage_plan_id=usage_plan.usage_plan_id,
        )

        usage_plan.add_api_stage(
            stage=api.deployment_stage,
        )

        # Output the API Gateway URL
        CfnOutput(
            self,
            "ApiUrl",
            value=api.url,
            description=f"API Gateway URL for {env_name}",
        )

        # Output the shared password secret name
        CfnOutput(
            self,
            "SharedPasswordSecretName",
            value=shared_password_secret.secret_name,
            description=f"Secrets Manager secret name for shared password in {env_name}",
        )

        # Output the DynamoDB table name
        CfnOutput(
            self,
            "MessagesTableName",
            value=messages_table.table_name,
            description=f"DynamoDB messages table name for {env_name}",
        )
