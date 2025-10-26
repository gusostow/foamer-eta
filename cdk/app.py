#!/usr/bin/env python3
import os

import aws_cdk as cdk

from cdk.cdk_stack import CdkStack


app = cdk.App()

# Development stack
CdkStack(app, "FoamerDev", env_name="dev")

# Production stack
CdkStack(app, "FoamerProd", env_name="prod")

app.synth()
