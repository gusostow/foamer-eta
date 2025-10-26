from aws_cdk import (
    Stack,
    RemovalPolicy,
    aws_s3 as s3,
    aws_ecr as ecr,
    aws_secretsmanager as secretsmanager,
    aws_ecs as ecs,
    aws_logs as logs,
    aws_ec2 as ec2,
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

        # Use default VPC
        vpc = ec2.Vpc.from_lookup(self, "DefaultVPC", is_default=True)

        # ECS Cluster
        cluster = ecs.Cluster(
            self, "FoamerCluster",
            cluster_name=f"foamer-{env_name}-cluster",
            vpc=vpc,
            container_insights=True,
        )

        # CloudWatch Log Group
        log_group = logs.LogGroup(
            self, "FoamerSvcLogGroup",
            log_group_name=f"/ecs/foamer-{env_name}-svc",
            retention=logs.RetentionDays.ONE_WEEK if env_name == "dev" else logs.RetentionDays.ONE_MONTH,
            removal_policy=RemovalPolicy.DESTROY,
        )

        # Task Definition
        task_definition = ecs.FargateTaskDefinition(
            self, "FoamerSvcTaskDef",
            family=f"foamer-{env_name}-svc",
            cpu=256,
            memory_limit_mib=512,
        )

        # Container Definition
        container = task_definition.add_container(
            "FoamerSvcContainer",
            image=ecs.ContainerImage.from_ecr_repository(ecr_repository, tag="latest"),
            logging=ecs.LogDriver.aws_logs(
                stream_prefix="svc",
                log_group=log_group,
            ),
            environment={},
            secrets={
                "TRANSIT_KEY": ecs.Secret.from_secrets_manager(transit_key_secret),
            },
        )

        # Port mapping
        container.add_port_mappings(
            ecs.PortMapping(
                container_port=3000,
                protocol=ecs.Protocol.TCP,
            )
        )

        # Grant read access to the secret
        transit_key_secret.grant_read(task_definition.task_role)
