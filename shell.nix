with (import <nixpkgs> {});

let logforward = callPackage ./default.nix {};
in logforward.overrideAttrs(o:
   let nb = if o ? nativeBuildInputs then o.nativeBuildInputs else [] ; in { 
        nativeBuildInputs = nb ++ [ pkgs.gdb ];
        })
        
