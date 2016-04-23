export PATH=/c/msys64/mingw$ABI/bin:$PATH

cd /c/projects/flint2

if [ ! -d "/c/projects/flint2/mpir-2.7.0" ]; then
    ./.build_dependencies
fi

./configure ABI=$ABI --with-mpir=/c/projects/mpir --with-mpfr=/c/projects/mpfr --disable-shared
make -j4
make -j4 check
