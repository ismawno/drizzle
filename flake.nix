{
  description = "Dev shell for drizzle";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-25.05";
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
          cmake
          clang-tools
          clang
          lld
          llvmPackages_19.llvm
          linuxPackages.perf
          fmt
          hwloc
          pkg-config
          wayland
          wayland-protocols
          wayland-scanner
          fontconfig
          expat
          glm
          glfw
          libxkbcommon
          libffi
          python313
          vulkan-headers
          vulkan-loader
          vulkan-tools
          vulkan-memory-allocator
          vulkan-validation-layers
          spirv-tools
          shaderc
        ];
        shellHook = ''
          export SHELL=${pkgs.zsh}/bin/zsh
          export LD_LIBRARY_PATH=${pkgs.vulkan-loader}/lib:$LD_LIBRARY_PATH

          export VK_LAYER_PATH=${pkgs.vulkan-validation-layers}/share/vulkan/explicit_layer.d

          export CC=clang
          export CXX=clang++
        '';
      };
    };
}
