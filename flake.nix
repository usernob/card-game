{
  description = "simple c/c+ dev environment";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = import nixpkgs { inherit system; };
      in
      {
        devShell = pkgs.mkShell rec {
          name = "sdl";

          nativeBuildInputs = with pkgs; [
            cmake
            cmake-lint
            cmake-format
            cmake-language-server
            ninja
            pkg-config
            wayland-scanner
            libgcc
          ];

          buildInputs = with pkgs; [
            zenity
            alsa-lib
            dbus
            libdrm
            libgbm
            pipewire
            libpulseaudio
            sndio
            systemdLibs
            libGL
            libxkbcommon
            wayland
            libdecor
            xorg.libX11
            xorg.libXScrnSaver
            xorg.libXcursor
            xorg.libXext
            xorg.libXfixes
            xorg.libXi
            xorg.libXrandr
            vulkan-headers
            vulkan-loader
          ];

          packages = with pkgs; [
            clang-tools
            valgrind
            gdb
          ];
          LD_LIBRARY_PATH = pkgs.lib.makeLibraryPath buildInputs;
          NIX_LDFLAGS = "-rpath ${pkgs.lib.makeLibraryPath buildInputs}";
        };
      }
    );
}
