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

        esptoolWindowsVersion = "5.4.0";

        esptoolWindowsZip = pkgs.fetchurl {
          url = "https://github.com/espressif/esptool/releases/download/v${esptoolWindowsVersion}/esptool-v${esptoolWindowsVersion}-windows-amd64.zip";
          hash = "sha256-t/a53TAaIQsx9IKRGMkJyEquIxB/nKH9wUzPTXOEvi4=";
        };

        esptoolWindows = pkgs.runCommand
          "esptool-${esptoolWindowsVersion}-windows-amd64"
          {
            nativeBuildInputs = [ pkgs.unzip ];
          }
          ''
            mkdir unpacked
            unzip ${esptoolWindowsZip} -d unpacked
            exe="$(find unpacked -name esptool.exe -print -quit)"

            if [ -z "$exe" ]; then
              echo "esptool.exe not found in archive"
              exit 1
            fi

            mkdir -p $out/bin
            cp "$exe" $out/bin/esptool.exe
          '';
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

            cat > "$instructions" <<'EOF'
            Flashing the Foamer (MatrixPortal S3) - drag and drop, no drivers needed:

            1. Plug the board into your PC with a USB-C DATA cable (not charge-only).
            2. Double-tap the RESET button. A USB drive named MATRXS3BOOT appears.
               (If nothing shows up, double-tap RESET again - timing is the only trick.)
            3. Drag foamer.uf2 onto the MATRXS3BOOT drive.
            4. It copies, the board reboots into the firmware, and the drive disappears. Done.
            EOF

            echo "created $output"
            echo "created $instructions"
          '';
        };

        makeWindowsFlasher = pkgs.writeShellApplication {
          name = "make-windows-flasher";
          runtimeInputs = [
            pkgs.coreutils
            pkgs.esptool
            pkgs.zip
          ];

          text = ''
            set -euo pipefail

            FRAMEWORK="$HOME/.platformio/packages/framework-arduinoespressif32"
            BUILD="firmware/foamer-display/.pio/build/adafruit_matrixportal_esp32s3"

            output="$(pwd)/foamer-windows-flasher.zip"
            work="$(mktemp -d)"
            trap 'rm -rf "$work"' EXIT

            esptool \
              --chip esp32s3 \
              merge-bin \
              --flash-mode dio \
              --flash-freq 80m \
              --flash-size 8MB \
              -o "$work/foamer.bin" \
              0x0000   "$FRAMEWORK/variants/adafruit_matrixportal_esp32s3/bootloader-tinyuf2.bin" \
              0x8000   "$BUILD/partitions.bin" \
              0xe000   "$FRAMEWORK/tools/partitions/boot_app0.bin" \
              0x10000  "$BUILD/firmware.bin" \
              0x410000 "$FRAMEWORK/variants/adafruit_matrixportal_esp32s3/tinyuf2.bin"

            cp ${esptoolWindows}/bin/esptool.exe "$work/esptool.exe"

            cat > "$work/flash.bat" <<'EOF'
@echo off
cd /d "%~dp0"

echo Flashing Foamer firmware...
echo.

esptool.exe ^
  --chip esp32s3 ^
  --baud 460800 ^
  --before default-reset ^
  --after hard-reset ^
  write-flash ^
  --flash-mode dio ^
  --flash-freq 80m ^
  --flash-size 8MB ^
  0x0 foamer.bin

if errorlevel 1 (
    echo.
    echo Flash failed.
    echo Make sure the Foamer is connected over USB.
) else (
    echo.
    echo Flash complete.
)

echo.
pause
EOF
            rm -f "$output"

            (
              cd "$work"
              zip -9 "$output" esptool.exe foamer.bin flash.bat
            )

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

        packages.make-windows-flasher = makeWindowsFlasher;
        packages.make-uf2 = makeUf2;
      }
    );
}
