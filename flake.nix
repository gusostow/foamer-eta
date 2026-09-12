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
      }
    );
}
