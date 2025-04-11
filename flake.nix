{
  description = ''
    huwaireb/mandelbulb: A Mandelbulb rendered using Apple's Metal Graphics API (C++23). Built with Nix (SP)
  '';

  outputs =
    inputs:
    inputs.parts.lib.mkFlake { inherit inputs; } {
      systems = [
        "aarch64-darwin"
        "x86_64-darwin"
      ];
      perSystem =
        { pkgs, config, ... }:
        let
          llvmPackages = pkgs.llvmPackages_19;
          inherit (llvmPackages) stdenv;
        in
        {
          packages.mandelbulb = pkgs.callPackage ./package.nix {
            inherit llvmPackages stdenv;
          };

          devShells.default = pkgs.mkShell.override { inherit stdenv; } {
            packages = [ (pkgs.ccls.override { inherit llvmPackages; }) ];
            inputsFrom = [ config.packages.mandelbulb ];
          };

          apps.ccdb = {
            type = "app";
            program = pkgs.lib.getExe (
              pkgs.writeShellApplication {
                name = "ccdb";
                runtimeInputs = [ pkgs.gnused ];
                text =
                  let
                    proj = config.packages.mandelbulb.development;
                  in
                  ''
                    sed  -e '1s/^/[\n/' -e '$s/,$/\n]/' \
                         -e "s|/private/tmp/nix-build-${proj.name}.drv-0\(/source\)\?|$(pwd)|g" \
                         -e "s|prebuilt-module-path=pcms|prebuilt-module-path=${proj}/pcms|g" \
                         ${proj}/fragments/*.o.json \
                         > compile_commands.json
                  '';
              }
            );
          };

          packages.default = config.packages.mandelbulb;
        };
    };

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    parts = {
      url = "github:hercules-ci/flake-parts";
      inputs.nixpkgs-lib.follows = "nixpkgs";
    };
  };
}
