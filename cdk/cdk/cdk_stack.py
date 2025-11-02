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
    aws_cloudfront as cloudfront,
    aws_cloudfront_origins as origins,
    aws_route53 as route53,
    aws_route53_targets as targets,
    aws_certificatemanager as acm,
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

        # Look up the Route 53 hosted zone
        hosted_zone = route53.HostedZone.from_lookup(
            self,
            "HostedZone",
            domain_name="foamer.net",
        )

        # Define domain names based on environment
        if env_name == "prod":
            api_domain_name = "api.foamer.net"
            frontend_domain_name = "asakura-eccles.foamer.net"
        else:  # dev
            api_domain_name = f"api-{env_name}.foamer.net"
            frontend_domain_name = f"{env_name}.foamer.net"

        # Create ACM certificate for API domain (must be in us-east-1 for API Gateway)
        api_certificate = acm.Certificate(
            self,
            "ApiCertificate",
            domain_name=api_domain_name,
            validation=acm.CertificateValidation.from_dns(hosted_zone),
        )

        # Create ACM certificate for frontend domain (must be in us-east-1 for CloudFront)
        frontend_certificate = acm.Certificate(
            self,
            "FrontendCertificate",
            domain_name=frontend_domain_name,
            validation=acm.CertificateValidation.from_dns(hosted_zone),
        )

        # Create CloudFront distribution for frontend
        distribution = cloudfront.Distribution(
            self,
            "FrontendDistribution",
            default_behavior=cloudfront.BehaviorOptions(
                origin=origins.S3StaticWebsiteOrigin(frontend_bucket),
                viewer_protocol_policy=cloudfront.ViewerProtocolPolicy.REDIRECT_TO_HTTPS,
            ),
            domain_names=[frontend_domain_name],
            certificate=frontend_certificate,
            default_root_object="index.html",
            error_responses=[
                cloudfront.ErrorResponse(
                    http_status=404,
                    response_page_path="/index.html",
                    response_http_status=200,
                ),
            ],
        )

        # Create custom domain for API Gateway
        api_custom_domain = api.add_domain_name(
            "ApiCustomDomain",
            domain_name=api_domain_name,
            certificate=api_certificate,
        )

        # Create Route 53 A record for API
        route53.ARecord(
            self,
            "ApiAliasRecord",
            zone=hosted_zone,
            record_name=api_domain_name,
            target=route53.RecordTarget.from_alias(
                targets.ApiGateway(api)
            ),
        )

        # Create Route 53 A record for frontend
        route53.ARecord(
            self,
            "FrontendAliasRecord",
            zone=hosted_zone,
            record_name=frontend_domain_name,
            target=route53.RecordTarget.from_alias(
                targets.CloudFrontTarget(distribution)
            ),
        )

        # Output the custom domain URLs
        CfnOutput(
            self,
            "ApiUrl",
            value=f"https://{api_domain_name}",
            description=f"API custom domain URL for {env_name}",
        )

        CfnOutput(
            self,
            "FrontendUrl",
            value=f"https://{frontend_domain_name}",
            description=f"Frontend custom domain URL for {env_name}",
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

        # Output CloudFront distribution ID (useful for cache invalidation)
        CfnOutput(
            self,
            "CloudFrontDistributionId",
            value=distribution.distribution_id,
            description=f"CloudFront distribution ID for {env_name}",
        )
