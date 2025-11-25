{
  description = "Dev shell for drizzle";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs =
    { self, nixpkgs }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs { inherit system; };
    in
    {
      devShells.${system}.default = pkgs.mkShell {
        name = "drizzle";

        buildInputs = with pkgs; [
          clang
          clang-tools
          lld
          libcxx
          llvm

          cmake
          fmt
          pkg-config
          hwloc
          perf
          gnumake
          python313

          vulkan-loader
          vulkan-headers
          vulkan-tools
          vulkan-memory-allocator
          vulkan-validation-layers

          spirv-tools
          shaderc

          glfw
          wayland
          wayland-protocols
          wayland-scanner
          libffi
          libxkbcommon

          fontconfig
          expat
        ];
        shellHook = ''
          export SHELL=${pkgs.zsh}/bin/zsh
          export LD_LIBRARY_PATH=${pkgs.vulkan-loader}/lib:$LD_LIBRARY_PATH

          export VK_LAYER_PATH=${pkgs.vulkan-validation-layers}/share/vulkan/explicit_layer.d

          export CC=clang
          export CXX=clang++
          export CLANGD_FLAGS="$CLANGD_FLAGS --query-driver=/nix/store/*-clang-wrapper-*/bin/clang++"
        '';
      };
    };
}
