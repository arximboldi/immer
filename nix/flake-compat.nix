# Minimal Nix flake compatibility layer. It doesn't support all flake
# features and versions, but supports enough for this repository's
# flake usage. Alternatively one may use:
#
#   https://github.com/NixOS/flake-compat
#
# I prefer to have a minimal implementation here that doesn't require
# fetching third party code and can be easily copy-pasted and modified
# if needed.
{
  flakeLock ? ../flake.lock,
}:
let
  lock = builtins.fromJSON (builtins.readFile flakeLock);
  fetchNode = name: node: builtins.fetchTree node.locked;
  inputs = builtins.mapAttrs fetchNode lock.nodes;
  flake = import inputs.root { inherit inputs; };
  outputs = flake.outputs inputs;
in
{
  inherit flake inputs outputs;
}
