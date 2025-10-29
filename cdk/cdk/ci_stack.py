from aws_cdk import (
    Stack,
    CfnOutput,
    Duration,
    aws_iam as iam,
)
from constructs import Construct

class CIStack(Stack):
    """
    Stack for GitHub Actions CI/CD identity using OIDC.
    This stack is independent of dev/prod and provides a role that
    GitHub Actions can assume to deploy to any environment.
    """

    def __init__(self, scope: Construct, construct_id: str, github_repo: str, **kwargs) -> None:
        super().__init__(scope, construct_id, **kwargs)

        # GitHub OIDC Provider
        github_provider = iam.OpenIdConnectProvider(
            self, "GitHubOidcProvider",
            url="https://token.actions.githubusercontent.com",
            client_ids=["sts.amazonaws.com"],
            thumbprints=["6938fd4d98bab03faadb97b34396831e3780aea1"]  # GitHub's thumbprint
        )

        # IAM Role for GitHub Actions
        github_role = iam.Role(
            self, "GitHubActionsRole",
            role_name="foamer-github-actions-role",
            assumed_by=iam.FederatedPrincipal(
                federated=github_provider.open_id_connect_provider_arn,
                conditions={
                    "StringLike": {
                        "token.actions.githubusercontent.com:sub": f"repo:{github_repo}:*"
                    },
                    "StringEquals": {
                        "token.actions.githubusercontent.com:aud": "sts.amazonaws.com"
                    }
                },
                assume_role_action="sts:AssumeRoleWithWebIdentity"
            ),
            description="Role for GitHub Actions to deploy foamer-eta via CDK",
            max_session_duration=Duration.hours(1),
        )

        # Grant admin permissions for CDK deployment
        # In production, you might want to scope this down to specific resources
        github_role.add_managed_policy(
            iam.ManagedPolicy.from_aws_managed_policy_name("AdministratorAccess")
        )

        # Outputs
        CfnOutput(
            self, "GitHubActionsRoleArn",
            value=github_role.role_arn,
            description="ARN of the role for GitHub Actions to assume",
            export_name="FoamerGitHubActionsRoleArn",
        )

        CfnOutput(
            self, "GitHubOidcProviderArn",
            value=github_provider.open_id_connect_provider_arn,
            description="ARN of the GitHub OIDC provider",
        )
