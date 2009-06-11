#!/bin/sh

if [ -d gedit.app ] && [ "$1x" = "-fx" ]; then
	rm -rf gedit.app
fi

ige-mac-bundler gedit.bundle

# Strip debug symbols
find gedit.app/Contents/Resources | grep -E '\.(so|dylib)' | xargs strip -S
find gedit.app/Contents/Resources/bin | xargs strip -S
strip -S gedit.app/Contents/MacOS/gedit-bin