{
  description = "ATI tests";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
    utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, utils }:
    utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };
      in
      {
        devShells.default = pkgs.mkShell {
          buildInputs = [
            # build
            pkgs.gcc
            pkgs.gnumake
            # real hardware
            pkgs.pciutils
            # dev tools
            pkgs.clang-tools
            pkgs.bear
            # baremetal boot image
            pkgs.libisoburn
            pkgs.mtools
            pkgs.grub2
          ];
          shellHook = ''
            export PATH="$PWD/bin:$PATH"
          '';
        };
      });
}
