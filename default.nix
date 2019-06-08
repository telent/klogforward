{stdenv ? (import <nixpkgs> {}).stdenv, ...} : stdenv.mkDerivation {
  name = "logforward";
  version = "1";
  src = ./.;
  installPhase = ''
    mkdir -p $out/bin
    make install DESTDIR=$out
  '';
}
