{
  description = "Silicon Heaven library Flake";

  inputs.necrolog.url = "github:fvacek/necrolog";

  outputs = {
    self,
    flake-utils,
    nixpkgs,
    necrolog,
  }: let
    inherit (flake-utils.lib) eachDefaultSystem;
    inherit (nixpkgs.lib) hasSuffix composeManyExtensions optional optionals strings;
    inherit (strings) cmakeBool;
    rev = self.shortRev or self.dirtyShortRev or "unknown";

    libshv = {
      stdenv,
      qt6,
      cmake,
      necrolog,
      chainpack ? true,
      chainpack_cpp ? true,
      core ? true,
      coreqt ? true,
      iotqt ? true,
      visu ? true,
      broker ? true,
      cli_examples ? false,
      gui_examples ? false,
    }:
      stdenv.mkDerivation {
        name = "libshv-${rev}";
        src = builtins.path {
          name = "libshv-src";
          path = ./.;
          filter = path: type: ! hasSuffix ".nix" path;
        };
        buildInputs =
          [necrolog]
          ++ (optionals coreqt [
            qt6.wrapQtAppsHook
            qt6.qtbase
            qt6.qtserialport
            qt6.qtwebsockets
          ]);
        nativeBuildInputs = [cmake] ++ (optional coreqt qt6.qttools);
        cmakeFlags = [
          "-DLIBSHV_USE_LOCAL_NECROLOG=ON"
          (cmakeBool "LIBSHV_WITH_CHAINPACK" chainpack)
          (cmakeBool "LIBSHV_WITH_CHAINPACK_CPP" chainpack_cpp)
          (cmakeBool "LIBSHV_WITH_CORE" core)
          (cmakeBool "LIBSHV_WITH_COREQT" coreqt)
          (cmakeBool "LIBSHV_WITH_IOTQT" iotqt)
          (cmakeBool "LIBSHV_WITH_VISU" visu)
          (cmakeBool "LIBSHV_WITH_BROKER" broker)
          (cmakeBool "LIBSHV_WITH_CLI_EXAMPLES" cli_examples)
          (cmakeBool "LIBSHV_WITH_GUI_EXAMPLES" gui_examples)
        ];
      };
  in
    {
      overlays = {
        pkgs = final: prev: {
          libshv = final.callPackage libshv {};
          libshvFull = final.callPackage libshv {
            cli_examples = true;
            gui_examples = true;
          };
          libshvCli = final.callPackage libshv {
            visu = false;
            cli_examples = true;
          };
          libshvForClients = final.callPackage libshv {
            broker = false;
          };
        };
        default = composeManyExtensions [
          necrolog.overlays.default
          self.overlays.pkgs
        ];
      };
    }
    // eachDefaultSystem (system: let
      pkgs = nixpkgs.legacyPackages.${system}.extend self.overlays.default;
    in {
      packages = {
        default = pkgs.libshv;
        full = pkgs.libshvFull;
        cli = pkgs.libshvCli;
        forClients = pkgs.libshvForClients;
      };
      legacyPackages = pkgs;

      formatter = pkgs.alejandra;
    });
}
