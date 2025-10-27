from aws_cdk import (
    Stack,
    RemovalPolicy,
    Duration,
    CfnOutput,
    aws_secretsmanager as secretsmanager,
    aws_lambda as lambda_,
)
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

        # Add Function URL for direct HTTP access (no API Gateway needed)
        function_url = lambda_function.add_function_url(
            auth_type=lambda_.FunctionUrlAuthType.NONE,  # Public access
        )

        # Output the Function URL
        CfnOutput(
            self, "FunctionUrl",
            value=function_url.url,
            description=f"Lambda Function URL for {env_name}",
        )
