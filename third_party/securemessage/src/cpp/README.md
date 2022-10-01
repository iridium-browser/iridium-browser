# Compilation

## Build with make
Compile the dependencies first:

```$ make deps```

Then make everything:

```$ make```

## Build with CMake
```
cd <project source root>
mkdir build; cd build
cmake -Dsecuremessage_USE_LOCAL_PROTOBUF=ON ..
make
```

# Running tests

## Tests with make-based build
To run tests do:

```$ make run_tests```

## Tests with CMake-based build

```
cd <project source root>/build
ctest -V
```


# Language specific details
This library has been created to run on a number of systems.  Specifically,
layers of indirection were built such that it is able to be run on systems such
as iOS (by simply replacing the wrapper classes) and on systems with
custom/non-openssl implementations (by providing alternative implementations of
the code in ```crypto_ops_openssl.cc```).
