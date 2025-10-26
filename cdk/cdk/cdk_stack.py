from aws_cdk import (
    Stack,
    RemovalPolicy,
    aws_s3 as s3,
    aws_ecr as ecr,
    aws_secretsmanager as secretsmanager,
)
from constructs import Construct

class CdkStack(Stack):

    def __init__(self, scope: Construct, construct_id: str, env_name: str, **kwargs) -> None:
        super().__init__(scope, construct_id, **kwargs)

        # Test S3 bucket
        bucket = s3.Bucket(
            self, "FoamerTestBucket",
            bucket_name=f"foamer-{env_name}-bucket",
            versioned=False,
            removal_policy=RemovalPolicy.DESTROY,
            auto_delete_objects=True,
        )

        # ECR repository for svc container images
        ecr_repository = ecr.Repository(
            self, "FoamerSvcRepository",
            repository_name=f"foamer-{env_name}-svc",
            image_scan_on_push=True,
            removal_policy=RemovalPolicy.DESTROY if env_name == "dev" else RemovalPolicy.RETAIN,
            empty_on_delete=True if env_name == "dev" else False,
            lifecycle_rules=[
                ecr.LifecycleRule(
                    description="Keep last 5 images",
                    max_image_count=5,
                )
            ],
        )

        # Secrets Manager secret for Transit API key
        transit_key_secret = secretsmanager.Secret(
            self, "TransitKeySecret",
            secret_name=f"foamer/{env_name}/transit-key",
            description=f"Transit API key for {env_name} environment",
            removal_policy=RemovalPolicy.DESTROY if env_name == "dev" else RemovalPolicy.RETAIN,
        )
