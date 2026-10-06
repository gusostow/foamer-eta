{
  description = "foamer-eta firmware development environment";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};

        # microsoft/uf2 conversion tooling, pinned to a commit for reproducibility
        uf2conv = pkgs.fetchurl {
          url = "https://raw.githubusercontent.com/microsoft/uf2/f3f9f1ea052c32d7a15e2633b54ad582d8cc7809/utils/uf2conv.py";
          hash = "sha256-rTa6LWH7LqNxgyJiOSCIKB7uR0xgnfQUKtvuTqPCDyY=";
        };

        uf2families = pkgs.fetchurl {
          url = "https://raw.githubusercontent.com/microsoft/uf2/f3f9f1ea052c32d7a15e2633b54ad582d8cc7809/utils/uf2families.json";
          hash = "sha256-VWZS8QWsj4F4f/riz5mYF7MO7kqatVNrjCQxVyi2jKw=";
        };

        # ESP32-S3 UF2 family magic, used by TinyUF2 to pick the write target
        esp32s3Family = "0xc47e5767";

        makeUf2 = pkgs.writeShellApplication {
          name = "make-uf2";
          runtimeInputs = [
            pkgs.coreutils
            pkgs.python3
          ];

          text = ''
            set -euo pipefail

            BUILD="firmware/foamer-display/.pio/build/adafruit_matrixportal_esp32s3"
            app="$BUILD/firmware.bin"

            if [ ! -f "$app" ]; then
              echo "firmware.bin not found at $app"
              echo "build first, e.g. 'make compile PROFILE=prod'"
              exit 1
            fi

            output="$(pwd)/foamer.uf2"
            instructions="$(pwd)/INSTRUCTIONS.txt"

            work="$(mktemp -d)"
            trap 'rm -rf "$work"' EXIT

            # uf2conv.py reads uf2families.json from its own directory, so co-locate them
            cp ${uf2conv} "$work/uf2conv.py"
            cp ${uf2families} "$work/uf2families.json"

            # -b 0x0: TinyUF2 treats this as the start of the OTA app partition
            python3 "$work/uf2conv.py" \
              "$app" \
              -b 0x0 \
              -f ${esp32s3Family} \
              -o "$output"

            echo "created $output"
          '';
        };
      in
      {
        devShells.default = pkgs.mkShell {
          buildInputs = [
            # Firmware build tools
            pkgs.platformio
            pkgs.uv
            pkgs.python3
            pkgs.jq
            pkgs.gnumake
          ];

          shellHook = ''
            echo "foamer-eta firmware dev environment loaded"
            echo "Available commands:"
            echo "  make compile PROFILE=dev|prod - Build firmware"
            echo "  make upload PROFILE=dev|prod  - Flash firmware to device"
            echo "  make monitor                  - Open serial monitor"
          '';
        };

        packages.make-uf2 = makeUf2;
      }
    );
}
