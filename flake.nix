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

        # Python environment with build dependencies
        pythonEnv = pkgs.python3.withPackages (ps: with ps; [
          pillow
          pyserial
        ]);
      in
      {
        devShells.default = pkgs.mkShell {
          buildInputs = [
            # Firmware build tools
            pkgs.platformio
            pythonEnv
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
      }
    );
}
