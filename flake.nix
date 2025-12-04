{
  description = "Silicon Heaven library Flake";

  outputs = {
    self,
    systems,
    nixpkgs,
  }: let
    inherit (nixpkgs.lib) genAttrs optional optionals cmakeBool;
    forSystems = genAttrs (import systems);
    withPkgs = func: forSystems (system: func self.legacyPackages.${system});
    rev = self.shortRev or self.dirtyShortRev or "unknown";

    libshv = {
      stdenv,
      qt6,
      cmake,
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
        src = ./.;
        buildInputs = optionals coreqt [
          qt6.wrapQtAppsHook
          qt6.qtbase
          qt6.qtnetworkauth
          qt6.qtserialport
          qt6.qtwebsockets
        ];
        nativeBuildInputs = [cmake] ++ (optional coreqt qt6.qttools);
        cmakeFlags = [
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
  in {
    overlays.default = final: _: {
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

    packages = withPkgs (pkgs: {
      default = pkgs.libshv;
      full = pkgs.libshvFull;
      cli = pkgs.libshvCli;
      forClients = pkgs.libshvForClients;
    });

    legacyPackages =
      forSystems (system:
        nixpkgs.legacyPackages.${system}.extend self.overlays.default);

    formatter = withPkgs (pkgs: pkgs.alejandra);
  };
}
