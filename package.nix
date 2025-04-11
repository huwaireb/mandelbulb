{
  lib,
  llvmPackages,
  stdenv,
  apple-sdk_15,
  darwinMinVersionHook,
}:
stdenv.mkDerivation {
  pname = "mandelbulb";
  version = "0.1.0";
  src = lib.fileset.toSource {
    root = ./.;
    fileset = lib.fileset.unions [
      ./include
      ./src
    ];
  };

  outputs = [
    "out"
    "development"
  ];

  buildInputs = [
    apple-sdk_15
    llvmPackages.libcxx

    (darwinMinVersionHook "12.0")
  ];

  FLAGS = [
    "--start-no-unused-arguments"
    "-std=c++23"
    "-stdlib=libc++"
    "-fstrict-enums"
    "-fsanitize=undefined"
    "-fsanitize=address"
    "-flto"
    "-fno-exceptions"
    "-fno-rtti"
    "-fno-threadsafe-statics"
    "-fno-operator-names"
    "-fno-common"
    "-fvisibility=hidden"
    "-Wall"
    "-Wconversion"
    "-Wno-c23-extensions" # Embedding the shader in-source using #embed
  ];

  preBuild = ''
    mkdir -p pcms
    $CXX  -Wno-reserved-identifier -Wno-reserved-module-identifier --precompile \
          -o pcms/std.pcm ${llvmPackages.libcxx}/share/libc++/v1/std.cppm $FLAGS
  '';

  buildPhase = ''
    runHook preBuild
    $CXX src/main.cc src/AppDelegate.cc src/MTKViewDelegate.cc src/Renderer.cc \
         -o mandelbulb -fprebuilt-module-path=pcms -Iinclude -lobjc -framework MetalKit \
         -framework AppKit -framework Metal -framework QuartzCore -framework Foundation \
         -MJ mandelbulb.o.json $FLAGS
  '';

  installPhase = ''
    install -D -t $out/bin mandelbulb
    install -D -t $development/pcms pcms/*
    install -D -t $development/fragments *.o.json
  '';

  meta.mainProgram = "mandelbulb";
}
