# Prerequisites

- raylib5.5 as a system dependency
- meson + ninja

# How to build

Clone the repository:

```
git clone https://github.com/mkatalanos/quadruple-pendulum.git
cd quadruple-pendulum
```

Configure a build directory:

```
meson setup builddir --reconfigure
```

Build:

```
meson compile -C builddir
```

Run:

```
builddir/pendulums
```
