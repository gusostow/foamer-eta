from aws_cdk import (
    Stack,
    RemovalPolicy,
    Duration,
    CfnOutput,
    aws_secretsmanager as secretsmanager,
    aws_lambda as lambda_,
    aws_apigatewayv2 as apigw,
)
from aws_cdk.aws_apigatewayv2_integrations import HttpLambdaIntegration
from constructs import Construct

class CdkStack(Stack):

    def __init__(self, scope: Construct, construct_id: str, env_name: str, **kwargs) -> None:
        super().__init__(scope, construct_id, **kwargs)

        # Secrets Manager secret for Transit API key
        transit_key_secret = secretsmanager.Secret(
            self, "TransitKeySecret",
            secret_name=f"foamer/{env_name}/transit-key",
            description=f"Transit API key for {env_name} environment",
            removal_policy=RemovalPolicy.DESTROY if env_name == "dev" else RemovalPolicy.RETAIN,
        )

        # Lambda function using pre-built cargo-lambda output
        # Build with: cargo lambda build -p svc --release --x86-64
        lambda_function = lambda_.Function(
            self, "FoamerSvcFunction",
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
                    self, "LambdaWebAdapterLayer",
                    layer_version_arn=f"arn:aws:lambda:{self.region}:753240598075:layer:LambdaAdapterLayerX86:22"
                )
            ],
        )

        # Add TRANSIT_KEY secret to Lambda
        transit_key_secret.grant_read(lambda_function)
        lambda_function.add_environment(
            "TRANSIT_KEY",
            transit_key_secret.secret_value.unsafe_unwrap()
        )

        # API Gateway HTTP API
        http_api = apigw.HttpApi(
            self, "FoamerHttpApi",
            api_name=f"foamer-{env_name}-api",
            description=f"HTTP API for foamer {env_name} environment",
        )

        # Lambda integration
        lambda_integration = HttpLambdaIntegration(
            "LambdaIntegration",
            lambda_function,
        )

        # Add route - proxy all requests to Lambda
        http_api.add_routes(
            path="/{proxy+}",
            methods=[apigw.HttpMethod.ANY],
            integration=lambda_integration,
        )

        # Output the API Gateway URL
        CfnOutput(
            self, "ApiUrl",
            value=http_api.url,
            description=f"API Gateway URL for {env_name}",
        )
