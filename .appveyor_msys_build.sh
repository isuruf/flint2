export PATH=/c/msys64/mingw$ABI/bin:$PATH

cd /c/projects/flint2

if [ ! -d "/c/projects/flint2/deps" ]; then
    ./.build_dependencies
fi

./configure ABI=$ABI --with-mpir=deps --with-mpfr=deps --disable-shared
make -j4
make -j4 check
