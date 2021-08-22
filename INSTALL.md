# Build and installation instructions

## Compile-time build dependencies

 - cmake (>= 3.13)
 - cmake-extras
 - intltool
 - vala (>= 0.20)
 - systemd
 - libayatana-common (>=0.9.3)
 - glib-2.0 (>= 2.36)
 - accountsservice
 - libpulse (>= 5.0)
 - dbus
 - gobject-introspection
 - pulseaudio
 - libnotify
 - libgee
 - libxml2
 - gtest (>= 1.6.0) - **For testing**
 - python3-dbusmock - **For testing**
 - qt5-base5 - **For testing**
 - qt5-declarative5 - **For testing**
 - dbus-test-runner - **For testing**
 - gcovr (>= 2.4) - **For coverage**
 - lcov (>= 1.9) - **For coverage**

## For end-users and packagers

```
cd ayatana-indicator-sound-X.Y.Z
mkdir build
cd build
cmake ..
make
sudo make install
```

**The install prefix defaults to `/usr`, change it with `-DCMAKE_INSTALL_PREFIX=/some/path`**

## For testers - unit tests only

```
cd ayatana-indicator-sound-X.Y.Z
mkdir build
cd build
cmake .. -DENABLE_TESTS=ON
make
make test
make cppcheck
```

## For testers - both unit tests and code coverage

```
cd ayatana-indicator-sound-X.Y.Z
mkdir build-coverage
cd build-coverage
cmake .. -DENABLE_COVERAGE=ON
make
make coverage-html
```
