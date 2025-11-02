{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  buildInputs = [
    pkgs.clang-tools
    pkgs.curl
    pkgs.stb
  ];
}
