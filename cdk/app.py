#!/usr/bin/env python3
import os

import aws_cdk as cdk

from cdk.cdk_stack import CdkStack


app = cdk.App()

# Get account and region from environment
env = cdk.Environment(
    account=os.environ.get("CDK_DEFAULT_ACCOUNT"),
    region=os.environ.get("CDK_DEFAULT_REGION", "us-east-1")
)

# Development stack
CdkStack(app, "FoamerDev", env_name="dev", env=env)

# Production stack
CdkStack(app, "FoamerProd", env_name="prod", env=env)

app.synth()
