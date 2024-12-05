{
  description = "Particle System in Metals in Pure C++23";

  outputs =
    inputs@{ flake-parts, ... }:
    flake-parts.lib.mkFlake { inherit inputs; } {
      systems = [
        "aarch64-darwin"
        "x86_64-darwin"
      ];
      perSystem =
        {
          lib,
          pkgs,
          config,
          ...
        }:
        let
          llvm = pkgs.llvmPackages_19;
          stdenv = llvm.stdenv;
        in
        {
          packages.particles = stdenv.mkDerivation {
            pname = "particles";
            version = "0.1.0";
            src = ./src;

            outputs = [
              "out"
              "development"
            ];

            buildInputs = [
              pkgs.apple-sdk_15
              llvm.libcxx

              (pkgs.darwinMinVersionHook "12.0")
            ];

            FLAGS = [
              "--start-no-unused-arguments"
              "-std=c++23"
              "-stdlib=libc++"
              "-fstrict-enums"
              "-fsanitize=undefined"
              "-fsanitize=address"
              "-fcoroutines"
              "-flto"
              "-fno-exceptions"
              "-fno-rtti"
              "-fno-threadsafe-statics"
              "-fno-operator-names"
              "-fno-common"
              "-fvisibility=hidden"
              "-Wall"
              "-Wextra"
              "-Werror"
              "-Wpedantic"
              "-Wconversion"
            ];

            preBuild = ''
              mkdir -p pcms
              $CXX  -Wno-reserved-identifier -Wno-reserved-module-identifier \
                    --precompile -o pcms/std.pcm ${llvm.libcxx}/share/libc++/v1/std.cppm $FLAGS
            '';

            buildPhase = ''
              runHook preBuild
              $CXX main.cc -o particles -fprebuilt-module-path=pcms -MJ particles.o.json $FLAGS
            '';

            installPhase = ''
              install -D -t $out/bin particles
              install -D -t $development/pcms pcms/*
              install -D -t $development/fragments *.o.json
            '';

            meta.mainProgram = "bin/particles";
          };

          devShells.default = pkgs.mkShell.override { inherit stdenv; } {
            packages = [ llvm.clang-tools ];
            inputsFrom = [ config.packages.particles ];
          };

          apps.ccdb = {
            type = "app";
            program = lib.getExe (
              pkgs.writeShellApplication {
                name = "ccdb";
                runtimeInputs = [ pkgs.gnused ];
                text =
                  let
                    proj = config.packages.particles.development;
                  in
                  ''
                    sed  -e '1s/^/[\n/' -e '$s/,$/\n]/' \
                         -e "s|/private/tmp/nix-build-${proj.name}.drv-0|$(pwd)|g" \
                         -e "s|prebuilt-module-path=pcms|prebuilt-module-path=${proj}/pcms|g" \
                         ${proj}/fragments/*.o.json \
                         > compile_commands.json
                  '';
              }
            );
          };

          packages.default = config.packages.particles;
        };
    };

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-parts.url = "github:hercules-ci/flake-parts";
  };
}
