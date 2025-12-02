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
            pkgs.gcc
            pkgs.gnumake
            pkgs.pciutils
            # dev tools
            pkgs.clang-tools
            pkgs.bear
            pkgs.picocom
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
