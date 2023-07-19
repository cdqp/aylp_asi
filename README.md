Anyloop plugin for ZWO ASI cameras
==================================

Linux only, targeting x64 and arm64.


libaylp dependency
------------------

Symlink or copy the `libaylp` directory from anyloop to `libaylp`. For example:

```sh
ln -s $HOME/git/anyloop/libaylp libaylp
```


libasi dependency
-----------------

The ASI library is bundled with this repo under libasi. It seems the binary is
MIT licensed so we can do that.


Building
--------

Use meson:

```sh
meson setup build
meson compile -C build
```

Note that ZWO did not set a SONAME on their shared library, so meson will link
`aylp_asi` against the absolute path of `libASICamera2.so`.


