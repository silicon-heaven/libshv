{
  description = "Silicon Heaven library Flake";

  outputs = {
    self,
    flake-utils,
    nixpkgs,
  }:
    with builtins;
    with flake-utils.lib;
    with nixpkgs.lib; let
      packages = pkgs:
        with pkgs;
        with qt6Packages; rec {
          libshv = stdenv.mkDerivation {
            name = "libshv";
            src = builtins.path {
              name = "libshv-src";
              path = ./.;
              filter = path: type: ! hasSuffix ".nix" path;
            };
            outputs = ["out" "dev"];
            buildInputs = [
              wrapQtAppsHook
              qtbase
              qtserialport
              qtwebsockets
            ];
            nativeBuildInputs = [
              cmake
            ];
            cmakeFlags = ["-DWITH_CLI_EXAMPLES=ON"];
          };
          default = libshv;
        };
    in
      {
        overlays = {
          libshv = final: prev: packages (id prev);
          default = self.overlays.libshv;
        };
      }
      // eachDefaultSystem (system: let
        pkgs = nixpkgs.legacyPackages.${system}.extend self.overlays.default;
      in {
        packages = filterPackages system rec {
          inherit (pkgs) libshv;
          default = libshv;
        };
        legacyPackages = pkgs;

        formatter = pkgs.alejandra;
      });
}
