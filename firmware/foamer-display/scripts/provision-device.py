#!/usr/bin/env python3
"""
Provision a new IoT device:
- Create AWS IoT Thing
- Generate certificate
- Download cert files to device cache
- Create device.json with per-device config
"""

import argparse
import json
import subprocess
import sys
from pathlib import Path


def run(cmd, check=True):
    """Run shell command and return output"""
    result = subprocess.run(
        cmd, shell=True, capture_output=True, text=True, check=check
    )
    if result.returncode != 0 and check:
        print(f"ERROR: Command failed: {cmd}", file=sys.stderr)
        print(result.stderr, file=sys.stderr)
        sys.exit(1)
    return result.stdout.strip()


def provision_device(serial_number: str, profile: str):
    """Provision device in AWS IoT and download certificates"""

    # Normalize serial number (remove spaces, lowercase)
    serial_normalized = serial_number.replace(" ", "").lower()

    # Thing name format: foamer-{profile}-{serial}
    thing_name = f"foamer-{profile}-{serial_normalized}"
    log_topic = f"device/{thing_name}/logs"

    print(f"Provisioning device: {thing_name}")
    print(f"USB Serial: {serial_number}")
    print(f"Profile: {profile}")

    # Get policy name from CDK outputs (or use default)
    policy_name = f"foamer-{profile}-device-policy"

    # Create device cache directory
    project_root = Path(__file__).parent.parent.parent.parent
    device_dir = project_root / "firmware" / "devices" / serial_normalized
    device_dir.mkdir(parents=True, exist_ok=True)

    print(f"Device directory: {device_dir}")

    # Step 1: Create AWS IoT Thing
    print("\n[1/5] Creating IoT Thing...")
    result = run(f"aws iot create-thing --thing-name {thing_name}", check=False)
    if result:
        print(f"  ✓ Thing created: {thing_name}")
    else:
        # Thing might already exist
        existing = run(f"aws iot describe-thing --thing-name {thing_name}", check=False)
        if existing:
            print(f"  ✓ Thing already exists: {thing_name}")
        else:
            print("  ✗ Failed to create thing", file=sys.stderr)
            sys.exit(1)

    # Step 2: Create certificate
    print("\n[2/5] Creating certificate...")
    cert_output = run(
        f"aws iot create-keys-and-certificate --set-as-active "
        f"--certificate-pem-outfile {device_dir}/cert.pem "
        f"--private-key-outfile {device_dir}/private_key.pem "
        f"--public-key-outfile {device_dir}/public_key.pem"
    )
    cert_data = json.loads(cert_output)
    cert_arn = cert_data["certificateArn"]
    print(f"  ✓ Certificate created: {cert_arn}")

    # Step 3: Download Amazon Root CA
    print("\n[3/5] Downloading Amazon Root CA...")
    run(
        f"curl -s https://www.amazontrust.com/repository/AmazonRootCA1.pem -o {device_dir}/root_ca.pem"
    )
    print(f"  ✓ Root CA downloaded")

    # Step 4: Attach policy to certificate
    print("\n[4/5] Attaching policy to certificate...")
    run(f"aws iot attach-policy --policy-name {policy_name} --target {cert_arn}")
    print(f"  ✓ Policy attached: {policy_name}")

    # Step 5: Attach certificate to thing
    print("\n[5/5] Attaching certificate to thing...")
    run(
        f"aws iot attach-thing-principal --thing-name {thing_name} --principal {cert_arn}"
    )
    print(f"  ✓ Certificate attached to thing")

    # Create device.json with per-device config
    device_config = {
        "thing_name": thing_name,
        "log_topic": log_topic,
        "certificate_arn": cert_arn,
    }

    device_json_path = device_dir / "device.json"
    with open(device_json_path, "w") as f:
        json.dump(device_config, f, indent=2)

    print(f"\n✓ Device provisioned successfully!")
    print(f"  Thing name: {thing_name}")
    print(f"  Certificate ARN: {cert_arn}")
    print(f"  Device config: {device_json_path}")
    print(f"  Certificates: {device_dir}/")


def main():
    parser = argparse.ArgumentParser(description="Provision AWS IoT device")
    parser.add_argument("--serial", required=True, help="Device USB serial number")
    parser.add_argument("--profile", required=True, help="Config profile (dev/prod)")

    args = parser.parse_args()

    provision_device(args.serial, args.profile)


if __name__ == "__main__":
    main()
